#include "EventObservablesBase.h"
#include <iostream>
#include <fstream>
#include <numeric>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCIntVec.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>
#include "inv_mass.h"

using namespace lcio ;
using namespace marlin ;
using namespace std ;

TLorentzVector v4(ReconstructedParticle* p){
	return TLorentzVector( p->getMomentum()[0], p->getMomentum()[1], p->getMomentum()[2], p->getEnergy() );
}
TLorentzVector v4(LCObject* lcobj){
	ReconstructedParticle* p = dynamic_cast<ReconstructedParticle*>(lcobj); return v4(p);
}

EventObservablesBase::EventObservablesBase(const std::string &name) : Processor(name),
  m_write_ttree(true),
  m_pTFile(NULL),
  m_nRun(0),
  m_nEvt(0),
  m_errorCode(0)
{
	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
            "isolatedleptonCollection" ,
            "Name of the Isolated Lepton collection"  ,
            m_inputIsolatedleptonCollection ,
            std::string("ISOLeptons")
            );

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
            "LepPairCollection",
            "Name of input lepton pair collection",
            m_inputLepPairCollection,
            std::string("LeptonPair")
            );

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
            "JetCollectionName" ,
            "Name of the Jet collection"  ,
            m_inputJetCollection ,
            std::string("Durham4Jets")
            );

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
            "inputPfoCollection",
            "Name of input pfo collection",
            m_inputPfoCollection,
            std::string("PandoraPFOs")
            );

	registerProcessorParameter("whichPreselection",
            "Which set of cuts to use in the preselection. This will overwrite any input preselection values.",
            m_whichPreselection,
            std::string("llbbbb")
            );

	registerProcessorParameter("cutDefinitionsJSONFile",
            "A JSON file containing cut definitions. See cuts.json in the repository for an example. If given, this will overwrite any input preselection as well as any predefined (hard-coded) preselection values.",
            m_cutDefinitionsJSONFile,
            std::string("")
            );

	registerProcessorParameter("JetTaggingPIDAlgorithm",
            "Number of jet should be in the event",
            m_JetTaggingPIDAlgorithm,
            std::string("lcfiplus")
            );

	registerProcessorParameter("JetTaggingPIDParameterB",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterB,
            std::string("BTag")
            );

	registerProcessorParameter("JetTaggingPIDParameterC",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterC,
            std::string("CTag")
            );

	registerProcessorParameter("JetTaggingPIDAlgorithm2",
            "Number of jet should be in the event",
            m_JetTaggingPIDAlgorithm2,
            std::string("weaver")
            );

	registerProcessorParameter("JetTaggingPIDParameterB2",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterB2,
            std::string("mc_b")
            );

	registerProcessorParameter("JetTaggingPIDParameterC2",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterC2,
            std::string("mc_c")
            );
  	
	registerProcessorParameter("maxdileptonmassdiff",
			"maximum on dilepton mass difference",
			m_maxdileptonmassdiff,
			float(999.)
			);

	registerProcessorParameter("maxdijetmassdiff",
			"maximum on dijet mass difference (m_jj-125 GeV)",
			m_maxdijetmassdiff,
			float(999.)
			);

	registerProcessorParameter("mindijetmass",
			"minimum on dijet mass",
			m_mindijetmass,
			float(0.)
			);

	registerProcessorParameter("maxdijetmass",
			"maximum on dijet mass",
			m_maxdijetmass,
			float(999.)
			);

	registerProcessorParameter("minmissingPT",
			"minimum on missing PT",
			m_minmissingPT,
			float(0.)
			);

	registerProcessorParameter("maxmissingPT",
			"maximum on missing PT",
			m_maxmissingPT,
			float(999.)
			);

	registerProcessorParameter("maxthrust",
			"maximum on thrust",
			m_maxthrust,
			float(999.)
			);

	registerProcessorParameter("minblikeliness",
			"minimum on blikeliness",
			m_minblikeliness,
			float(0.)
			);

	registerProcessorParameter("minnbjets",
			"minimum number of bjets that fulfill blikeliness criteria",
			m_minnbjets,
			int(0)
			);

	registerProcessorParameter("maxEvis",
			"maximum on visible energy",
			m_maxEvis,
			float(999.)
			);

	registerProcessorParameter("minHHmass",
			"minimum on higgs pairs mass",
			m_minHHmass,
			float(0.)
			);
	
	registerProcessorParameter("ECM" ,
			"Center-of-Mass Energy in GeV",
			m_ECM,
			float(500.f)
			);

	registerProcessorParameter("polarizations",
            "vector of (Pe-, Pe+). used for averaging over zhh and zzh matrix elements.",
            m_polarizations,
            std::vector<float>{ -0.8, 0.3 }
            );

  	registerProcessorParameter("outputFilename",
			"name of output root file",
			m_outputFile,
			std::string("")
			);
};

