#include "FinalStateRecorder.h"
#include "FinalStates.h"
#include "marlin/VerbosityLevels.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <ctime>
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
  m_beam_pol1(0.),
  m_beam_pol2(0.),
  m_cross_section(0.),
  m_cross_section_err(0.),
  m_event_weight(1.),
  m_process_id(0),
  m_passed_filter(true),
  m_n_run(0),
  m_n_evt(0),
  m_n_evt_sum(0),
  m_error_code(ERROR_CODES::UNINITIALIZED),
  m_process(0),
  m_event_category(EVENT_CATEGORY_TRUE::OTHER),
  m_event_category_zhh(EVENT_CATEGORY::NONE),
  m_n_fermion(0),
  m_n_higgs(0),
  m_pTFile(NULL)
{

	_description = "FinalStateRecorder writes meta information about the hard interaction, e.g. number of produced quarks per flavor, b/c from Higgs etc. should support all major SM, di-Higgs and single Higgs productions " ;

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);

  	registerProcessorParameter("outputRootFilename",
				"name of output root file. will use AIDA if 'None'",
				m_outputRootFile,
				std::string("")
				);

	registerProcessorParameter("writeTTree",
				"whether or not to write event meta information",
				m_write_ttree,
				true
				);

	registerProcessorParameter("outputJsonFilename",
				"name of output json file containing meta information about the process. if 'None', no JSON will be written",
				m_outputJsonFile,
				std::string("FinalStates.json")
				);
	
	registerProcessorParameter("EventFilter",
				"controls the GoodEvent return value which is true only if all conditions are fulfilled. expects a string vector of the form prop=(int)value; supports nu,nd,nc,ns,nb,nt,ne1,ne2,ne3,nv1,nv2,nv3,ngluon,ngamma,nW,nZ,nb_from_higgs,nc_from_higgs ",
				m_eventFilter,
				std::vector<std::string>{}
				);
}

std::vector<std::pair<int*, int>> FinalStateRecorder::construct_filter_lookup(std::vector<std::string> filter) {
	std::vector<std::pair<int*, int>> result;
	std::vector<std::string> operators = { "=", "<", ">", "<=", ">=" };

	for (std::string &piece : filter) {
		int operator_id = -1;
		size_t split_pos;
		for (size_t i = 0; i < operators.size(); i++) {
			split_pos = piece.find(operators[i]);
			if (split_pos != std::string::npos) {
				operator_id = (int)i;
				break;
			}
		}
		
		if (operator_id == -1) {
			streamlog_out(ERROR) << "No valid delimiter (=,<,>,>=,<=) found in filter. " << std::endl;
			throw EVENT::Exception("Cannot parse filter");
		}

		std::string property_name = piece.substr(0, split_pos);
		std::string property_value = piece.substr(split_pos+1, piece.size() - split_pos - 1);
		int property_should;
		try {
			property_should = std::stoi(property_value);
		} catch (...) {
			streamlog_out(ERROR) << "Attempting to parse: " << property_value << std::endl;
			throw EVENT::Exception("Cannot parse filter");
		}

		int* property_is;
		if (property_name == "nd") {
			property_is = &m_final_state_counts[1];
		} else if (property_name == "nu") {
			property_is = &m_final_state_counts[2];
		} else if (property_name == "ns") {
			property_is = &m_final_state_counts[3];
		} else if (property_name == "nc") {
			property_is = &m_final_state_counts[4];
		} else if (property_name == "nb") {
			property_is = &m_final_state_counts[5];
		} else if (property_name == "nt") {
			property_is = &m_final_state_counts[6];
		} else if (property_name == "ne1") {
			property_is = &m_final_state_counts[11];
		} else if (property_name == "ne2") {
			property_is = &m_final_state_counts[13];
		} else if (property_name == "ne3") {
			property_is = &m_final_state_counts[15];
		} else if (property_name == "nv1") {
			property_is = &m_final_state_counts[12];
		} else if (property_name == "nv2") {
			property_is = &m_final_state_counts[14];
		} else if (property_name == "nv3") {
			property_is = &m_final_state_counts[16];
		} else if (property_name == "ngluon") {
			property_is = &m_final_state_counts[21];
		} else if (property_name == "ngamma") {
			property_is = &m_final_state_counts[22];
		} else if (property_name == "nW") {
			property_is = &m_final_state_counts[24];
		} else if (property_name == "nZ") {
			property_is = &m_final_state_counts[23];
		} else if (property_name == "nb_from_higgs") {
			property_is = &m_n_b_from_higgs;
		} else if (property_name == "nc_from_higgs") {
			property_is = &m_n_c_from_higgs;
		} else {
			streamlog_out(ERROR) << "Cannot parse property '" << property_name << "' = " << piece << std::endl;
			throw EVENT::Exception("Unkown property");
		}
		streamlog_out(MESSAGE) << "Added check for " << property_name << " = " << property_should << std::endl ;

		m_filter_quantities.push_back(property_name);
		m_filter_operators.push_back(operator_id);
		m_filter_operator_names.push_back(operators[operator_id]);

		result.push_back(std::make_pair(
			property_is,
			property_should
		));		
	}

	return result;
};

