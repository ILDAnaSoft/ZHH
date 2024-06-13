#include "FinalStateRecorder.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCIntVec.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;

bool haveSameParent(MCParticle* p1, MCParticle* p2) {
	return false;
}

FinalStateRecorder aFinalStateRecorder ;

FinalStateRecorder::FinalStateRecorder() :

  Processor("FinalStateRecorder"),
  m_nRun(0),
  m_nEvt(0)

{

	_description = "FinalStateRecorder writes relevant observables to root-file " ;

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);

  	registerProcessorParameter("outputRootFilename",
				"name of output root file",
				m_outputRootFile,
				std::string("FinalStates.root")
				);

	registerProcessorParameter("outputJsonFilename",
				"name of output json file",
				m_outputJsonFile,
				std::string("FinalStates.json")
				);
}

void FinalStateRecorder::init()
{
	streamlog_out(DEBUG) << "   init called  " << std::endl;
	this->Clear();

	m_nRun = 0;
	m_nEvt = 0;
	m_nEvtSum = 0;

	m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree", "eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");

	m_pTTree->Branch("error_code", &m_errorCode);
	m_pTTree->Branch("final_states", &m_final_states);
	m_pTTree->Branch("final_states_h_decay", &m_final_states_h_decay);
	m_pTTree->Branch("final_state_string", &m_final_state_string);
	m_pTTree->Branch("process", &m_process);
	m_pTTree->Branch("event_category", &m_event_category);
	m_pTTree->Branch("n_higgs", &m_n_higgs);

	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void FinalStateRecorder::Clear() 
{
	streamlog_out(DEBUG) << "   Clear called  " << std::endl;

	m_errorCode = ERROR_CODES::OK;
	m_final_states.clear();
	m_final_states_h_decay.clear();

	m_final_state_string = "";
	for (auto const& [key, value] : m_final_state_counts)
		m_final_state_counts[key] = 0;

	m_process = "";
	m_event_category = EVENT_CATEGORY_TRUE::OTHER;
	m_n_higgs = 0;
}
void FinalStateRecorder::processRunHeader( LCRunHeader*  /*run*/) { 
	m_nRun++ ;
} 

void FinalStateRecorder::processEvent( EVENT::LCEvent *pLCEvent )
{
	// Initialize JSON metadata file
	if (m_errorCode == ERROR_CODES::UNINITIALIZED) {
		m_jsonFile["beamPol1"] = pLCEvent->getParameters().getFloatVal("beamPol1");
		m_jsonFile["beamPol2"] = pLCEvent->getParameters().getFloatVal("beamPol2");
		m_jsonFile["crossSection"] = pLCEvent->getParameters().getFloatVal("crossSection");
	}

	this->Clear();

	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << std::endl;

	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();
	m_nEvtSum++;
	m_process = pLCEvent->getParameters().getStringVal("processName");

	LCCollection *inputMCParticleCollection{};

	try {
		streamlog_out(DEBUG0) << "        getting jet collection: " << m_mcParticleCollection << std::endl ;
		inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		if (FinalStateMap.find(m_process) != FinalStateMap.end()) {
			std::vector<int> fs_metadata = FinalStateMap.at(m_process);
			m_n_higgs = fs_metadata[0];

			const EVENT::MCParticle *mcParticle;

			// Final state data for other particles
			for (size_t i = 1; i < fs_metadata.size() - m_n_higgs; i++) {
				mcParticle = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(fs_metadata[i]));
				m_final_states.push_back(abs(mcParticle->getPDG()));
			}

			// Final state data for Higgs children
			for (size_t i = fs_metadata.size() - m_n_higgs; i < fs_metadata.size(); i++) {
				mcParticle = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(fs_metadata[i]));

				for (unsigned int j = 0; j < mcParticle->getDaughters().size(); j++) {
					m_final_states_h_decay.push_back(abs((mcParticle->getDaughters()[j])->getPDG()));
				}

				m_final_states.push_back(25);
			}
			
			// Set ZHH event category
			if (m_process == "e1e1hh" || m_process == "e2e2hh" || m_process == "e3e3hh")
				m_event_category = EVENT_CATEGORY_ZHH::LEPTONIC;
			else if (m_process == "n1n1hh" || m_process == "n23n23hh")
				m_event_category = EVENT_CATEGORY_ZHH::NEUTRINO;
			else if (m_process == "qqhh")
				m_event_category = EVENT_CATEGORY_ZHH::HADRONIC;

			// Set specific event category
			// Collect counts of final state particles
			size_t n_it = 0;
			for (auto const& [key, value] : m_final_state_counts) {
				m_final_state_counts[key] = count(m_final_states.begin(), m_final_states.end(), PDG_NUMBERING[n_it]);
				n_it++;
			}

			// Classify first leptonic, then neutrino, then hadronic channels
			size_t n_fs = m_final_states.size();
			// 1 Leptonic states
			if ( n_fs >= 4 ) {
				if ( m_n_higgs == 1 ) {
					if ( m_final_state_counts["e"] == 2 || m_final_state_counts["μ"] == 2 || m_final_state_counts["τ"] == 2 ) {
						m_event_category = EVENT_CATEGORY_TRUE::μμbb;
					}
				}
			}

			switch (m_final_states.size()) {
				case 4:
					break;
				
				default:
					break;
			}

		} else {
			m_errorCode = ERROR_CODES::PROCESS_NOT_FOUND;
		}

	} catch(DataNotAvailableException &e) {
		m_errorCode = ERROR_CODES::COLLECTION_NOT_FOUND;
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
	}

	m_pTTree->Fill();
}

void FinalStateRecorder::check()
{
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}


void FinalStateRecorder::end()
{
	// Write ROOT file
	m_pTFile->cd();
	m_pTTree->Write();
	m_pTFile->Close();

	delete m_pTFile;

	// Write JSON metadata file
	m_jsonFile["nEvtSum"] = m_nEvtSum;

	std::ofstream file(m_outputJsonFile);
	file << m_jsonFile;
}