void EventObservablesBase::prepareBaseTree()
{
	TTree* ttree = getTTree();

	if (m_outputFile.size()) {
		m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
		ttree->SetDirectory(m_pTFile);
	}

	if (m_write_ttree) {
		ttree->Branch("run", &m_nRun, "run/I");
		ttree->Branch("event", &m_nEvt, "event/I");

		// evis:mm:mh1:mh2:mhh:pz:ph1:ph2:cosz:cosh1:cosh2:yminus:yplus
		ttree->Branch("evis", &m_Evis, "evis/F");
		ttree->Branch("m_miss", &m_missingMass, "m_miss/F");
		ttree->Branch("mh1", &m_mh1, "mh1/F");
		ttree->Branch("mh2", &m_mh2, "mh2/F");
		ttree->Branch("mhh", &m_mhh, "mhh/F");

		ttree->Branch("pz", &m_pz, "pz/F");
		ttree->Branch("ph1", &m_ph1, "ph1/F");
		ttree->Branch("ph2", &m_ph2, "ph2/F");
		ttree->Branch("cosz", &m_cosz, "cosz/F");
		ttree->Branch("cosh1", &m_cosh1, "cosh1/F");
		ttree->Branch("cosh2", &m_cosh2, "cosh2/F");
		ttree->Branch("yminus", &m_todo, "yminus/F");
		ttree->Branch("yplus", &m_yplus, "yplus/F");

		// nhbb:njet:chi2:mpt:prob11:prob12:prob21:prob22
		ttree->Branch("n_jets",&m_nJets,"n_jets/I");
		ttree->Branch("n_iso_leptons",&m_nIsoLeps,"n_iso_leptons/I");
		ttree->Branch("lep_types", &m_lep_types);
		ttree->Branch("pt_miss", &m_missingPT, "pt_miss/F");
		ttree->Branch("e_miss", &m_missingE, "e_miss/F");
		ttree->Branch("thrust", &m_thrust, "thrust/F");
		ttree->Branch("bTags", &m_bTagValues);
		ttree->Branch("cTags", &m_cTagValues);

		// bmax1:bmax2:bmax3:bmax4:pj1jets2:pj2jets2
		ttree->Branch("bmax1", &m_bmax1, "bmax1/F");
		ttree->Branch("bmax2", &m_bmax2, "bmax2/F");
		ttree->Branch("bmax3", &m_bmax3, "bmax3/F");
		ttree->Branch("bmax4", &m_bmax4, "bmax4/F");

		ttree->Branch("cmax1", &m_cmax1, "cmax1/F");
		ttree->Branch("cmax2", &m_cmax2, "cmax2/F");
		ttree->Branch("cmax3", &m_cmax3, "cmax3/F");
		ttree->Branch("cmax4", &m_cmax4, "cmax4/F");

		if (m_use_tags2) {
			ttree->Branch("bTags2", &m_bTagValues2);
			ttree->Branch("cTags2", &m_cTagValues2);

			ttree->Branch("bmax12", &m_bmax12, "bmax12/F");
			ttree->Branch("bmax22", &m_bmax22, "bmax22/F");
			ttree->Branch("bmax32", &m_bmax32, "bmax32/F");
			ttree->Branch("bmax42", &m_bmax42, "bmax42/F");

			ttree->Branch("cmax12", &m_cmax12, "cmax12/F");
			ttree->Branch("cmax22", &m_cmax22, "cmax22/F");
			ttree->Branch("cmax32", &m_cmax32, "cmax32/F");
			ttree->Branch("cmax42", &m_cmax42, "cmax42/F");
		}

		// pxij:pyij:pzij:eij for all dijets i=(1,2) and associated jets (1,2)
		// that all hypotheses have in common
		ttree->Branch("px11", &m_px11, "px11/F");
		ttree->Branch("py11", &m_py11, "py11/F");
		ttree->Branch("pz11", &m_pz11, "pz11/F");
		ttree->Branch("e11", &m_e11, "e11/F");

		ttree->Branch("px12", &m_px12, "px12/F");
		ttree->Branch("py12", &m_py12, "py12/F");
		ttree->Branch("pz12", &m_pz12, "pz12/F");
		ttree->Branch("e12", &m_e12, "e12/F");

		ttree->Branch("px21", &m_px21, "px21/F");
		ttree->Branch("py21", &m_py21, "py21/F");
		ttree->Branch("pz21", &m_pz21, "pz21/F");
		ttree->Branch("e21", &m_e21, "e21/F");

		ttree->Branch("px22", &m_px22, "px22/F");
		ttree->Branch("py22", &m_py22, "py22/F");
		ttree->Branch("pz22", &m_pz22, "pz22/F");
		ttree->Branch("e22", &m_e22, "e22/F");

		// jet matching
		ttree->Branch("jet_matching", &m_jet_matching);
		ttree->Branch("jet_matching_source", &m_jet_matching_source, "jet_matching_source/I");
		ttree->Branch("using_kinfit", &m_using_kinfit, "using_kinfit/B");
		ttree->Branch("using_mass_chi2", &m_using_mass_chi2, "using_mass_chi2/B");

		// matrix elements
		if (m_use_matrix_elements()) {
			ttree->Branch("me_zhh_log", &m_lcme_zhh_log, "me_zhh_log/D");
			ttree->Branch("me_zzh_log", &m_lcme_zzh_log, "me_zzh_log/D");
		}

		/* old variables for pre selection
		ttree->Branch("dileptonMassPrePairing", &m_dileptonMassPrePairing, "dileptonMassPrePairing/F");
		ttree->Branch("dileptonMass", &m_dileptonMass, "dileptonMass/F");
		ttree->Branch("dileptonMassDiff", &m_dileptonMassDiff, "dileptonMassDiff/F");
		ttree->Branch("dijetChi2min", &m_chi2min, "dijetChi2min/F");
		ttree->Branch("dijetPairing", &m_dijetPairing);
		ttree->Branch("dijetMass", &m_dijetMass);
		ttree->Branch("dijetMassDiff", &m_dijetMassDiff);
		ttree->Branch("dihiggsMass", &m_dihiggsMass, "dihiggsMass/F");
		ttree->Branch("nbjets", &m_nbjets, "nbjets/I");
		ttree->Branch("preselsPassedVec", &m_preselsPassedVec);
		ttree->Branch("preselsPassedAll", &m_preselsPassedAll);
		ttree->Branch("preselsPassedConsec", &m_preselsPassedConsec);
		ttree->Branch("preselPassed", &m_isPassed);
		*/
	}

	streamlog_out(DEBUG) << "   init finished  " << std::endl;

	// m_nAskedJets() must be implemented by the processor

	if (m_cutDefinitionsJSONFile.length() > 0) {
		streamlog_out(DEBUG) << "Reading preselection cuts from file " << m_cutDefinitionsJSONFile << std::endl;

		std::ifstream ifs(m_cutDefinitionsJSONFile);
		jsonf cuts = jsonf::parse(ifs);
		
		std::string preselection_key = m_whichPreselection.substr(0, 2);

		m_maxdileptonmassdiff = cuts[preselection_key]["maxDileptonMassDiff"];
		m_maxdijetmassdiff = cuts[preselection_key]["maxDijetMassDiff"];
		m_mindijetmass = cuts[preselection_key]["dijetMass"][0];
		m_maxdijetmass = cuts[preselection_key]["dijetMass"][1];
		m_minmissingPT = cuts[preselection_key]["missingPT"][0];
		m_maxmissingPT = cuts[preselection_key]["missingPT"][1];
		m_maxthrust = cuts[preselection_key]["maxThrust"];
		m_minblikeliness = cuts[preselection_key]["minBLikeliness"];
		m_minnbjets = cuts[preselection_key]["minNBJets"];
		m_maxEvis = cuts[preselection_key]["maxEvis"];
		m_minHHmass = cuts[preselection_key]["minHHmass"];
	} else {
		if (m_whichPreselection == "llbbbb") {
			m_maxdileptonmassdiff = 40.;
			m_maxdijetmassdiff = 80.;
			m_mindijetmass = 60.;
			m_maxdijetmass = 180.;
			m_minmissingPT = 0.;
			m_maxmissingPT = 70.;
			m_maxthrust = 0.9;
			m_minblikeliness = 0.; 
			m_minnbjets = 0;
			m_maxEvis = 999.;
			m_minHHmass = 0.;
		} else if (m_whichPreselection == "vvbbbb") {
			m_maxdileptonmassdiff = 999.;
			m_maxdijetmassdiff = 80.;
			m_mindijetmass = 60.;
			m_maxdijetmass = 180.;
			m_minmissingPT = 10.;
			m_maxmissingPT = 180.;
			m_maxthrust = 0.9;
			m_minblikeliness = 0.2;
			m_minnbjets = 3;
			m_maxEvis= 400.;
			m_minHHmass = 220.;
		} else if (m_whichPreselection == "qqbbbb") {
			m_maxdileptonmassdiff = 999.;
			m_maxdijetmassdiff = 999.;
			m_mindijetmass = 60.;
			m_maxdijetmass = 180.;
			m_minmissingPT = 0.;
			m_maxmissingPT = 70.;
			m_maxthrust = 0.9;
			m_minblikeliness = 0.16;
			m_minnbjets = 4;
			m_maxEvis = 999.;
			m_minHHmass = 0.;
		}
	}
}