bool FinalStateRecorder::process_filter(){
	for (size_t i = 0; i < m_filter_lookup.size(); i++) {
		auto& is_should_pair = m_filter_lookup[i];
		bool check;

		switch (m_filter_operators[i]) {
			case 0: check = *is_should_pair.first == is_should_pair.second; break;
			case 1: check = *is_should_pair.first  < is_should_pair.second; break;
			case 2: check = *is_should_pair.first  > is_should_pair.second; break;
			case 3: check = *is_should_pair.first <= is_should_pair.second; break;
			case 4: check = *is_should_pair.first >= is_should_pair.second; break;
			default:
				// do nothing
				break;
		}
		
		if (!check) {
			streamlog_out(MESSAGE) << "Event did not pass filter for quantity " << m_filter_quantities[i] << ": " << *is_should_pair.first << m_filter_operator_names[i] << is_should_pair.second << std::endl ;
			return false;
		}
	}

	return true;
}

void FinalStateRecorder::init()
{
	printParameters();
	
	this->clear();

	m_setReturnValues = m_eventFilter.size() > 0;

	if (m_write_ttree) {
		if (m_outputRootFile.size()) {
			m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
			m_pTTree->SetDirectory(m_pTFile);
		}

		m_pTTree->Branch("run", &m_n_run, "run/I");
		m_pTTree->Branch("event", &m_n_evt, "event/I");

		m_pTTree->Branch("error_code", &m_error_code);
		m_pTTree->Branch("final_states", &m_final_states);
		m_pTTree->Branch("final_state_counts", &m_final_state_counts);

		m_pTTree->Branch("process", &m_process);
		m_pTTree->Branch("process_id", &m_process_id);
		m_pTTree->Branch("polarization_code", &m_polarization_code);
		m_pTTree->Branch("cross_section", &m_cross_section);
		m_pTTree->Branch("event_category", &m_event_category);
		m_pTTree->Branch("event_category_zhh", &m_event_category_zhh);
		m_pTTree->Branch("n_fermion", &m_n_fermion);
		m_pTTree->Branch("n_higgs", &m_n_higgs);
		m_pTTree->Branch("n_b_from_higgs", &m_n_b_from_higgs);
		m_pTTree->Branch("n_c_from_higgs", &m_n_c_from_higgs);

		if (m_setReturnValues) {
			m_pTTree->Branch("passed", &m_passed_filter);
		}
	}

	m_t_start = std::time(nullptr);
	
	if (m_setReturnValues) {
		streamlog_out(MESSAGE) << "Constructing filter...";
		m_filter_lookup = construct_filter_lookup(m_eventFilter);
	}

	// Register physics processes / final state resolvers
	// hh2f
	this->register_process(new e1e1hh());
	this->register_process(new e2e2hh());
	this->register_process(new e3e3hh());
	this->register_process(new qqhh());
	this->register_process(new n1n1hh());
	this->register_process(new n23n23hh());

	// h4f
	this->register_process(new e1e1qqh());
	this->register_process(new e2e2qqh());
	this->register_process(new e3e3qqh());
	this->register_process(new qqqqh());
	this->register_process(new n1n1qqh());
	this->register_process(new n23n23qqh());

	// h2f
	this->register_process(new e1e1h());
	this->register_process(new e2e2h());
	this->register_process(new e3e3h());
	this->register_process(new qqh());
	this->register_process(new n1n1h());
	this->register_process(new n23n23h());

	// 2f
	this->register_process(new ll());
	this->register_process(new qq());
	this->register_process(new vv());
	this->register_process(new ee1());
	this->register_process(new ee2());

	// 4f
	this->register_process(new llll_zz());
	this->register_process(new llll_szee_lmee());
	this->register_process(new llll_szsw_lmee());
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
	this->register_process(new p5_ae_eeevv());
	this->register_process(new p5_ea_eyyyy());
	this->register_process(new p5_ae_elevv());
	this->register_process(new p5_ae_eyyyy());
	this->register_process(new p5_ea_exxxx());
	this->register_process(new p5_ae_evvxx());
	this->register_process(new p5_ae_eeeyy());
	this->register_process(new p5_ae_eevxy());
	this->register_process(new p5_ae_lvvyy());
	this->register_process(new p5_ea_eeevv());
	this->register_process(new p5_ea_ellxx());
	this->register_process(new p5_ae_eeeee());
	this->register_process(new p5_ea_elvxy());
	this->register_process(new p5_ea_evvyy());
	this->register_process(new p5_ea_evlxy());
	this->register_process(new p5_ae_ellvv());
	this->register_process(new p5_ea_vxyyy());
	this->register_process(new p5_ea_eeexx());
	this->register_process(new p5_ae_ellll());
	this->register_process(new p5_ae_eeell());
	this->register_process(new p5_ae_ellxx());
	this->register_process(new p5_ae_elvxy());
	this->register_process(new p5_ae_vxxxy());
	this->register_process(new p5_ae_exxxx());
	this->register_process(new p5_ea_eelvv());
	this->register_process(new p5_ea_eeeee());
	this->register_process(new p5_ae_evvvv());
	this->register_process(new p5_ea_lvvyy());
	this->register_process(new p5_ae_evvyy());
	this->register_process(new p5_ea_exxyy());
	this->register_process(new p5_ea_eevxy());
	this->register_process(new p5_ea_eeeyy());
	this->register_process(new p5_ea_ellyy());
	this->register_process(new p5_ea_vxxxy());
	this->register_process(new p5_ae_vvvxy());
	this->register_process(new p5_ea_vvvxy());
	this->register_process(new p5_ae_ellyy());
	this->register_process(new p5_ea_evvvv());
	this->register_process(new p5_ae_exxyy());
	this->register_process(new p5_ae_evlxy());
	this->register_process(new p5_ae_vxyyy());
	this->register_process(new p5_ae_lllvv());
	this->register_process(new p5_ae_eelvv());
	this->register_process(new p5_ae_lvvxx());
	this->register_process(new p5_ea_ellvv());
	this->register_process(new p5_ae_evexy());
	this->register_process(new p5_ea_ellll());
	this->register_process(new p5_ea_elevv());
	this->register_process(new p5_ea_eeell());
	this->register_process(new p5_ae_lvvvv());
	this->register_process(new p5_ea_evexy());
	this->register_process(new p5_ae_eeexx());
	this->register_process(new p5_ea_lllvv());
	this->register_process(new p5_ea_evvxx());
	this->register_process(new p5_ea_llvxy());
	this->register_process(new p5_ae_llvxy());
	this->register_process(new p5_ea_lvvvv());

	// 6f
	this->register_process(new p6_yycyyc());
	this->register_process(new p6_yyvlyx());
	this->register_process(new p6_yyxylv());
	this->register_process(new p6_yyuyyu());
	this->register_process(new p6_yyuyyc());
	this->register_process(new p6_yyxyev());
	this->register_process(new p6_yyvllv());
	this->register_process(new p6_yyvelv());
	this->register_process(new p6_yycyyu());
	this->register_process(new p6_yyveyx());
	this->register_process(new p6_yyvlev());
	this->register_process(new p6_yyveev());
	this->register_process(new p6_yyyyZ_yyyyee());
	this->register_process(new p6_yyyyZ_eeeexx());
	this->register_process(new p6_yyyyZ_eeeell());
	this->register_process(new p6_yyyyZ_eeeeyy());
	this->register_process(new p6_yyyyZ_eellyy());
	this->register_process(new p6_yyyyZ_yyyyyy());
	this->register_process(new p6_yyyyZ_llllee());
	this->register_process(new p6_yyyyZ_yyyyll());
	this->register_process(new p6_yyyyZ_yyyyvv());
	this->register_process(new p6_yyyyZ_eellxx());
	this->register_process(new p6_yyyyZ_eeeeee());
	this->register_process(new p6_vvWW_vvxyyx());
	this->register_process(new p6_vvWW_vvxylv());
	this->register_process(new p6_vvWW_vvveev());
	this->register_process(new p6_vvWW_vvveyx());
	this->register_process(new p6_vvWW_vvvlyx());
	this->register_process(new p6_vvWW_vvvllv());
	this->register_process(new p6_vvWW_vvxyev());
	this->register_process(new p6_vvWW_vvvlev());
	this->register_process(new p6_vvWW_vvvelv());
	this->register_process(new p6_eeWW_eeveev());
	this->register_process(new p6_eeWW_eexyyx());
	this->register_process(new p6_eeWW_eevlev());
	this->register_process(new p6_eeWW_eexyev());
	this->register_process(new p6_eeWW_eeveyx());
	this->register_process(new p6_eeWW_eevllv());
	this->register_process(new p6_eeWW_eevlyx());
	this->register_process(new p6_eeWW_eexylv());
	this->register_process(new p6_eeWW_eevelv());
	this->register_process(new p6_xxWW_xxveyx());
	this->register_process(new p6_xxWW_xxxyyx());
	this->register_process(new p6_xxWW_xxxylv());
	this->register_process(new p6_xxWW_xxvlyx());
	this->register_process(new p6_xxWW_xxveev());
	this->register_process(new p6_xxWW_xxvelv());
	this->register_process(new p6_xxWW_xxxyev());
	this->register_process(new p6_xxWW_xxvllv());
	this->register_process(new p6_xxWW_xxvlev());
	this->register_process(new p6_xxxxZ_xxxxee());
	this->register_process(new p6_xxxxZ_vvvvyy());
	this->register_process(new p6_xxxxZ_xxxxvv());
	this->register_process(new p6_xxxxZ_xxxxxx());
	this->register_process(new p6_xxxxZ_xxxxll());
	this->register_process(new p6_xxxxZ_vvvvxx());
	this->register_process(new p6_llWW_llxylv());
	this->register_process(new p6_llWW_llveyx());
	this->register_process(new p6_llWW_llvlev());
	this->register_process(new p6_llWW_llvelv());
	this->register_process(new p6_llWW_llvlyx());
	this->register_process(new p6_llWW_llxyev());
	this->register_process(new p6_llWW_llxyyx());
	this->register_process(new p6_llWW_llvllv());

	// inclusive 6f mc-2025 production
	this->register_process(new p6_inclusive_eeeexx());
	this->register_process(new p6_inclusive_eeeeyy());
	this->register_process(new p6_inclusive_eeevxy());
	this->register_process(new p6_inclusive_eelvxy());
	this->register_process(new p6_inclusive_eeveyx());
	this->register_process(new p6_inclusive_eevlyx());
	this->register_process(new p6_inclusive_eexxxx());
	this->register_process(new p6_inclusive_eexyyx());
	this->register_process(new p6_inclusive_eeyyyy());
	this->register_process(new p6_inclusive_lleexx());
	this->register_process(new p6_inclusive_lleeyy());
	this->register_process(new p6_inclusive_llevxy());
	this->register_process(new p6_inclusive_llllll());
	this->register_process(new p6_inclusive_llllxx());
	this->register_process(new p6_inclusive_llllyy());
	this->register_process(new p6_inclusive_llveyx());
	this->register_process(new p6_inclusive_llvllv());
	this->register_process(new p6_inclusive_llxxxx());
	this->register_process(new p6_inclusive_llxyyx());
	this->register_process(new p6_inclusive_llyyyy());
	this->register_process(new p6_inclusive_veevxx());
	this->register_process(new p6_inclusive_veevyy());
	this->register_process(new p6_inclusive_velvxx());
	this->register_process(new p6_inclusive_velvyy());
	this->register_process(new p6_inclusive_vlevxx());
	this->register_process(new p6_inclusive_vlevyy());
	this->register_process(new p6_inclusive_vllvxx());
	this->register_process(new p6_inclusive_vllvyy());
	this->register_process(new p6_inclusive_vvevxy());
	this->register_process(new p6_inclusive_vvlvxy());
	this->register_process(new p6_inclusive_vvveyx());
	this->register_process(new p6_inclusive_vvvllv());
	this->register_process(new p6_inclusive_vvvlyx());
	this->register_process(new p6_inclusive_vvvvvv());
	this->register_process(new p6_inclusive_vvvvxx());
	this->register_process(new p6_inclusive_vvvvyy());
	this->register_process(new p6_inclusive_vvxxxx());
	this->register_process(new p6_inclusive_vvxyyx());
	this->register_process(new p6_inclusive_vvyyyy());
	this->register_process(new p6_inclusive_xxveyx());
	this->register_process(new p6_inclusive_xxvlyx());
	this->register_process(new p6_inclusive_xxxyev());
	this->register_process(new p6_inclusive_xxxylv());
	this->register_process(new p6_inclusive_yyveyx());
	this->register_process(new p6_inclusive_yyvlyx());
	this->register_process(new p6_inclusive_yyxyev());
	this->register_process(new p6_inclusive_yyxylv());

	this->register_process(new p6_ftag_uuuuuu());
	this->register_process(new p6_ftag_dddddd());
	this->register_process(new p6_ftag_ssssss());
	this->register_process(new p6_ftag_cccccc());
	this->register_process(new p6_ftag_bbbbbb());



	streamlog_out(MESSAGE) << "   init finished  " << std::endl;
}

