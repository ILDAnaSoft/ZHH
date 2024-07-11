#include "FinalStateRecorder.h"
#include "FinalStates.h"
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
  m_nEvt(0),
  m_nEvtSum(0),
  m_beamPol1(0.),
  m_beamPol2(0.),
  m_crossSection(0.),
  m_crossSection_err(0.),
  m_errorCode(ERROR_CODES::UNINITIALIZED),
  m_eventWeight(1.)
{

	_description = "FinalStateRecorder writes relevant observables to root-file " ;

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollectionAlt" ,
				"Name of an alternative MCParticle collection in case MCParticleCollection is not found"  ,
				m_mcParticleCollectionAlt,
				std::string("MCParticlesSkimmed")
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
	this->clear();

	m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree", "eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");

	m_pTTree->Branch("error_code", &m_errorCode);
	m_pTTree->Branch("final_states", &m_final_states);
	m_pTTree->Branch("final_state_counts", &m_final_state_counts);

	m_pTTree->Branch("process", &m_process);
	m_pTTree->Branch("event_category", &m_event_category);
	m_pTTree->Branch("event_category_zhh", &m_event_category_zhh);
	m_pTTree->Branch("n_fermion", &m_n_fermion);
	m_pTTree->Branch("n_higgs", &m_n_higgs);

	// Register physics processes / final state resolvers
	// hh2f
	this->register_process(new e1e1hh());
	this->register_process(new e2e2hh());
	this->register_process(new e3e3hh());
	this->register_process(new qqhh());
	this->register_process(new n1n1hh());
	this->register_process(new n23n23hh());

	// h2f
	this->register_process(new e1e1qqh());
	this->register_process(new e2e2qqh());
	this->register_process(new e3e3qqh());
	this->register_process(new qqqqh());
	this->register_process(new n1n1qqh());
	this->register_process(new n23n23qqh());

	// 2f
	this->register_process(new ll());
	this->register_process(new qq());
	this->register_process(new vv());
	this->register_process(new ee1());
	this->register_process(new ee2());

	// 4f
	this->register_process(new llll_zz());
	this->register_process(new qqqq_zz());
	this->register_process(new llqq_zz());
	this->register_process(new llll_ww());
	this->register_process(new qqqq_ww());
	this->register_process(new llqq_ww());
	this->register_process(new llll_zzorww());
	this->register_process(new qqqq_zzorww());
	this->register_process(new llll_sw());
	this->register_process(new llqq_sw());
	this->register_process(new llll_sze());
	this->register_process(new llqq_sze());
	this->register_process(new llvv_sznu());
	this->register_process(new vvqq_sznu());
	this->register_process(new llvv_szeorsw());

	// 5f

	// 6f


	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void FinalStateRecorder::clear() 
{
	streamlog_out(DEBUG) << "   clear called  " << std::endl;

	m_errorCode = ERROR_CODES::UNKNOWN_ERROR;
	m_final_states.clear();

	for (auto const& [key, value] : m_final_state_counts)
		m_final_state_counts[key] = 0;

	m_process = 0;
	m_n_fermion = 0;
	m_n_higgs = 0;

	m_event_category = EVENT_CATEGORY_TRUE::OTHER;
	m_event_category_zhh = EVENT_CATEGORY_ZHH::OTHER;
}
void FinalStateRecorder::processRunHeader( LCRunHeader*  /*run*/) { 
	m_nRun++ ;
} 

void FinalStateRecorder::processEvent( EVENT::LCEvent *pLCEvent )
{
	pLCEvent->getWeight();

	// Initialize JSON metadata file
	if (m_nEvt == 0) {
		m_beamPol1 = pLCEvent->getParameters().getFloatVal("Pol0");
		m_beamPol2 = pLCEvent->getParameters().getFloatVal("Pol1");
		m_crossSection = pLCEvent->getParameters().getFloatVal("crossSection");
		m_crossSection_err = pLCEvent->getParameters().getFloatVal("crossSectionError");
		m_eventWeight = pLCEvent->getWeight();
		m_process_id = pLCEvent->getParameters().getIntVal("ProcessID");
		m_process_name = pLCEvent->getParameters().getStringVal("processName");
	}

	this->clear();

	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << std::endl;
	
	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();
	m_nEvtSum++;

	// Extract process meta data
	std::string process = pLCEvent->getParameters().getStringVal("processName");
	std::vector<int> fs_metadata = find_process_meta(process);

	try {
		LCCollection *inputMCParticleCollection;
		try {
			streamlog_out(DEBUG0) << "        getting jet collection: " << m_mcParticleCollection << std::endl ;
			inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );
		} catch(DataNotAvailableException &e) {
			streamlog_out(DEBUG0) << "        jet collection not found. using alternative collection: " << m_mcParticleCollectionAlt << std::endl ;
			inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollectionAlt );
		}

		if (m_resolvers.find(process) != m_resolvers.end()) {
			FinalStateResolver* resolver = m_resolvers.at(process);

			m_process = resolver->get_process_id();
			m_event_category = resolver->get_event_category();
			m_n_fermion = resolver->get_n_fermions();
			m_n_higgs = resolver->get_n_higgs();

			// Get final state information
			m_final_states = resolver->m_resolve(inputMCParticleCollection);

			for (size_t i = 0; i < m_final_states.size(); i++) {
				int particle_pdg = abs(m_final_states[i]);

				if (m_final_state_counts.find(abs(particle_pdg)) != m_final_state_counts.end()) {
					m_final_state_counts[particle_pdg]++;
				} else {
					throw ERROR_CODES::UNALLOWED_VALUES;
				}
				
			}
			
			// Set ZHH event category
			if (m_process >= PROCESS_ID::e1e1hh && m_process <= PROCESS_ID::e2e2hh) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::LEPTONIC;
			} else if (m_process == PROCESS_ID::n1n1hh || m_process == PROCESS_ID::n23n23hh) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::NEUTRINO;
			} else if (m_process == PROCESS_ID::qqhh) {
				m_event_category_zhh = EVENT_CATEGORY_ZHH::HADRONIC;
			}
			
			m_errorCode = ERROR_CODES::OK;
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
	m_jsonFile["run"] = m_nRun;
	m_jsonFile["nEvtSum"] = m_nEvtSum;
	m_jsonFile["polElectron"] = m_beamPol1;
	m_jsonFile["polPositron"] = m_beamPol2;
	m_jsonFile["crossSection"] = m_crossSection;
	m_jsonFile["crossSectionError"] = m_crossSection_err;
	m_jsonFile["eventWeight"] = m_eventWeight;
	m_jsonFile["processId"] = m_process_id;
	m_jsonFile["processName"] = m_process_name;

	std::ofstream file(m_outputJsonFile);
	file << m_jsonFile;
}

std::vector<int> FinalStateRecorder::find_process_meta(std::string process) {
	std::vector<int> res;

	if (m_resolvers.find(process) != m_resolvers.end()) {
		FinalStateResolver* resolver = m_resolvers.at(process);

		res.push_back(resolver->get_process_id());
		res.push_back(resolver->get_event_category());
		res.push_back(resolver->get_n_fermions());
		res.push_back(resolver->get_n_higgs());
	} else {
		res.push_back(PROCESS_INVALID);
		res.push_back(PROCESS_INVALID);
		res.push_back(PROCESS_INVALID);
		res.push_back(PROCESS_INVALID);
	}

	return res;
}