void EventObservablesBase::clearBaseValues() 
{
	streamlog_out(DEBUG) << "   Clear called  " << std::endl;

	m_errorCode = 0;

	m_nJets = 0;
	m_nIsoLeps = 0;
	m_lep_types.clear();
	m_missingPT = -999.;
	m_missingMass = -999.;
	m_missingE = -999.;
	m_Evis  = -999.;
	m_thrust = -999.;
	std::fill(m_bTagValues.begin(), m_bTagValues.end(), -1.);
	std::fill(m_cTagValues.begin(), m_cTagValues.end(), -1.);

	std::fill(m_bTagValues2.begin(), m_bTagValues2.end(), -1.);
	std::fill(m_cTagValues2.begin(), m_cTagValues2.end(), -1.);

	m_jet_matching.clear();
	m_jet_matching_source = 0;
	m_using_kinfit = false;
	m_using_mass_chi2 = false;
	
	/*
	m_dileptonMassPrePairing = -999.;
	m_dileptonMass = -999.;
	m_dileptonMassDiff = -999.;
	m_dijetMass.clear();
	m_dijetMassDiff.clear();
	m_dihiggsMass = -999;
	m_nbjets = 0;
	m_dijetPairing.clear();
	m_chi2min = 99999.;
	m_preselsPassedVec.clear();
	m_preselsPassedAll = 0;
	m_preselsPassedConsec = 0;
	m_isPassed = 0;
	*/
}