void FinalStateRecorder::clear() 
{
	m_error_code = ERROR_CODES::UNKNOWN_ERROR;
	//m_final_states.clear();

	for (auto const& [key, value] : m_final_state_counts)
		m_final_state_counts[key] = 0;

	m_process = 0;
	m_n_fermion = 0;
	m_n_higgs = 0;
	m_n_b_from_higgs = 0;
	m_n_c_from_higgs = 0;

	m_event_category = EVENT_CATEGORY_TRUE::OTHER;
	m_event_category_zhh = EVENT_CATEGORY::NONE;
}
void FinalStateRecorder::processRunHeader( LCRunHeader*  /*run*/) { 
	m_n_run++ ;
} 

void FinalStateRecorder::processEvent( EVENT::LCEvent *pLCEvent )
{
	// Initialize JSON metadata file
	if (m_n_evt == 0) {
		if (m_mcParticleCollection == "MCParticlesSkimmed") {
			m_beam_pol1 = pLCEvent->getParameters().getFloatVal("beamPol1");
			m_beam_pol2 = pLCEvent->getParameters().getFloatVal("beamPol2");
		} else {
			m_beam_pol1 = pLCEvent->getParameters().getFloatVal("Pol0");
			m_beam_pol2 = pLCEvent->getParameters().getFloatVal("Pol1");
		}

		m_cross_section = pLCEvent->getParameters().getFloatVal("crossSection");
		m_cross_section_err = pLCEvent->getParameters().getFloatVal("crossSectionError");
		m_event_weight = pLCEvent->getWeight();
		m_process_id = pLCEvent->getParameters().getIntVal("ProcessID");
		m_process_name = pLCEvent->getParameters().getStringVal("processName");

		if (m_beam_pol1 == -1 && m_beam_pol2 == -1) m_polarization_code = POLARIZATION_CODES::EL_PL; else
		if (m_beam_pol1 == -1 && m_beam_pol2 ==  1) m_polarization_code = POLARIZATION_CODES::EL_PR; else
		if (m_beam_pol1 ==  1 && m_beam_pol2 == -1) m_polarization_code = POLARIZATION_CODES::ER_PL; else
		if (m_beam_pol1 ==  1 && m_beam_pol2 ==  1) m_polarization_code = POLARIZATION_CODES::ER_PR;
	}

	this->clear();
	
	streamlog_out(MESSAGE)  << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << std::endl;
	
	m_n_run = pLCEvent->getRunNumber();
	m_n_evt = pLCEvent->getEventNumber();
	m_n_evt_sum++;

	// Extract process meta data
	std::string process = pLCEvent->getParameters().getStringVal("processName");

	try {
		LCCollection *inputMCParticleCollection;

		inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		if (resolvers.find(process) != resolvers.end()) {
			FinalStateResolver* resolver = resolvers.at(process);

			m_process = resolver->get_process_id();
			m_n_fermion = resolver->get_n_fermions();
			m_n_higgs = resolver->get_n_higgs();

			// Get final state information
			try {
				m_final_states = resolver->resolve(inputMCParticleCollection);
				m_n_b_from_higgs = resolver->get_n_b_from_higgs();
				m_n_c_from_higgs = resolver->get_n_c_from_higgs();

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
					m_event_category_zhh = EVENT_CATEGORY::LEPTONIC;
				} else if (m_process == PROCESS_ID::n1n1hh || m_process == PROCESS_ID::n23n23hh) {
					m_event_category_zhh = EVENT_CATEGORY::NEUTRINO;
				} else if (m_process == PROCESS_ID::qqhh) {
					m_event_category_zhh = EVENT_CATEGORY::HADRONIC;
				}

				m_event_category = resolver->get_event_category(m_final_state_counts);
				m_error_code = ERROR_CODES::OK;

				if (m_setReturnValues) {
					m_passed_filter = process_filter();
					streamlog_out(MESSAGE) << "Passed event " << m_n_evt << ": " << (m_passed_filter ? "YES" : "NO") << std::endl;
				}
				setReturnValue("GoodEvent", m_passed_filter);
			} catch (int err) {
				std::cerr << "Encountered exception (error " << err << ") in run " << m_n_run << " (process " << m_process << ") at event " << m_n_evt << std::endl ;
				setReturnValue("GoodEvent", false);
				throw err;
			}

		} else {
			throw EVENT::Exception("Critical error: process not registered: " + process);
			//m_error_code = ERROR_CODES::PROCESS_NOT_FOUND;
			//streamlog_out(MESSAGE) << "processEvent : process not registered: " << process << std::endl;
			//setReturnValue("GoodEvent", false);
		}

	} catch(DataNotAvailableException &e) {
		m_error_code = ERROR_CODES::COLLECTION_NOT_FOUND;
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_n_evt << std::endl;
		setReturnValue("GoodEvent", false);
	}

	if (m_write_ttree) {
		m_pTTree->Fill();
	}
}

