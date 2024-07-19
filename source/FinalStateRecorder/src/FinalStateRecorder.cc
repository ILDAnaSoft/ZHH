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
  m_beamPol1(0.),
  m_beamPol2(0.),
  m_crossSection(0.),
  m_crossSection_err(0.),
  m_eventWeight(1.),
  m_process_id(0),
  m_n_run(0),
  m_n_evt(0),
  m_n_evt_sum(0),
  m_error_code(ERROR_CODES::UNINITIALIZED),
  m_process(0),
  m_event_category(EVENT_CATEGORY_TRUE::OTHER),
  m_event_category_zhh(EVENT_CATEGORY_ZHH::OTHER),
  m_n_fermion(0),
  m_n_higgs(0)
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
	this->clear();

	m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree", "eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_n_run, "run/I");
	m_pTTree->Branch("event", &m_n_evt, "event/I");

	m_pTTree->Branch("error_code", &m_error_code);
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
	this->register_process(new f5_ae_eeevv());
	this->register_process(new f5_ea_eyyyy());
	this->register_process(new f5_ae_elevv());
	this->register_process(new f5_ae_eyyyy());
	this->register_process(new f5_ea_exxxx());
	this->register_process(new f5_ae_evvxx());
	this->register_process(new f5_ae_eeeyy());
	this->register_process(new f5_ae_eevxy());
	this->register_process(new f5_ae_lvvyy());
	this->register_process(new f5_ea_eeevv());
	this->register_process(new f5_ea_ellxx());
	this->register_process(new f5_ae_eeeee());
	this->register_process(new f5_ea_elvxy());
	this->register_process(new f5_ea_evvyy());
	this->register_process(new f5_ea_evlxy());
	this->register_process(new f5_ae_ellvv());
	this->register_process(new f5_ea_vxyyy());
	this->register_process(new f5_ea_eeexx());
	this->register_process(new f5_ae_ellll());
	this->register_process(new f5_ae_eeell());
	this->register_process(new f5_ae_ellxx());
	this->register_process(new f5_ae_elvxy());
	this->register_process(new f5_ae_vxxxy());
	this->register_process(new f5_ae_exxxx());
	this->register_process(new f5_ea_eelvv());
	this->register_process(new f5_ea_eeeee());
	this->register_process(new f5_ae_evvvv());
	this->register_process(new f5_ea_lvvyy());
	this->register_process(new f5_ae_evvyy());
	this->register_process(new f5_ea_exxyy());
	this->register_process(new f5_ea_eevxy());
	this->register_process(new f5_ea_eeeyy());
	this->register_process(new f5_ea_ellyy());
	this->register_process(new f5_ea_vxxxy());
	this->register_process(new f5_ae_vvvxy());
	this->register_process(new f5_ea_vvvxy());
	this->register_process(new f5_ae_ellyy());
	this->register_process(new f5_ea_evvvv());
	this->register_process(new f5_ae_exxyy());
	this->register_process(new f5_ae_evlxy());
	this->register_process(new f5_ae_vxyyy());
	this->register_process(new f5_ae_lllvv());
	this->register_process(new f5_ae_eelvv());
	this->register_process(new f5_ae_lvvxx());
	this->register_process(new f5_ea_ellvv());
	this->register_process(new f5_ae_evexy());
	this->register_process(new f5_ea_ellll());
	this->register_process(new f5_ea_elevv());
	this->register_process(new f5_ea_eeell());
	this->register_process(new f5_ae_lvvvv());
	this->register_process(new f5_ea_evexy());
	this->register_process(new f5_ae_eeexx());
	this->register_process(new f5_ea_lllvv());
	this->register_process(new f5_ea_evvxx());
	this->register_process(new f5_ea_llvxy());
	this->register_process(new f5_ae_llvxy());
	this->register_process(new f5_ea_lvvvv());

	// 6f
	this->register_process(new f6_ttbar_yycyyc());
	this->register_process(new f6_ttbar_yyvlyx());
	this->register_process(new f6_ttbar_yyxylv());
	this->register_process(new f6_ttbar_yyuyyu());
	this->register_process(new f6_ttbar_yyuyyc());
	this->register_process(new f6_ttbar_yyxyev());
	this->register_process(new f6_ttbar_yyvllv());
	this->register_process(new f6_ttbar_yyvelv());
	this->register_process(new f6_ttbar_yycyyu());
	this->register_process(new f6_ttbar_yyveyx());
	this->register_process(new f6_ttbar_yyvlev());
	this->register_process(new f6_ttbar_yyveev());
	this->register_process(new f6_yyyyZ_yyyyee());
	this->register_process(new f6_yyyyZ_eeeexx());
	this->register_process(new f6_yyyyZ_eeeell());
	this->register_process(new f6_yyyyZ_eeeeyy());
	this->register_process(new f6_yyyyZ_eellyy());
	this->register_process(new f6_yyyyZ_yyyyyy());
	this->register_process(new f6_yyyyZ_llllee());
	this->register_process(new f6_yyyyZ_yyyyll());
	this->register_process(new f6_yyyyZ_yyyyvv());
	this->register_process(new f6_yyyyZ_eellxx());
	this->register_process(new f6_yyyyZ_eeeeee());
	this->register_process(new f6_vvWW_vvxyyx());
	this->register_process(new f6_vvWW_vvxylv());
	this->register_process(new f6_vvWW_vvveev());
	this->register_process(new f6_vvWW_vvveyx());
	this->register_process(new f6_vvWW_vvvlyx());
	this->register_process(new f6_vvWW_vvvllv());
	this->register_process(new f6_vvWW_vvxyev());
	this->register_process(new f6_vvWW_vvvlev());
	this->register_process(new f6_vvWW_vvvelv());
	this->register_process(new f6_eeWW_eeveev());
	this->register_process(new f6_eeWW_eexyyx());
	this->register_process(new f6_eeWW_eevlev());
	this->register_process(new f6_eeWW_eexyev());
	this->register_process(new f6_eeWW_eeveyx());
	this->register_process(new f6_eeWW_eevllv());
	this->register_process(new f6_eeWW_eevlyx());
	this->register_process(new f6_eeWW_eexylv());
	this->register_process(new f6_eeWW_eevelv());
	this->register_process(new f6_xxWW_xxveyx());
	this->register_process(new f6_xxWW_xxxyyx());
	this->register_process(new f6_xxWW_xxxylv());
	this->register_process(new f6_xxWW_xxvlyx());
	this->register_process(new f6_xxWW_xxveev());
	this->register_process(new f6_xxWW_xxvelv());
	this->register_process(new f6_xxWW_xxxyev());
	this->register_process(new f6_xxWW_xxvllv());
	this->register_process(new f6_xxWW_xxvlev());
	this->register_process(new f6_xxxxZ_xxxxee());
	this->register_process(new f6_xxxxZ_vvvvyy());
	this->register_process(new f6_xxxxZ_xxxxvv());
	this->register_process(new f6_xxxxZ_xxxxxx());
	this->register_process(new f6_xxxxZ_xxxxll());
	this->register_process(new f6_xxxxZ_vvvvxx());
	this->register_process(new f6_llWW_llxylv());
	this->register_process(new f6_llWW_llveyx());
	this->register_process(new f6_llWW_llvlev());
	this->register_process(new f6_llWW_llvelv());
	this->register_process(new f6_llWW_llvlyx());
	this->register_process(new f6_llWW_llxyev());
	this->register_process(new f6_llWW_llxyyx());
	this->register_process(new f6_llWW_llvllv());



	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void FinalStateRecorder::clear() 
{
	streamlog_out(DEBUG) << "   clear called  " << std::endl;

	m_error_code = ERROR_CODES::UNKNOWN_ERROR;
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
	m_n_run++ ;
} 