void EventObservablesBase::updateBaseValues(EVENT::LCEvent *pLCEvent) {
	this->clearBaseValues();

	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();

	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

	try {
		LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
		LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
		LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
		LCCollection *inputPfoCollection = pLCEvent->getCollection( m_inputPfoCollection );

		m_nJets = inputJetCollection->getNumberOfElements();
		assert(m_nJets == m_nAskedJets());

		m_nIsoLeps = inputLeptonCollection->getNumberOfElements();
		int nPFOs = inputPfoCollection->getNumberOfElements();    
		// ---------- MISSING PT ----------
		// correct for crossing angle
		float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
		double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);
		TLorentzVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
		TLorentzVector pfosum(0.,0.,0.,0.);
		for (int i=0; i<nPFOs; i++) {
		ReconstructedParticle* pfo = (ReconstructedParticle*) inputPfoCollection->getElementAt(i);
		pfosum+= TLorentzVector( pfo->getMomentum() , pfo->getEnergy() );
		}
		TLorentzVector pmis = ecms - pfosum;
		m_missingPT = pmis.Pt();
		m_missingMass = pmis.M();
		m_missingE = pmis.E();
		
		// ---------- VISIBLE ENERGY ----------                                          
		m_Evis = pfosum.E();

		// ---------- THRUST ----------
		const EVENT::LCParameters& pfo_params = inputPfoCollection->getParameters();
		m_thrust = pfo_params.getFloatVal("principleThrustValue");

		// ---------- SAVE TYPES OF ALL ISOLATED LEPTONS ----------
		for (int j = 0; j < inputLeptonCollection->getNumberOfElements(); j++) {
			ReconstructedParticle* iso_lepton = dynamic_cast<ReconstructedParticle*>( inputLeptonCollection->getElementAt( j ) );
			m_lep_types.push_back( iso_lepton->getType() );
		}

		// continue only if the number of jets and isolated leptons match the preselection
		// and the numbers in the Kinfit processors
		if ( m_nJets == m_nAskedJets() && inputLepPairCollection->getNumberOfElements() == m_nAskedIsoLeps() ) {
			streamlog_out(MESSAGE) << "Continue with "<< m_nJets << " jets and "<< inputLepPairCollection->getNumberOfElements() << " ISOLeptons" << std::endl;

			// ---------- JET MATCHING ----------
			(void) inputJetCollection->getParameters().getIntVals(m_jetMatchingParameter().c_str(), m_jet_matching);
			m_jet_matching_source = inputJetCollection->getParameters().getIntVal(m_jetMatchingSourceParameter());

			assert(m_jet_matching.size() == m_nJets);
			assert(m_jet_matching_source == 1 || m_jet_matching_source == 2);

			m_using_kinfit = m_jet_matching_source == 2;
			m_using_mass_chi2 = m_jet_matching_source == 1;

			streamlog_out(MESSAGE) << "Jet matching from source " << m_jet_matching_source << " : (" << m_jet_matching[0] << "," << m_jet_matching[1] << ") + (" << m_jet_matching[2] << "," << m_jet_matching[3] << ")" << std::endl;

			// ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
		
			vector<ReconstructedParticle*> jets;
			for (int i=0; i<m_nJets; ++i) {
				ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
				jets.push_back(jet);
			}

			PIDHandler FTHan(inputJetCollection);
			int _FTAlgoID = FTHan.getAlgorithmID(m_JetTaggingPIDAlgorithm);
			int _FTAlgoID2 = m_use_tags2 ? FTHan.getAlgorithmID(m_JetTaggingPIDAlgorithm2) : -1;

			int BTagID = FTHan.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
			int CTagID = FTHan.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterC);

			int BTagID2 = m_use_tags2 ? FTHan.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterB2) : -1;
			int CTagID2 = m_use_tags2 ? FTHan.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterC2) : -1;
			//int OTagID = FTHan.getParameterIndex(_FTAlgoID, "OTag");

			for (int i=0; i<m_nJets; ++i) {
				const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(FTHan.getParticleID(jets[i], _FTAlgoID));
				const FloatVec& FTPara = FTImpl.getParameters();

				streamlog_out(MESSAGE) << "Reading parameters " << BTagID << " " << CTagID << " of vector with " << FTPara.size() << " elements" << std::endl;
				//double oTagValue = FTPara[OTagID];

				m_bTagValues[i] = FTPara[BTagID];
				m_cTagValues[i] = FTPara[CTagID];

				if (m_use_tags2) {
					const ParticleIDImpl& FTImpl2 = dynamic_cast<const ParticleIDImpl&>(FTHan.getParticleID(jets[i], _FTAlgoID2));
					const FloatVec& FTPara2 = FTImpl2.getParameters();

					streamlog_out(MESSAGE) << "Reading parameters2 " << BTagID2 << " " << CTagID2 << " of vector with " << FTPara2.size() << " elements" << std::endl;
					//double oTagValue = FTPara[OTagID];

					m_bTagValues2[i] = FTPara2[BTagID2];
					m_cTagValues2[i] = FTPara2[CTagID2];
				}
			}

			// calculate bmax1,2,3,4
			std::vector<double> bTagsSorted(m_bTagValues.begin(), m_bTagValues.end());
			std::sort (bTagsSorted.begin(), bTagsSorted.end());

			m_bmax1 = bTagsSorted.rbegin()[0];
			m_bmax2 = bTagsSorted.rbegin()[1];
			m_bmax3 = bTagsSorted.rbegin()[2];
			m_bmax4 = bTagsSorted.rbegin()[3];

			std::vector<double> cTagsSorted(m_cTagValues.begin(), m_cTagValues.end());
			std::sort (cTagsSorted.begin(), cTagsSorted.end());

			m_cmax1 = cTagsSorted.rbegin()[0];
			m_cmax2 = cTagsSorted.rbegin()[1];
			m_cmax3 = cTagsSorted.rbegin()[2];
			m_cmax4 = cTagsSorted.rbegin()[3];

			if (m_use_tags2) {
				std::vector<double> bTagsSorted2(m_bTagValues2.begin(), m_bTagValues2.end());
				std::sort (bTagsSorted2.begin(), bTagsSorted2.end());

				m_bmax12 = bTagsSorted2.rbegin()[0];
				m_bmax22 = bTagsSorted2.rbegin()[1];
				m_bmax32 = bTagsSorted2.rbegin()[2];
				m_bmax42 = bTagsSorted2.rbegin()[3];

				std::vector<double> cTagsSorted2(m_cTagValues2.begin(), m_cTagValues2.end());
				std::sort (cTagsSorted2.begin(), cTagsSorted2.end());

				m_cmax12 = cTagsSorted2.rbegin()[0];
				m_cmax22 = cTagsSorted2.rbegin()[1];
				m_cmax32 = cTagsSorted2.rbegin()[2];
				m_cmax42 = cTagsSorted2.rbegin()[3];
			}
		}

		// JET-MATCHING
		//const EVENT::LCParameters& pfo_params = inputPfoCollection->getParameters();
		//m_thrust = pfo_params.getFloatVal("principleThrustValue");

		// MATRIX ELEMENTS

		// ---------- PRESELECTION ----------
		// so far skipped, do it in post
		/*
		m_preselsPassedVec.push_back(m_nJets == m_nAskedJets());
		m_preselsPassedVec.push_back(m_nIsoLeps == m_nAskedIsoLeps());
		m_preselsPassedVec.push_back(m_dileptonMassDiff <= m_maxdileptonmassdiff );

		for (size_t i=0; i < m_dijetMassDiff.size(); i++) {
			m_preselsPassedVec.push_back(m_dijetMassDiff[i] <= m_maxdijetmassdiff) ;
			m_preselsPassedVec.push_back(m_dijetMass[i] <= m_maxdijetmass && m_dijetMass[i] >= m_mindijetmass);
		}

		m_preselsPassedVec.push_back(m_missingPT <= m_maxmissingPT && m_missingPT >= m_minmissingPT);
		m_preselsPassedVec.push_back(m_thrust <= m_maxthrust);
		m_preselsPassedVec.push_back(m_Evis <= m_maxEvis);
		m_preselsPassedVec.push_back(m_dihiggsMass >= m_minHHmass);
		m_preselsPassedVec.push_back(m_nbjets >= m_minnbjets);
		
		m_preselsPassedAll = std::accumulate(m_preselsPassedVec.begin(), m_preselsPassedVec.end(), 0); // 
		m_isPassed = m_preselsPassedAll == m_preselsPassedVec.size(); // Passed if passed all

		for (size_t i=0; i < m_preselsPassedVec.size(); i++) {
			if (m_preselsPassedVec[i]) {
				m_preselsPassedConsec++;
			} else {
				break;
			}
		}
		*/

	} catch(DataNotAvailableException &e) {
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
	} catch (const std::exception &exc) {
		// remove for production
    	streamlog_out(MESSAGE) << exc.what();
	}
};