void FinalStateRecorder::check()
{
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}


void FinalStateRecorder::end()
{
	// Write ROOT file
	if (m_write_ttree) {
		if (m_pTFile != NULL) {
			m_pTFile->cd();
		}

		m_pTTree->Write();

		if (m_pTFile != NULL) {
			m_pTFile->Close();
			delete m_pTFile;
		}
	}

	// Write JSON metadata file
	if (m_outputJsonFile != "None") {
		m_jsonFile["run"] = m_n_run;
		m_jsonFile["nEvtSum"] = m_n_evt_sum;
		m_jsonFile["polElectron"] = m_beam_pol1;
		m_jsonFile["polPositron"] = m_beam_pol2;
		m_jsonFile["polCode"] = m_polarization_code;
		m_jsonFile["crossSection"] = m_cross_section;
		m_jsonFile["crossSectionError"] = m_cross_section_err;
		m_jsonFile["eventWeight"] = m_event_weight;
		m_jsonFile["processId"] = m_process_id;
		m_jsonFile["processName"] = m_process_name;
		m_jsonFile["tStart"] = m_t_start;
		m_jsonFile["tEnd"] = std::time(nullptr);
	
		std::ofstream file(m_outputJsonFile);
		file << m_jsonFile;
	}

	// delete all registered FinalStateRecorders
	for (auto [key, value]: resolvers) {
		delete value;
	};
	resolvers.clear();
}