void FinalStateRecorder::processEvent( EVENT::LCEvent *pLCEvent )
{
	pLCEvent->getWeight();

	// Initialize JSON metadata file
	if (m_n_evt == 0) {
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
	
	m_n_run = pLCEvent->getRunNumber();
	m_n_evt = pLCEvent->getEventNumber();
	m_n_evt_sum++;

	// Extract process meta data
	std::string process = pLCEvent->getParameters().getStringVal("processName");

	try {
		LCCollection *inputMCParticleCollection;

		streamlog_out(DEBUG0) << "        getting jet collection: " << m_mcParticleCollection << std::endl ;
		inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		if (m_resolvers.find(process) != m_resolvers.end()) {
			FinalStateResolver* resolver = m_resolvers.at(process);

			m_process = resolver->get_process_id();
			m_n_fermion = resolver->get_n_fermions();
			m_n_higgs = resolver->get_n_higgs();

			// Get final state information
			try {
				m_final_states = resolver->m_resolve(inputMCParticleCollection);

				for (size_t i = 0; i < m_final_states.size(); i++) {
					int particle_pdg = abs(m_final_states[i]);

					if (m_final_state_counts.find(abs(particle_pdg)) != m_final_state_counts.end()) {
						m_final_state_counts[particle_pdg]++;
					} else {
						std::cerr << "Encountered unallowed final state particle " << particle_pdg << " in run " << m_n_run << " (process " << m_process << ") at event " << m_n_evt << std::endl ;
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

				m_event_category = resolver->get_event_category(m_final_state_counts);

				m_error_code = ERROR_CODES::OK;
			} catch (int err) {
				std::cerr << "Encountered exception in run " << m_n_run << " (process " << m_process << ") at event " << m_n_evt << std::endl ;
				throw err;
			}

		} else {
			m_error_code = ERROR_CODES::PROCESS_NOT_FOUND;
		}

	} catch(DataNotAvailableException &e) {
		m_error_code = ERROR_CODES::COLLECTION_NOT_FOUND;
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_n_evt << std::endl;
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
	m_jsonFile["run"] = m_n_run;
	m_jsonFile["nEvtSum"] = m_n_evt_sum;
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