void EventObservablesBase::init(){
    printParameters();

	// A. prepare some variables
	assert(m_polarizations.size() == 2);

	float Pem = m_polarizations[0];
	float Pep = m_polarizations[1];

	assert(abs(Pem) <= 1 && abs(Pep) <= 1);

	// A.2. prepare the matrix elements
	m_lcmezhh = new LCMEZHH("LCMEZHH", "ZHH", 125., Pem, Pep);
	m_lcmezzh = new LCMEZZH("LCMEZZH", "ZZH", 125., Pem, Pep);

	// A.3. flavor tagging variables
	m_bTagValues = std::vector<double>(m_nAskedJets(), -1.);
  	m_cTagValues = std::vector<double>(m_nAskedJets(), -1.);

	m_use_tags2 = m_JetTaggingPIDAlgorithm2.size() && m_JetTaggingPIDParameterB2.size() && m_JetTaggingPIDParameterC2.size();

	m_bTagValues2 = std::vector<double>(m_nAskedJets(), -1.);
  	m_cTagValues2 = std::vector<double>(m_nAskedJets(), -1.);

	prepareBaseTree();
	prepareChannelTree();
};

void EventObservablesBase::processEvent( LCEvent* evt ){
	clearBaseValues();
	updateBaseValues(evt);

	clearChannelValues();
	updateChannelValues(evt);

	if (m_write_ttree)
		getTTree()->Fill();
};

