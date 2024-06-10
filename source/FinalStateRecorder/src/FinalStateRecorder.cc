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

	m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree", "eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");

	m_pTTree->Branch("error_code", &m_errorCode);
	m_pTTree->Branch("final_states", &m_final_states);
	m_pTTree->Branch("final_state_string", &m_final_state_string);
	m_pTTree->Branch("process", &m_process);
	m_pTTree->Branch("zhh_channel", &m_zhh_channel);
	m_pTTree->Branch("n_higgs", &m_n_higgs);

	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void FinalStateRecorder::Clear() 
{
	streamlog_out(DEBUG) << "   Clear called  " << std::endl;

	m_errorCode = ERROR_CODES::OK;
	m_final_states.clear();
	m_final_state_string = "";
	m_process = "";
	m_zhh_channel = ZHH_CHANNEL::NONE;
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
	m_process = pLCEvent->getParameters().getStringVal("processName");

	LCCollection *inputMCParticleCollection{};

	try {
		streamlog_out(DEBUG0) << "        getting jet collection: " << m_mcParticleCollection << std::endl ;
		inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		if (FinalStateMap.find(m_process) != FinalStateMap.end()) {
			std::vector<int> fs_metadata = FinalStateMap.at(m_process);
			int n_higgs = fs_metadata[0];

			const EVENT::MCParticle *mcParticle;

			// Final state data for other particles
			for (size_t i = 1; i < fs_metadata.size() - n_higgs; i++) {
				mcParticle = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(fs_metadata[i]));
				m_final_states.push_back(abs(mcParticle->getPDG()));
			}

			// Final state data for Higgs particles
			for (size_t i = fs_metadata.size() - n_higgs; i < fs_metadata.size(); i++) {
				mcParticle = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(fs_metadata[i]));

				for (unsigned int j = 0; j < mcParticle->getDaughters().size(); j++) {
					m_final_states.push_back(abs((mcParticle->getDaughters()[j])->getPDG()));
				}

				m_final_state_string += "H";
				m_n_higgs++;
			}
			
			// Set final_state_string
			if (m_process == "e1e1hh" || m_process == "e2e2hh" || m_process == "e3e3hh")
				m_final_state_string = ZHH_CHANNEL::LEPTONIC;
			else if (m_process == "n1n1hh" || m_process == "n23n23hh")
				m_final_state_string = ZHH_CHANNEL::NEUTRINO;
			else if (m_process == "qqhh")
				m_final_state_string = ZHH_CHANNEL::HADRONIC;

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
	std::ofstream file(m_outputJsonFile);
	file << m_jsonFile;
}
