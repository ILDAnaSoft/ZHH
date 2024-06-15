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

std::vector<int> find_process_meta(std::string process) {
	std::vector<int> res;

	if (ProcessMap.find(process) != ProcessMap.end()) {
		res = ProcessMap.at(process);
	} else {
		res.push_back(PROCESS_INVALID);
		res.push_back(PROCESS_INVALID);
		res.push_back(PROCESS_INVALID);
	}

	return res;
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
	m_pTTree->Branch("final_state_counts", &m_final_state_counts);

	m_pTTree->Branch("process", &m_process);
	m_pTTree->Branch("event_category", &m_event_category);
	m_pTTree->Branch("event_category_zhh", &m_event_category_zhh);
	m_pTTree->Branch("n_fermion", &m_n_fermion);
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

	m_event_category = EVENT_CATEGORY_TRUE::OTHER;
	m_event_category_zhh = EVENT_CATEGORY_ZHH::OTHER;
	m_process = 0;
	m_n_fermion = 0;
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

	// Extract process meta data
	std::vector<int> fs_metadata = find_process_meta(pLCEvent->getParameters().getStringVal("processName"));

	try {
		streamlog_out(DEBUG0) << "        getting jet collection: " << m_mcParticleCollection << std::endl ;
		LCCollection *inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		if (fs_metadata[0] != PROCESS_INVALID) {
			m_process = fs_metadata[0];
			m_n_fermion = fs_metadata[1];
			m_n_higgs = fs_metadata[2];

			const EVENT::MCParticle *mcParticle;

			// Final state data for other particles
			for (size_t i = 3; i < fs_metadata.size() - m_n_higgs; i++) {
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

			// Set specific event category
			// Collect counts of final state particles
			size_t n_it = 0;
			for (auto const& [key, value] : m_final_state_counts) {
				m_final_state_counts[key] = count(m_final_states.begin(), m_final_states.end(), PDG_NUMBERING[n_it]);
				n_it++;
			}

			// Classify first leptonic, then neutrino, then hadronic channels
			// 1 Leptonic states
			if ( m_n_fermion >= 4 ) {
				if ( m_n_higgs == 1 ) {
					if ( m_final_state_counts["e"] == 2 || m_final_state_counts["μ"] == 2 || m_final_state_counts["τ"] == 2 ) {
						m_event_category = EVENT_CATEGORY_TRUE::μμbb;
					}
				}
			}
			
			// Set ZHH event category
			if (m_process >= 1111 && m_process <= 1113) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::LEPTONIC;
				m_event_category = EVENT_CATEGORY_TRUE::llHH;
			} else if (m_process == 1311 || m_process == 1312) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::NEUTRINO;
				m_event_category = EVENT_CATEGORY_TRUE::vvHH;
			} else if (m_process == 1511) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::HADRONIC;
				m_event_category = EVENT_CATEGORY_TRUE::qqHH;
			} else {
				// other event categories
				if (m_n_higgs == 1) {
					if (m_process == ) {
						
					}

				}
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