void EventObservablesBase::processRunHeader( LCRunHeader* run ){
    (void) run;
}

void EventObservablesBase::check( LCEvent* evt ){
    (void) evt;
}

void EventObservablesBase::end(){
	if (m_pTFile != NULL) {
      m_pTFile->cd();
    }
    getTTree()->Write();

    if (m_pTFile != NULL) {
      m_pTFile->Close();
      delete m_pTFile;
    }
}

// so far only supports
// (dijet_targets)=(Z,H,H), 6 jets
// (dijet targets)=(Z,H), 4asdf jets
std::tuple<std::vector<unsigned short>, vector<float>, float> EventObservablesBase::pairJetsByMass(
	std::vector<ReconstructedParticle*> jets,
	std::vector<unsigned short> dijet_targets
) {
	vector<vector<unsigned short>> perms;

	assert(dijet_targets.size() *2 == jets.size());

	if (jets.size() == 6 && dijet_targets[0] == 23 && dijet_targets[1] == 25 && dijet_targets[2] == 25) {
		perms = {
			{0,1,2,3,4,5},
			{0,2,1,3,4,5},
			{0,3,1,2,4,5},
			{0,4,1,2,3,5},
			{0,5,1,2,3,4},
			{1,2,0,3,4,5},
			{1,3,0,2,4,5},
			{1,4,0,2,3,5},
			{1,5,0,2,3,4},
			{2,3,0,1,4,5},
			{2,4,0,1,3,5},
			{2,5,0,1,3,4},
			{3,4,0,1,2,5},
			{3,5,0,1,2,4},
			{4,5,0,1,2,3}
		};
	} else if (jets.size() == 4 && dijet_targets[0] == 23 && dijet_targets[1] == 25) {
		perms = {
			{0,1,2,3},
			{0,2,1,3},
			{0,3,1,2},
			{1,2,0,3},
			{1,3,0,2},
			{2,3,0,1}
		};
	} else
		throw EVENT::Exception("Not implemented dijet pairing case");

	size_t nperm = perms.size();
	unsigned int best_idx = 0;
	float chi2min = 999999.;

	vector<float> dijetmasses_target (dijet_targets.size(), -999);
	for (size_t j=0; j < dijetmasses_target.size(); j++) {
		if (dijet_targets[j] == 23) {
			dijetmasses_target[j] = 91.1876;
		} else if (dijet_targets[j] == 25) {
			dijetmasses_target[j] = 125.;
		} else 
			throw EVENT::Exception("Not implemented dijet pairing case");
	}

	vector<float> dijetmasses_temp (dijet_targets.size(), -999);
	vector<float> dijetmasses (dijet_targets.size(), -999);
	
	float chi2;
	for (size_t i=0; i < nperm; i++) {
		chi2 = 0.;

		for (size_t j=0; j < dijetmasses.size(); j++) {
			dijetmasses_temp[j] = inv_mass(jets[perms[i][j*2]], jets[perms[i][j*2 + 1]]);
			chi2 += TMath::Power(dijetmasses_temp[j] - dijetmasses_target[j], 2);
		}

		if (chi2 < chi2min) {
			chi2min = chi2;
			dijetmasses = dijetmasses_temp;
			best_idx = i;
		}
	}
	
	return { perms[best_idx], dijetmasses, chi2min };
};

ReconstructedParticleVec EventObservablesBase::getElements(LCCollection *collection, std::vector<int> elements) {
	ReconstructedParticleVec vec;
	
	for (size_t i=0; i < elements.size(); i++) {
		ReconstructedParticle* p = dynamic_cast<ReconstructedParticle*>(collection->getElementAt(elements[i]));
		vec.push_back(p);
	}
	
	return vec;
};

void EventObservablesBase::calculateMatrixElements(
	int b1_decay_pdg,
	int b2_decay_pdg,
	TLorentzVector from_z_1, TLorentzVector from_z_2, // from z
	TLorentzVector jet1, TLorentzVector jet2, // from z or h
	TLorentzVector jet3, TLorentzVector jet4, // from h
	bool permute_from_z
){
	m_lcmezhh->SetZDecayMode(m_pdg_to_lcme_mode[b1_decay_pdg]);
	m_lcmezzh->SetZDecayMode(m_pdg_to_lcme_mode[b1_decay_pdg], m_pdg_to_lcme_mode[b2_decay_pdg]);

	std::vector<unsigned short> idx = { 0, 1, 2, 3 };
	TLorentzVector vecs_jet [4] = { jet1, jet2, jet3, jet4 };

	double result_zhh = 0.;
	double result_zzh = 0.;

	streamlog_out(MESSAGE) << "LCME Debug: E, Px, Py, Pz " << std::endl;
	streamlog_out(MESSAGE) << "  Z_1: " << from_z_1.E() << " " << from_z_1.Px() << " " << from_z_1.Py() << " " << from_z_1.Pz() << std::endl;
	streamlog_out(MESSAGE) << "  Z_2: " << from_z_2.E() << " " << from_z_2.Px() << " " << from_z_2.Py() << " " << from_z_2.Pz() << std::endl;
	streamlog_out(MESSAGE) << "  J_1: " << vecs_jet[idx[0]].E() << " " << vecs_jet[idx[0]].Px() << " " << vecs_jet[idx[0]].Py() << " " << vecs_jet[idx[0]].Pz() << std::endl;
	streamlog_out(MESSAGE) << "  J_2: " << vecs_jet[idx[1]].E() << " " << vecs_jet[idx[1]].Px() << " " << vecs_jet[idx[1]].Py() << " " << vecs_jet[idx[1]].Pz() << std::endl;
	streamlog_out(MESSAGE) << "  J_3: " << vecs_jet[idx[2]].E() << " " << vecs_jet[idx[2]].Px() << " " << vecs_jet[idx[2]].Py() << " " << vecs_jet[idx[2]].Pz() << std::endl;
	streamlog_out(MESSAGE) << "  J_4: " << vecs_jet[idx[3]].E() << " " << vecs_jet[idx[3]].Px() << " " << vecs_jet[idx[3]].Py() << " " << vecs_jet[idx[3]].Pz() << std::endl;

	/*
	std::cout << "  TOT: " << from_z_1.E() + from_z_2.E() + vecs_jet[idx[0]].E() + vecs_jet[idx[1]].E() + vecs_jet[idx[2]].E() + vecs_jet[idx[3]].E() << " "
							<< from_z_1.Px() + from_z_2.Px() + vecs_jet[idx[0]].Px() + vecs_jet[idx[1]].Px() + vecs_jet[idx[2]].Px() + vecs_jet[idx[3]].Px() << " "
							<< from_z_1.Py() + from_z_2.Py() + vecs_jet[idx[0]].Py() + vecs_jet[idx[1]].Py() + vecs_jet[idx[2]].Py() + vecs_jet[idx[3]].Py() << " "
							<< from_z_1.Pz() + from_z_2.Pz() + vecs_jet[idx[0]].Pz() + vecs_jet[idx[1]].Pz() + vecs_jet[idx[2]].Pz() + vecs_jet[idx[3]].Pz()
							<< std::endl;
	*/

	int nperms = 24;

	std::vector<std::vector<unsigned short>> perms_from_z = {{ 0, 1 }};
	if (permute_from_z) {
		perms_from_z.push_back({1, 0});
		nperms = nperms * 2;
	}
	TLorentzVector vecs_from_z [2] = { from_z_1, from_z_2 };

	// TODO: only calculating 12 is necessary, as H -> a+b is symmetric with respect to a <-> b
	
	float default_weight = 1./nperms;

	std::vector<float> weights (nperms, default_weight);

	int nperm = 0;
	double lcme_zhh = 0.;
	double lcme_zzh = 0.;

	for (auto perm_from_z: perms_from_z) {
		do {
			TLorentzVector zhh_inputs [4] = { vecs_from_z[perm_from_z[0]], vecs_from_z[perm_from_z[1]], vecs_jet[idx[0]] + vecs_jet[idx[1]], vecs_jet[idx[2]] + vecs_jet[idx[3]] };
			TLorentzVector zzh_inputs [5] = { vecs_from_z[perm_from_z[0]], vecs_from_z[perm_from_z[1]], vecs_jet[idx[0]] , vecs_jet[idx[1]], vecs_jet[idx[2]] + vecs_jet[idx[3]] };

			m_lcmezhh->SetMomentumFinal(zhh_inputs);
			m_lcmezzh->SetMomentumFinal(zzh_inputs);

			lcme_zhh = m_lcmezhh->GetMatrixElement2();
			lcme_zzh = m_lcmezzh->GetMatrixElement2();
			
			streamlog_out(DEBUG) << "Perm " << nperm << "/" <<nperms  << ": MatrixElement2 (ZHH; ZZH)=(" << lcme_zhh << "; " << lcme_zzh << ") | Weight " << weights[nperm] << std::endl;

			result_zhh += lcme_zhh * weights[nperm];
			result_zzh += lcme_zzh * weights[nperm];

			nperm += 1;
		} while (std::next_permutation(idx.begin(), idx.end()));
	}

	m_lcme_zhh_log = std::log(result_zhh);
	m_lcme_zzh_log = std::log(result_zzh);

	streamlog_out(MESSAGE) << " log(LCMEZHH)=" << m_lcme_zhh_log << std::endl;
	streamlog_out(MESSAGE) << " log(LCMEZZH)=" << m_lcme_zzh_log << std::endl;
};