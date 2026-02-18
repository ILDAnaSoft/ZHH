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

using namespace lcio ;
using namespace marlin ;
using namespace std ;

TLorentzVector v4old(ReconstructedParticle* p){ 
	return TLorentzVector( p->getMomentum()[0], p->getMomentum()[1], p->getMomentum()[2], p->getEnergy() );
}
TLorentzVector v4old(LCObject* lcobj){ ReconstructedParticle* p = dynamic_cast<ReconstructedParticle*>(lcobj); return v4old(p); }

const std::vector<std::vector<unsigned short>> EventObservablesBase::dijetPerms4 {
	{0, 1, 2, 3},
	{0, 2, 1, 3},
	{0, 3, 1, 2},
	{1, 2, 0, 3},
	{1, 3, 0, 2},
	{2, 3, 0, 1}
};

const std::vector<std::vector<unsigned short>> EventObservablesBase::dijetPerms6 {
	{0, 1, 2, 3, 4, 5}, {1, 2, 0, 3, 4, 5}, {2, 4, 0, 1, 3, 5},
    {0, 1, 2, 4, 3, 5}, {1, 2, 0, 4, 3, 5}, {2, 4, 0, 3, 1, 5},
    {0, 1, 2, 5, 3, 4}, {1, 2, 0, 5, 3, 4}, {2, 4, 0, 5, 1, 3},
    {0, 2, 1, 3, 4, 5}, {1, 3, 0, 2, 4, 5}, {2, 5, 0, 1, 3, 4},
    {0, 2, 1, 4, 3, 5}, {1, 3, 0, 4, 2, 5}, {2, 5, 0, 3, 1, 4},
    {0, 2, 1, 5, 3, 4}, {1, 3, 0, 5, 2, 4}, {2, 5, 0, 4, 1, 3},
    {0, 3, 1, 2, 4, 5}, {1, 4, 0, 2, 3, 5}, {3, 4, 0, 1, 2, 5},
    {0, 3, 1, 4, 2, 5}, {1, 4, 0, 3, 2, 5}, {3, 4, 0, 2, 1, 5},
    {0, 3, 1, 5, 2, 4}, {1, 4, 0, 5, 2, 3}, {3, 4, 0, 5, 1, 2},
    {0, 4, 1, 2, 3, 5}, {1, 5, 0, 2, 3, 4}, {3, 5, 0, 1, 2, 4},
	{0, 4, 1, 3, 2, 5}, {1, 5, 0, 3, 2, 4}, {3, 5, 0, 2, 1, 4},
    {0, 4, 1, 5, 2, 3}, {1, 5, 0, 4, 2, 3}, {3, 5, 0, 4, 1, 2},
    {0, 5, 1, 2, 3, 4}, {2, 3, 0, 1, 4, 5}, {4, 5, 0, 1, 2, 3},
    {0, 5, 1, 3, 2, 4}, {2, 3, 0, 4, 1, 5}, {4, 5, 0, 2, 1, 3},
    {0, 5, 1, 4, 2, 3}, {2, 3, 0, 5, 1, 4}, {4, 5, 0, 3, 1, 2},
};

EventObservablesBase::EventObservablesBase(const std::string &name) : Processor(name),
  m_write_ttree(true),
  m_pTFile(NULL),
  m_nRun(0),
  m_nEvt(0),
  m_statusCode(0)
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

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			"JetsKinFitZHH",
			"Name of the Jet collection of the ZHH kinfit",
			m_inputJetKinFitZHHCollection,
			std::string("")
			);

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			"JetsKinFitZZH",
			"Name of the Jet collection of the ZZH kinfit",
			m_inputJetKinFitZZHCollection,
			std::string("")
			);

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			"LeptonsKinFit_solveNu",
			"Name of the Lepton collection of the 4C kinfit",
			m_inputLeptonKinFit_solveNuCollection,
			std::string("LeptonsKinFit_solveNu")
			);

registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			"JetsKinFit_solveNu",
			"Name of the Jet collection of the 4C kinfit",
			m_inputJetKinFit_solveNuCollection,
			std::string("JetsKinFit_solveNu")
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
            "Name of flavor tagging algo 1",
            m_JetTaggingPIDAlgorithm,
            std::string("weaver")
            );

	registerProcessorParameter("JetTaggingPIDParameterB",
            "B parameter",
            m_JetTaggingPIDParameterB,
            std::string("mc_b")
            );
	
	registerProcessorParameter("JetTaggingPIDParameterBbar",
            "Bbar parameter",
            m_JetTaggingPIDParameterBbar,
            std::string("mc_bbar")
            );

	registerProcessorParameter("JetTaggingPIDParameterC",
            "C paramter",
            m_JetTaggingPIDParameterC,
            std::string("mc_c")
            );

	registerProcessorParameter("JetTaggingPIDParameterCbar",
            "Cbar parameter",
            m_JetTaggingPIDParameterCbar,
            std::string("mc_cbar")
            );

	registerProcessorParameter("JetTaggingPIDParameters",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameters,
            std::vector<std::string>{"mc_b mc_bbar mc_c mc_cbar mc_d mc_dbar mc_g mc_s mc_sbar mc_u mc_ubar"}
            );

	registerProcessorParameter("JetTaggingPIDAlgorithm2",
            "Name of flavor tagging algo 2",
            m_JetTaggingPIDAlgorithm2,
            std::string("lcfiplus")
            );

	registerProcessorParameter("JetTaggingPIDParameterB2",
            "B parameter for algo 2",
            m_JetTaggingPIDParameterB2,
            std::string("BTag")
            );

	registerProcessorParameter("JetTaggingPIDParameterC2",
            "C paramter for algo 2",
            m_JetTaggingPIDParameterC2,
            std::string("CTag")
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

	registerProcessorParameter("jetChargeKappa",
			"exponent kappa of constituents i, their energy fraction z_i for calculation of jet charge Q = sum_i (q_i * z_i^kappa)",
			m_jetChargeKappa,
			float(.3)
			);

  	registerProcessorParameter("outputFilename",
			"name of output root file",
			m_outputFile,
			std::string("")
			);

	// TrueJet_Parser parameters
  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
                           "TrueJets" ,
                           "Name of the TrueJetCollection input collection"  ,
                           _trueJetCollectionName ,
                           std::string("TrueJets") ) ;

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
                           "FinalColourNeutrals" ,
                           "Name of the FinalColourNeutralCollection input collection"  ,
                           _finalColourNeutralCollectionName ,
                           std::string("FinalColourNeutrals") ) ;

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
                           "InitialColourNeutrals" ,
                           "Name of the InitialColourNeutralCollection input collection"  ,
                           _initialColourNeutralCollectionName ,
                           std::string("InitialColourNeutrals") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "TrueJetPFOLink" ,
                            "Name of the TrueJetPFOLink input collection"  ,
                            _trueJetPFOLink,
                            std::string("TrueJetPFOLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "TrueJetMCParticleLink" ,
                            "Name of the TrueJetMCParticleLink input collection"  ,
                            _trueJetMCParticleLink,
                            std::string("TrueJetMCParticleLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "FinalElementonLink" ,
                            "Name of the  FinalElementonLink input collection"  ,
                            _finalElementonLink,
                            std::string("FinalElementonLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "InitialElementonLink" ,
                            "Name of the  InitialElementonLink input collection"  ,
                            _initialElementonLink,
                            std::string("InitialElementonLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "FinalColourNeutralLink" ,
                            "Name of the  FinalColourNeutralLink input collection"  ,
                            _finalColourNeutralLink,
                            std::string("FinalColourNeutralLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "InitialColourNeutralLink" ,
                            "Name of the  InitialColourNeutralLink input collection"  ,
                            _initialColourNeutralLink,
                            std::string("InitialColourNeutralLink") ) ;

  registerInputCollection( LCIO::LCRELATION,
                            "RecoMCTruthLink",
                            "Name of the RecoMCTruthLink input collection"  ,
                            _recoMCTruthLink,
                            string("RecoMCTruthLink")
                            );
};

void EventObservablesBase::prepareBaseTree()
{
	// PREPARE PROCESSOR ARGUMENTS
	float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
	double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);

	m_ecms = ROOT::Math::PxPyPzEVector(target_p_due_crossing_angle, 0., 0., E_lab) ;

	TTree* ttree = getTTree();

	if (m_outputFile.size()) {
		m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
		ttree->SetDirectory(m_pTFile);
	}

	if (m_write_ttree) {
		ttree->Branch("run", &m_nRun, "run/I");
		ttree->Branch("event", &m_nEvt, "event/I");
		ttree->Branch("errorCode", &m_statusCode, "errorCode/I");
		ttree->Branch("errorCodes", &m_errorCodes);

		ttree->Branch("evis", &m_Evis, "evis/F");
		ttree->Branch("m_miss", &m_missingMass, "m_miss/F");
		ttree->Branch("m_invjet", &m_invJetMass, "m_invjet/F");
		ttree->Branch("ptmiss", &m_missingPT, "ptmiss/F");
		ttree->Branch("emiss", &m_missingE, "emiss/F");
		ttree->Branch("thrust", &m_thrust, "thrust/F");
		ttree->Branch("thrustmajor", &m_thrustMajor, "thrustmajor/F");
		ttree->Branch("thrustminor", &m_thrustMinor, "thrustminor/F");
		ttree->Branch("costhrust", &m_thrustAxisCos, "costhrust/F");

		ttree->Branch("ptpfochargedmax", &m_ptpfochargedmax, "ptpfochargedmax/F");
		ttree->Branch("ppfochargedmax", &m_ppfochargedmax, "ppfochargedmax/F");

		ttree->Branch("ptpfomax", &m_ptpfomax, "ptpfomax/F");
		ttree->Branch("ptjmax", &m_ptjmax, "ptjmax/F");
		ttree->Branch("pjmax", &m_pjmax, "pjmax/F");
		ttree->Branch("cosjmax", &m_cosjmax, "cosjmax/F");

		ttree->Branch("njets", &m_nJets, "njets/I");
		ttree->Branch("nisoleptons", &m_nIsoLeps, "nisoleptons/I");
		ttree->Branch("nisoelectrons", &m_nIsoElectrons, "nisoelectrons/I");
		ttree->Branch("nisomuons", &m_nIsoMuons, "nisomuons/I");
		ttree->Branch("nisotaus", &m_nIsoTaus, "nisotaus/I");
		ttree->Branch("pairedLepType", &m_pairedLepType, "pairedLepType/I");
		ttree->Branch("npfos", &m_npfos, "npfos/I");
		ttree->Branch("lep_types", &m_lep_types);

		ttree->Branch("yminus", &m_yMinus, "yminus/F");
		ttree->Branch("yplus", &m_yPlus, "yplus/F");

		ttree->Branch("zhh_jet_matching", &m_zhh_jet_matching);
		ttree->Branch("zhh_mz", &m_zhh_mz, "zhh_mz/F");
		ttree->Branch("zhh_mh1", &m_zhh_mh1, "zhh_mh1/F");
		ttree->Branch("zhh_mh2", &m_zhh_mh2, "zhh_mh2/F");
		ttree->Branch("zhh_mhh", &m_zhh_mhh, "zhh_mhh/F");
		ttree->Branch("zhh_chi2", &m_zhh_chi2, "zhh_chi2/F");

		// jet matching from KinFit
		ttree->Branch("jet_matching_kinfit_zhh", &m_JMK_ZHH);
		ttree->Branch("jet_matching_kinfit_zzh", &m_JMK_ZZH);
		ttree->Branch("jet_matching_kinfit_best", &m_JMK_best);

		ttree->Branch("fitprob_ZHH", &m_fitprob_ZHH, "fitprob_ZHH/F");
		ttree->Branch("fitprob_ZZH", &m_fitprob_ZZH, "fitprob_ZZH/F");
		ttree->Branch("fitchi2_ZHH", &m_fitchi2_ZHH, "fitchi2_ZHH/F");
		ttree->Branch("fitchi2_ZZH", &m_fitchi2_ZZH, "fitchi2_ZZH/F");

		// 4C fit
		ttree->Branch("fit4C_masses", &m_fit4C_masses);
		ttree->Branch("fit4C_mz", &m_fit4C_mz, "fit4C_mz/F");
		ttree->Branch("fit4C_mh1", &m_fit4C_mh1, "fit4C_mh1/F");
		ttree->Branch("fit4C_mh2", &m_fit4C_mh2, "fit4C_mh2/F");
		ttree->Branch("fit4C_mhh", &m_fit4C_mhh, "fit4C_mhh/F");

		// nhbb:njet:chi2:mpt:prob11:prob12:prob21:prob22

		ttree->Branch("bTags", &m_bTagValues);
		ttree->Branch("cTags", &m_cTagValues);

		// bmax1:bmax2:bmax3:bmax4:pj1jets2:pj2jets2
		ttree->Branch("cosbmax", &m_cosbmax, "cosbmax/F");

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
		ttree->Branch("jet1_4v", &m_jets4v[0]);
		ttree->Branch("jet1_m", &m_jetsMasses[0]);
		ttree->Branch("jet1_tags", &m_jetTags[0]);
		ttree->Branch("jet1_q", &m_jet1_q, "jet1_q/F");
        ttree->Branch("jet1_qdyn", &m_jet1_qdyn, "jet1_qdyn/F");

		ttree->Branch("jet2_4v", &m_jets4v[1]);
		ttree->Branch("jet2_m", &m_jetsMasses[1]);
		ttree->Branch("jet2_tags", &m_jetTags[1]);
		ttree->Branch("jet2_q", &m_jet2_q, "jet2_q/F");
        ttree->Branch("jet2_qdyn", &m_jet2_qdyn, "jet2_qdyn/F");

		ttree->Branch("jet3_4v", &m_jets4v[2]);
		ttree->Branch("jet3_m", &m_jetsMasses[2]);
		ttree->Branch("jet3_tags", &m_jetTags[2]);
		ttree->Branch("jet3_q", &m_jet3_q, "jet3_q/F");
        ttree->Branch("jet3_qdyn", &m_jet3_qdyn, "jet3_qdyn/F");

		ttree->Branch("jet4_4v", &m_jets4v[3]);
		ttree->Branch("jet4_m", &m_jetsMasses[3]);
		ttree->Branch("jet4_tags", &m_jetTags[3]);
		ttree->Branch("jet4_q", &m_jet4_q, "jet4_q/F");
        ttree->Branch("jet4_qdyn", &m_jet4_qdyn, "jet4_qdyn/F");

		// jet matching
		//ttree->Branch("jet_matching", &m_jet_matching);
		//ttree->Branch("jet_matching_source", &m_jet_matching_source, "jet_matching_source/I");
		//ttree->Branch("using_kinfit", &m_using_kinfit, "using_kinfit/B");
		//ttree->Branch("using_mass_chi2", &m_using_mass_chi2, "using_mass_chi2/B");

		// matrix elements
		#ifdef CALC_ME
		if (m_use_matrix_elements()) {
			ttree->Branch("me_zhh_raw", &m_lcme_zhh_raw);
			ttree->Branch("me_zzh_raw", &m_lcme_zzh_raw);

			ttree->Branch("me_zhh_log", &m_lcme_zhh_log, "me_zhh_log/D");
			ttree->Branch("me_zzh_log", &m_lcme_zzh_log, "me_zzh_log/D");

			ttree->Branch("me_jmk_zhh_log", &m_lcme_jmk_zhh_log, "me_jmk_zhh_log/D");
			ttree->Branch("me_jmk_zzh_log", &m_lcme_jmk_zzh_log, "me_jmk_zzh_log/D");
		}
		#endif

		// TrueJet
		ttree->Branch("true_lep_n", &m_trueLeptonN);
		ttree->Branch("true_lep_pdgs", &m_trueLeptonPDGs);

		if (m_nAskedIsoLeps() == 2) {
			ttree->Branch("true_lep1_v4", &m_trueLeptonMomenta[0]);
			ttree->Branch("true_lep2_v4", &m_trueLeptonMomenta[1]);
		}

		ttree->Branch("true_jet_n", &m_trueJetN);
		ttree->Branch("true_jet_types", &m_trueJetTypes);
		ttree->Branch("true_jet_pdgs", &m_trueJetPDGs);
		ttree->Branch("true_dijet_icn_pdgs", &m_trueDijetICNPDGs);

		ttree->Branch("true_jet1_v4", &m_trueJetMomenta[0]);
		ttree->Branch("true_jet2_v4", &m_trueJetMomenta[1]);
		ttree->Branch("true_jet3_v4", &m_trueJetMomenta[2]);
		ttree->Branch("true_jet4_v4", &m_trueJetMomenta[3]);

		if (m_nAskedJets() == 6) {
			ttree->Branch("true_jet5_v4", &m_trueJetMomenta[4]);
			ttree->Branch("true_jet6_v4", &m_trueJetMomenta[5]);
		}

		ttree->Branch("true_reco_jets_mapped", &m_trueRecoJetsMapped, "true_reco_jets_mapped/I");
		ttree->Branch("true_isr_momenta", &m_trueISRMomenta);

		ttree->Branch("reco_to_true_index", &m_reco2TrueJetIndex);
		ttree->Branch("true_to_reco_index", &m_true2RecoJetIndex);

		// ttree->Branch("true_jet_higgs_icn_pairs", &m_trueJetHiggsICNPairs);

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

	// collections
	m_useTrueJet = false;
	inputLKF_solveNuCollection = NULL;
	inputJKF_solveNuCollection = NULL;

	m_statusCode = 0;
	m_errorCodes.clear();

	m_pmis.SetPxPyPzE(0.,0.,0.,0.);
	
	m_missingPT = -999.;
	m_missingE = -999.;
	m_thrust = -999.;
	m_thrustMajor = -999.;
	m_thrustMinor = -999.;
	m_thrustAxisCos = -999.;

	m_ptpfochargedmax = 0.;
	m_ppfochargedmax = 0.;

	m_ptpfomax = 0.;
	m_ptjmax = 0.;
	m_pjmax = 0.;
	m_cosjmax = 0.;

	m_nJets = 0;
	m_nIsoLeps = 0;
	m_nIsoElectrons = 0;
	m_nIsoMuons = 0;
	m_nIsoTaus = 0;
	m_pairedLepType = 0;
	m_npfos = 0;
	m_lep_types.clear();

	m_Evis  = -999.;
	m_missingMass = -999.;
	m_invJetMass = 0.;

	m_yMinus = 0.;
	m_yPlus = 0.;
	
	// flavor tagging
	m_bTagsSorted.clear();
	m_cTagsSorted.clear();

	std::fill(m_bTagValues.begin(), m_bTagValues.end(), -1.);
	std::fill(m_cTagValues.begin(), m_cTagValues.end(), -1.);

	m_cosbmax = 0.;

	m_bmax1 = 0.;
	m_bmax2 = 0.;
	m_bmax3 = 0.;
	m_bmax4 = 0.;

	m_cmax1 = 0.;
	m_cmax2 = 0.;
	m_cmax3 = 0.;
	m_cmax4 = 0.;

	m_bTagsSorted2.clear();
	m_cTagsSorted2.clear();
	std::fill(m_bTagValues2.begin(), m_bTagValues2.end(), -1.);
	std::fill(m_cTagValues2.begin(), m_cTagValues2.end(), -1.);

	m_bmax12 = 0.;
	m_bmax22 = 0.;
	m_bmax32 = 0.;
	m_bmax42 = 0.;

	m_cmax12 = 0.;
	m_cmax22 = 0.;
	m_cmax32 = 0.;
	m_cmax42 = 0.;

	m_jets.clear();

	// ZHH
	m_zhh_jet_matching.clear();
    m_zhh_mz = 0.;
    m_zhh_mh1 = 0.;
    m_zhh_mh2 = 0.;
	m_zhh_mhh = 0.;
    m_zhh_chi2 = 0.;
	m_zhh_p1st = 0.;
	m_zhh_cosTh1st = 0.;

	#ifdef CALC_ME
	m_lcme_zhh_log = 0.;
	m_lcme_zzh_log = 0.;

	m_lcme_jmk_zhh_log = 0.;
	m_lcme_jmk_zzh_log = 0.;

	m_lcme_weights.clear();
	m_lcme_zhh_raw.clear();
	m_lcme_zzh_raw.clear();
	#endif
	
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

	// jet quantities
	for (size_t i = 0; i < m_jets4v.size(); i++) {
		m_jets4v[i].SetPxPyPzE(0., 0., 0., 0.);
		m_jetsMasses[i] = 0.;
	}

	for (size_t i = 0; i < m_jetTags.size(); i++)
		std::fill(m_jetTags[i].begin(), m_jetTags[i].end(), 0.);

	m_jet1_q  = 0.;
	m_jet1_qdyn = 0.;

	m_jet2_q  = 0.;
	m_jet2_qdyn = 0.;

	m_jet3_q  = 0.;
	m_jet3_qdyn = 0.;

	m_jet4_q  = 0.;
	m_jet4_qdyn = 0.;

	// jet matching
	std::fill(m_JMK_ZHH.begin(), m_JMK_ZHH.end(), -1);
	std::fill(m_JMK_ZZH.begin(), m_JMK_ZZH.end(), -1);
	std::fill(m_JMK_best.begin(), m_JMK_best.end(), -1);

	m_fitprob_ZHH = 0.;
	m_fitprob_ZZH = 0.;
	m_fitchi2_ZHH = 0.;
	m_fitchi2_ZZH = 0.;

	m_fit4C_masses.clear();
	m_fit4C_mz = 0.;
	m_fit4C_mh1 = 0.;
	m_fit4C_mh2 = 0.;
	m_fit4C_mhh = 0.;

	//m_JMK_ZHH_perm_idx = -1;
	//m_JMK_ZZH_perm_idx = -1;

	// TrueJet
	m_trueLeptonN = 0;
	m_trueJetN = 0;
	for (size_t i = 0; i < m_trueLeptonMomenta.size(); i++) {
		m_trueLeptonMomenta[i].SetPxPyPzE(0., 0., 0., 0.);
		m_trueLeptonPDGs[i] = 0;
	}

	for (size_t i = 0; i < m_trueJetMomenta.size(); i++) {
		m_trueJetTypes[i] = 0;
		m_trueJetPDGs[i] = 0;
		m_trueJetMomenta[i].SetPxPyPzE(0., 0., 0., 0.);

		if (i % 2 == 0)
			m_trueDijetICNPDGs[i / 2] = 0;
	}

	m_trueISRMomenta[0].SetPxPyPzE(0., 0., 0., 0.);
	m_trueISRMomenta[1].SetPxPyPzE(0., 0., 0., 0.);

	m_trueRecoJetsMapped = false;

	std::fill(m_reco2TrueJetIndex.begin(), m_reco2TrueJetIndex.end(), -1.);
	std::fill(m_true2RecoJetIndex.begin(), m_true2RecoJetIndex.end(), -1.);

	//for (size_t i = 0; i < 2; i++) {
	//	m_trueJetHiggsICNPairs[i] = 0;
	//}
}

void EventObservablesBase::updateBaseValues(EVENT::LCEvent *pLCEvent) {
	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();

	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

	try {
		LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // main jet collection
		LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
		LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
		LCCollection *inputPfoCollection = pLCEvent->getCollection( m_inputPfoCollection );

		m_nJets = inputJetCollection->getNumberOfElements();
		if (m_nJets != m_nAskedJets()) {
			m_statusCode = 1;
			throw EVENT::Exception("Unexpected number of input jets");
		}

		// Handle kinfit
		try {
			inputLKF_solveNuCollection = pLCEvent->getCollection( m_inputLeptonKinFit_solveNuCollection );
			inputJKF_solveNuCollection = pLCEvent->getCollection( m_inputJetKinFit_solveNuCollection );

			LCCollection *inputJKF_ZHHCollection = pLCEvent->getCollection( m_inputJetKinFitZHHCollection );
			LCCollection *inputJKF_ZZHCollection = pLCEvent->getCollection( m_inputJetKinFitZZHCollection );

			m_fitprob_ZHH = inputJKF_ZHHCollection->parameters().getFloatVal("fitprob");
			m_fitprob_ZZH = inputJKF_ZZHCollection->parameters().getFloatVal("fitprob");
			m_fitchi2_ZHH = inputJKF_ZHHCollection->parameters().getFloatVal("fitchi2");
			m_fitchi2_ZZH = inputJKF_ZZHCollection->parameters().getFloatVal("fitchi2");

			EVENT::IntVec JMK_ZHH;
			EVENT::IntVec JMK_ZZH;

			inputJKF_ZHHCollection->parameters().getIntVals("permutation", JMK_ZHH);
			inputJKF_ZZHCollection->parameters().getIntVals("permutation", JMK_ZZH);

			// JMK_ZHH/ZZH might hold more than m_nAskedJets() entries, only use first m_nAskedJets() values
			if (JMK_ZHH.size() >= m_nAskedJets())
				for (unsigned int i = 0; i < m_nAskedJets(); i++)
					m_JMK_ZHH[i] = JMK_ZHH[i];

			if (JMK_ZZH.size() >= m_nAskedJets())
				for (unsigned int i = 0; i < m_nAskedJets(); i++)
					m_JMK_ZZH[i] = JMK_ZZH[i];

			if (JMK_ZHH.size() >= m_nAskedJets() || JMK_ZZH.size() >= m_nAskedJets()) {
				m_JMK_best = (m_fitchi2_ZHH <= m_fitchi2_ZZH ? m_JMK_ZHH : m_JMK_ZZH);
			}

		} catch(DataNotAvailableException &e) {
			streamlog_out(MESSAGE) << "processEvent : Input kinfit solveNu jet and lepton collections not found in event " << m_nEvt << std::endl;
			m_statusCode += 100;
		}

		// ---------- NUMBER OF ISOLATED LEPTONS and PFOs ----------
		m_nIsoLeps = inputLeptonCollection->getNumberOfElements();
		for (int i = 0; i < m_nIsoLeps; i++) {
			ReconstructedParticle* iso_lepton = (ReconstructedParticle*) inputLeptonCollection->getElementAt(i);
			int type = abs(iso_lepton->getType());

			switch (type) {
				case 11: m_nIsoElectrons++; break;
				case 13: m_nIsoMuons++; break;
				case 15: m_nIsoTaus++; break;
				default:
					streamlog_out(WARNING) << "Unknown lepton type: " << type << std::endl;
					break;
			}
		}

		m_npfos = inputPfoCollection->getNumberOfElements();

		// ISOLATED LEPTON PAIRING
		m_pairedLepType = inputLepPairCollection->parameters().getIntVal("PairedType");

		// ---------- MISSING PT ----------
		// corrected for crossing angle
		ROOT::Math::PxPyPzEVector pfosum(0.,0.,0.,0.);
		for (int i=0; i < m_npfos; i++) {
			ReconstructedParticle* pfo = (ReconstructedParticle*) inputPfoCollection->getElementAt(i);
			ROOT::Math::PxPyPzEVector pfo_v4 = v4(pfo);

			pfosum += pfo_v4;
			m_ptpfomax = std::max(m_ptpfomax, (float)pfo_v4.Pt());

			if (pfo->getCharge() != 0) {
				m_ptpfochargedmax = std::max(m_ptpfochargedmax, (float)pfo_v4.Pt());
				m_ppfochargedmax = std::max(m_ppfochargedmax, (float)pfo_v4.P());
			}
		}
		m_pmis = m_ecms - pfosum;
		
		m_missingPT = m_pmis.Pt();
		m_missingMass = m_pmis.M();
		m_missingE = m_pmis.E();
		
		// ---------- VISIBLE ENERGY ----------
		m_Evis = pfosum.E();

		// ---------- THRUST ----------
		const EVENT::LCParameters& raw_pfo_params = inputPfoCollection->getParameters();

		m_thrust = raw_pfo_params.getFloatVal("principleThrustValue");
		m_thrustMajor = raw_pfo_params.getFloatVal("majorThrustValue");
		m_thrustMinor = raw_pfo_params.getFloatVal("minorThrustValue");
		
		FloatVec thrustAxis;
		raw_pfo_params.getFloatVals("principleThrustAxis", thrustAxis);
		
		if (thrustAxis.size() >= 3) {
			TVector3 principleAxis = TVector3(thrustAxis[0], thrustAxis[1], thrustAxis[2]);
			m_thrustAxisCos = principleAxis.CosTheta();
		} else
			m_statusCode = 40;

		// ---------- SAVE TYPES OF ALL ISOLATED LEPTONS ----------
		for (int j = 0; j < inputLeptonCollection->getNumberOfElements(); j++) {
			ReconstructedParticle* iso_lepton = dynamic_cast<ReconstructedParticle*>( inputLeptonCollection->getElementAt( j ) );
			m_lep_types.push_back( iso_lepton->getType() );
		}

		// continue only if the number of jets and isolated leptons match the preselection
		// and the numbers in the Kinfit processors
		
		if ( m_nJets == m_nAskedJets() ) {
			streamlog_out(MESSAGE) << "Continue with "<< m_nJets << " jets and "<< inputLepPairCollection->getNumberOfElements() << " ISOLeptons" << std::endl;

			// ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
			ROOT::Math::PxPyPzEVector jetsum(0.,0.,0.,0.);
			for (unsigned int i=0; i < m_nJets; ++i) {
				ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
				ROOT::Math::PxPyPzEVector jet_v4 = v4(jet);
				
				float currentJetMomentum = jet_v4.P();

				if (currentJetMomentum > m_pjmax)
					m_cosjmax = jet_v4.Pz() / currentJetMomentum;

				m_ptjmax = std::max(m_ptjmax, (float)jet_v4.Pt());
				m_pjmax = std::max(m_pjmax, currentJetMomentum);
				jetsum += jet_v4;

				m_jets.push_back(jet);
				m_jets4v[i] = jet_v4;
				m_jetsMasses[i] = jet_v4.M();
			}
			m_invJetMass = jetsum.M();

			PIDHandler jetPIDh(inputJetCollection);
			int _FTAlgoID = jetPIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm);
			int _FTAlgoID2 = m_use_tags2 ? jetPIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm2) : -1;

			int BTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
			int BbarTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterBbar);
			int CTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterC);
			int CbarTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterCbar);

			int BTagID2 = m_use_tags2 ? jetPIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterB2) : -1;
			int CTagID2 = m_use_tags2 ? jetPIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterC2) : -1;
			//int OTagID = jetPIDh.getParameterIndex(_FTAlgoID, "OTag");

			streamlog_out(MESSAGE) << "--- FLAVOR TAG ---" << std::endl;
			for (unsigned int i=0; i < m_nJets; ++i) {
				const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(jetPIDh.getParticleID(m_jets[i], _FTAlgoID));
				const FloatVec& FTPara = FTImpl.getParameters();

				m_bTagValues[i] = FTPara[BTagID] + FTPara[BbarTagID];
				m_cTagValues[i] = FTPara[CTagID] + FTPara[CbarTagID];
				streamlog_out(MESSAGE) << "Jet " << i << std::endl << " > Algo1 - B,C [B,Bbar; C,Cbar] = " << m_bTagValues[i] << ", " << m_cTagValues[i] << " [" << FTPara[BTagID] << ", " << FTPara[BbarTagID] << ", " << FTPara[CTagID] << ", " << FTPara[CbarTagID] << "]" << std::endl;

				// write all requested parameters
				for (size_t j = 0; j < m_JetTaggingPIDParameters.size(); j++) {
					int param_id = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameters[j]);
					if (param_id + 1 > (int)FTPara.size()) {
						std::cerr << "Parameter error: Param " << j << ", value=" << m_JetTaggingPIDParameters[j] << std::endl;
						throw EVENT::Exception("No flavor tagging value for parameter");
					}
					
					m_jetTags[i][j] = FTPara[param_id];
				}

				if (m_use_tags2) {
					const ParticleIDImpl& FTImpl2 = dynamic_cast<const ParticleIDImpl&>(jetPIDh.getParticleID(m_jets[i], _FTAlgoID2));
					const FloatVec& FTPara2 = FTImpl2.getParameters();

					m_bTagValues2[i] = FTPara2[BTagID2];
					m_cTagValues2[i] = FTPara2[CTagID2];

					streamlog_out(MESSAGE) << " > Algo2 - B,C = " << FTPara2[BTagID2] << "," << FTPara2[CTagID2] << std::endl;
					//double oTagValue = FTPara[OTagID];

					// replace weaver value with LCFIPlus value if NaN (ca. 2% of b-jets in μμHH events)
					if (std::isnan(m_bTagValues[i]) && !std::isnan(m_bTagValues2[i])) {
						m_bTagValues[i] = m_bTagValues2[i];
					}
				}
			}

			// calculate bmax1,2,3,4
			for (size_t i = 0; i < m_bTagValues.size(); i++)
				m_bTagsSorted.push_back(std::make_pair(i, m_bTagValues[i]));

			std::sort (m_bTagsSorted.begin(), m_bTagsSorted.end(), jetTaggingComparator);

			TVector3 pjbmaxA1 (m_jets[m_bTagsSorted[0].first]->getMomentum());
			TVector3 pjbmaxA2 (m_jets[m_bTagsSorted[1].first]->getMomentum());
			m_cosbmax = pjbmaxA1.Dot(pjbmaxA2)/pjbmaxA1.Mag()/pjbmaxA2.Mag();

			m_bmax1 = m_bTagsSorted[0].second;
			m_bmax2 = m_bTagsSorted[1].second;
			m_bmax3 = m_bTagsSorted[2].second;
			m_bmax4 = m_bTagsSorted[3].second;

			for (size_t i = 0; i < m_cTagValues.size(); i++)
				m_cTagsSorted.push_back(std::make_pair(i, m_cTagValues[i]));

			std::sort (m_cTagsSorted.begin(), m_cTagsSorted.end(), jetTaggingComparator);

			m_cmax1 = m_cTagsSorted[0].second;
			m_cmax2 = m_cTagsSorted[1].second;
			m_cmax3 = m_cTagsSorted[2].second;
			m_cmax4 = m_cTagsSorted[3].second;

			if (m_use_tags2) {
				for (size_t i = 0; i < m_bTagValues2.size(); i++)
					m_bTagsSorted2.push_back(std::make_pair(i, m_bTagValues2[i]));

				std::sort (m_bTagsSorted2.begin(), m_bTagsSorted2.end(), jetTaggingComparator);

				m_bmax12 = m_bTagsSorted2[0].second;
				m_bmax22 = m_bTagsSorted2[1].second;
				m_bmax32 = m_bTagsSorted2[2].second;
				m_bmax42 = m_bTagsSorted2[3].second;

				for (size_t i = 0; i < m_cTagValues2.size(); i++)
					m_cTagsSorted2.push_back(std::make_pair(i, m_cTagValues2[i]));

				std::sort (m_cTagsSorted2.begin(), m_cTagsSorted2.end(), jetTaggingComparator);

				m_cmax12 = m_cTagsSorted2[0].second;
				m_cmax22 = m_cTagsSorted2[1].second;
				m_cmax32 = m_cTagsSorted2[2].second;
				m_cmax42 = m_cTagsSorted2[3].second;
			}

			// ---------- YMINUS, YPLUS ----------        
			int algo_y = jetPIDh.getAlgorithmID("yth");
			const ParticleID & ythID = jetPIDh.getParticleID(m_jets[0], algo_y); // same arguments for all jets
	
			FloatVec params_y = ythID.getParameters();
			m_yMinus = params_y[jetPIDh.getParameterIndex(algo_y, m_yMinusParameter())];
			m_yPlus = params_y[jetPIDh.getParameterIndex(algo_y, m_yPlusParameter())];

			if ( inputLepPairCollection->getNumberOfElements() != (int)m_nAskedIsoLeps() ) {
				m_statusCode = 2;
			}
		} else
			m_statusCode = 1;

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
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << ": " << e.what() << std::endl;
		m_statusCode = 20;
	} catch (const std::exception &exc) {
		// remove for production
    	streamlog_out(MESSAGE) << exc.what();
		m_statusCode = 30;
	}

	try {
		pLCEvent->getCollection(_trueJetCollectionName);
		m_useTrueJet = true;
	} catch(DataNotAvailableException &e) {
		streamlog_out(ERROR) << "No TrueJet collection " << _trueJetCollectionName << " found. Skipping recording of truth four momenta" << std::endl;
		m_useTrueJet = false;
	}

	if (m_nJets == m_nAskedJets()) {
		const std::string process_name = pLCEvent->getParameters().getStringVal("processName");

		if (m_useTrueJet) {
			TrueJet_Parser* trueJet = this;
			trueJet->getall( pLCEvent );
			
			vector<int> trueHadronicJetIndices;
			vector<int> trueLeptonIndices;
			vector<int> trueChargedLeptonIndices;
			vector<int> trueISRIndices;
			float smallestSumCosAngle = getMatchingByAngularSpace(m_jets, m_reco2TrueJetIndex, m_true2RecoJetIndex, trueHadronicJetIndices, trueLeptonIndices, trueISRIndices);

			// case in which number of hadronic jets == m_nJets
			m_trueRecoJetsMapped = smallestSumCosAngle < 99;

			// store hadronic true jets
			m_trueJetN = trueHadronicJetIndices.size();

			// store leptons
			m_trueLeptonN = trueLeptonIndices.size();

			std::copy_if(trueLeptonIndices.begin(), trueLeptonIndices.end(), std::back_inserter(trueChargedLeptonIndices),
				[trueJet](int index){
					int pdg = abs( ( (ReconstructedParticle*)trueJet->jet(index) )->getParticleIDs()[0]->getPDG() );
					return (pdg == 11 || pdg == 13 || pdg == 15);
				});

			// sort leptons ASC by energy
			std::sort(trueChargedLeptonIndices.begin(), trueChargedLeptonIndices.end(), [trueJet]( const int& index1, const int& index2 ) {
				const ReconstructedParticle* true_lep1 = trueJet->jet(index1);
				const ReconstructedParticle* true_lep2 = trueJet->jet(index2);

				return true_lep1->getEnergy() > true_lep2->getEnergy();
			});

			if (trueChargedLeptonIndices.size() >= m_nAskedIsoLeps()) {
				for (unsigned int i = 0; i < m_nAskedIsoLeps(); i++) {
					int index = trueChargedLeptonIndices[i];
					const ReconstructedParticle* tjet = trueJet->jet(index);

					m_trueLeptonMomenta[i] = v4(tjet);
					m_trueLeptonPDGs[i] = tjet->getParticleIDs()[0]->getPDG();
				}
			}

			std::vector<int> icn_pdgs = {23, 25};

			if (m_trueRecoJetsMapped) {
				std::vector<int> truejetpermICNs;

				//streamlog_out(DEBUG3) << "number of icns = " << nicn << endl;
				// sort ICNs: any Higgs first; important as this is the order assumed at MEM calculation for the jet matching 
				std::vector<int> icn_indices_sorted(trueJet->nicn());
				std::iota(icn_indices_sorted.begin(), icn_indices_sorted.end(), 0);

				std::sort(icn_indices_sorted.begin(), icn_indices_sorted.end(), [trueJet]( const int& icn1_idx, const int& icn2_idx ) {
					return abs(trueJet->pdg_icn_parent(icn1_idx)) == 25; // && trueJet->E_icn(icn1_idx) > trueJet->E_icn(icn2_idx); // need to have some sorting besides PDG=25
				});

				std::cerr << "ICN order: "; 
				for (int &i_icn: icn_indices_sorted)
					std::cerr << abs(pdg_icn_parent(i_icn)) << " ";
				std::cerr << std::endl;
				
				for (int &i_icn: icn_indices_sorted) {
					std::cerr << "PDG(icn)=" << abs(pdg_icn_parent(i_icn)) << std::endl;
					auto jet_ids = jets_of_initial_cn(i_icn);

					for (const int &icn_pdg: icn_pdgs) {
						if (abs(pdg_icn_parent(i_icn)) == icn_pdg) {
							IntVec icn_comps_pdgs = pdg_icn_comps(i_icn);
							bool is_dileptonic = icn_comps_pdgs.size() == 2 && (
								abs(icn_comps_pdgs[0]) == 11 ||
								abs(icn_comps_pdgs[0]) == 13 ||
								abs(icn_comps_pdgs[0]) == 15);

							if (is_dileptonic) {
								std::cerr << "Found dileptonic decay of boson type" << icn_pdg << " at evt_idx=" << m_nEvt << std::endl;
							} else {
								// check if hadronic
								short n_jets_from_icn = 0;

								for (const int& tj: jet_ids) {
									auto it = std::find(trueHadronicJetIndices.begin(), trueHadronicJetIndices.end(), tj);
									
									if (it != trueHadronicJetIndices.end()) {
										truejetpermICNs.push_back(std::distance(trueHadronicJetIndices.begin(), it));
										n_jets_from_icn += 1;
									}
								}
								
								if (n_jets_from_icn > 0) {
									//std::cerr << "Adding ICN PDG=" << icn_pdg << " to PermICNs at position " << (truejetpermICNs.size() / 2) << std::endl;
									if (n_jets_from_icn != 2 || truejetpermICNs.size() % 2 != 0) {
										//throw EVENT::Exception("Expected H/Z -> q + qbar decay");
										streamlog_out(ERROR) << "Expected H/Z -> q + qbar decay. Skipping event";

										// setting truejetpermICNs.size() to 0 will skip below loop
										truejetpermICNs = {};
									} else {
										m_trueDijetICNPDGs[truejetpermICNs.size() / 2 - 1] = icn_pdg;
									}
								}
							}
						}
					}
				}

				if (truejetpermICNs.size()) {
					if (truejetpermICNs.size() % 2 != 0)
						throw EVENT::Exception("Expected H/Z -> q + qbar decay");

					for (size_t i = 0; i < truejetpermICNs.size(); i++) {
						int index = trueHadronicJetIndices[truejetpermICNs[i]];
						const ReconstructedParticle* tjet = trueJet->jet(index);

						//m_trueJetMomenta[i] = v4(tjet);
						const double* v4_truejet = p4true(index);
						m_trueJetMomenta[i].SetPxPyPzE(v4_truejet[1], v4_truejet[2], v4_truejet[3], v4_truejet[0]);

						m_trueJetTypes[i] = type_jet(index);
						m_trueJetPDGs[i] = tjet->getParticleIDs()[0]->getPDG();
					}
				}
			}
		} else if ( (process_name == "e2e2hh" || process_name == "e2e2qqh") && (m_nAskedIsoLeps() == 2 && m_nAskedJets() == 4) ) {		
			/*
			THIS DOES NOT INCLUDE QUARK GLUON SPLITTINGS AND OTHER MORE COMPLICATED
			CASES. PROCEED WITH CAUTION WHEN USING TRUEJET (ABOVE) OR THIS APPROACH

			ONLY USED IN LEPTON CHANNEL
			*/

			LCCollection *mcParticles = pLCEvent->getCollection( "MCParticlesSkimmed" );
			
			IntVec fsIndices;
        	mcParticles->parameters().getIntVals("FINAL_STATE_PARTICLE_INDICES", fsIndices);

			// show Higgses first
			std::sort(fsIndices.begin(), fsIndices.end(), [mcParticles]( const int& mcp1_idx, const int& mcp2_idx ) {
				return abs( ((MCParticle*)mcParticles->getElementAt(mcp1_idx))->getPDG() ) == 25; // && trueJet->E_icn(icn1_idx) > trueJet->E_icn(icn2_idx); // need to have some sorting besides PDG=25
			});

			vector<MCParticle*> hadronicMCPs;
			vector<MCParticle*> leptonicMCPs;

			std::cerr << "fsParticle order: "; 
			for (int &i_part: fsIndices)
				std::cerr << ((MCParticle*)mcParticles->getElementAt(i_part))->getPDG() << " ";
			std::cerr << std::endl;

			for (size_t i = 0; i < fsIndices.size(); i++) {
				MCParticle* final_state_mcp = (MCParticle*)mcParticles->getElementAt(fsIndices[i]);
				int pdg = final_state_mcp->getPDG();
				m_trueJetTypes[i] = pdg;

				if (pdg == 25) {
					MCParticleVec Hdaughters = final_state_mcp->getDaughters();

					if (Hdaughters.size() == 2) {
						int decay1PDG = abs(Hdaughters[0]->getPDG());
						int decay2PDG = abs(Hdaughters[1]->getPDG());

						if (decay1PDG == decay2PDG) {
							if (decay1PDG <= 6) {
								//m_trueJetHiggsICNPairs.push_back(hadronicMCPs.size());
								hadronicMCPs.push_back(Hdaughters[0]);

								//m_trueJetHiggsICNPairs.push_back(hadronicMCPs.size());
								hadronicMCPs.push_back(Hdaughters[1]);
							} else if (decay1PDG >= 11 && decay1PDG <= 16) {
								//leptonicMCPs.push_back(Hdaughters[0]);
								//leptonicMCPs.push_back(Hdaughters[1]);
							}
						}
					}
				} else if (abs(pdg) <= 6) {
					hadronicMCPs.push_back(final_state_mcp);
				} else if (abs(pdg) >= 11 && abs(pdg) <= 16) {
					leptonicMCPs.push_back(final_state_mcp);
				} 
			}

			m_trueJetN = hadronicMCPs.size();
			m_trueLeptonN = leptonicMCPs.size();

			std::cerr << "Found nhadronicMCPs=" << m_trueJetN << " | nleptonicMCPs=" << m_trueLeptonN << std::endl;
			// populate four momenta and PDGs of jets and leptons
			if (m_trueJetN > m_nAskedJets() || m_trueLeptonN > m_nAskedIsoLeps())
				throw EVENT::Exception("Found more hadronic/leptonic MCPs than expected");
			
			
			for (unsigned short i = 0; i < m_trueJetN; i++) {
				m_trueJetMomenta[i] = v4(hadronicMCPs[i]);
				m_trueJetPDGs[i] = hadronicMCPs[i]->getPDG();

				if (i % 2 != 0) {
					std::cerr << "M(hadronic system " << ((i-1)/2 + 1) << ") = " << (m_trueJetMomenta[i] + m_trueJetMomenta[i - 1]).M() << std::endl;
				}
			}
			
			for (unsigned short i = 0; i < m_trueLeptonN; i++) {
				m_trueLeptonMomenta[i] = v4(leptonicMCPs[i]);
				m_trueLeptonPDGs[i] = leptonicMCPs[i]->getPDG();

				if (i % 2 != 0) {
					std::cerr << "M(leptonic system " << ((i-1)/2 + 1) << ") = " << (m_trueLeptonMomenta[i] + m_trueLeptonMomenta[i - 1]).M() << std::endl;
				}
			}

			/*
			
			float smallestSumCosAngle = getMatchingByAngularSpace(m_jets, hadronicMCPs, m_reco2TrueJetIndex, m_true2RecoJetIndex);

			// case in which number of hadronic jets == m_nJets
			m_trueRecoJetsMapped = smallestSumCosAngle < 99; */
		}
	}

	delall2();
};

void EventObservablesBase::getPermutationIndex(std::vector<int> input_perm, int size, short &perm_idx) {
	assert(size == 4);
	
	for (size_t i = 0; i < dijetPerms4.size(); i++) {
		if (dijetPerms4[i][0] == input_perm[0] &&
			dijetPerms4[i][1] == input_perm[1] &&
			dijetPerms4[i][2] == input_perm[2] &&
			dijetPerms4[i][3] == input_perm[3]) {
				perm_idx = i;
				return;
			}
	}
}

void EventObservablesBase::init(){
    printParameters();

	// A. prepare some variables
	if (m_polarizations.size() != 2)
		throw EVENT::Exception("Unexpected number of input jets");

	float Pem = m_polarizations[0];
	float Pep = m_polarizations[1];

	if (!(abs(Pem) <= 1 && abs(Pep) <= 1))
		throw EVENT::Exception("Unexpected number of input jets");

	// A.2. prepare the matrix elements
	#ifdef CALC_ME
	m_lcmezhh = new LCMEZHH("LCMEZHH", "ZHH", 125., Pem, Pep);
	m_lcmezzh = new LCMEZZH("LCMEZZH", "ZZH", 125., Pem, Pep);
	#endif

	// A.3. flavor tagging variables
	m_bTagValues = std::vector<double>(m_nAskedJets(), -1.);
  	m_cTagValues = std::vector<double>(m_nAskedJets(), -1.);

	m_use_tags2 = m_JetTaggingPIDAlgorithm2.size() && m_JetTaggingPIDParameterB2.size() && m_JetTaggingPIDParameterC2.size();

	m_bTagValues2 = std::vector<double>(m_nAskedJets(), -1.);
  	m_cTagValues2 = std::vector<double>(m_nAskedJets(), -1.);

	m_jets4v = std::vector<ROOT::Math::PxPyPzEVector>(m_nAskedJets());
	m_jetsMasses = std::vector<float>(m_nAskedJets());
	m_jetTags = std::vector<std::vector<float>>(m_nAskedJets(), std::vector<float>(m_JetTaggingPIDParameters.size(), 0));

	m_JMK_ZHH = std::vector<int>(m_nAskedJets(), 0);
	m_JMK_ZZH = std::vector<int>(m_nAskedJets(), 0);
	m_JMK_best = std::vector<int>(m_nAskedJets(), 0);

	// TrueJet
	m_reco2TrueJetIndex = std::vector<int>(m_nAskedJets(), -1);
	m_true2RecoJetIndex = std::vector<int>(m_nAskedJets(), -1);

	m_trueLeptonMomenta = std::vector<ROOT::Math::PxPyPzEVector>(m_nAskedIsoLeps());
	m_trueLeptonPDGs = std::vector<int>(m_nAskedIsoLeps(), 0);

	m_trueJetMomenta = std::vector<ROOT::Math::PxPyPzEVector>(m_nAskedJets());
	m_trueJetTypes = std::vector<int>(m_nAskedJets(), 0);
	m_trueJetPDGs = std::vector<int>(m_nAskedJets(), 0);
	m_trueDijetICNPDGs = std::vector<int>(m_nAskedJets() / 2, 0);

	m_trueISRMomenta = std::vector<ROOT::Math::PxPyPzEVector>(2);

	prepareBaseTree();
	prepareChannelTree();

	clearBaseValues();
	clearChannelValues();
};

void EventObservablesBase::processEvent( LCEvent* evt ){
	streamlog_out(DEBUG) << "START updateBaseValues" << std::endl;
	updateBaseValues(evt);
	streamlog_out(DEBUG) << "END updateBaseValues -> status code " << m_statusCode << std::endl;

	// all channel specific processors require that at least acquiring the base values succeeds
	if (m_statusCode == 0) {
		streamlog_out(DEBUG) << "START updateChannelValues" << std::endl;
		calculateSimpleZHHChi2();
		updateChannelValues(evt);
		streamlog_out(DEBUG) << "END updateChannelValues" << std::endl;
	} else
		streamlog_out(MESSAGE) << "skipping updateChannelValues due to non-zero state " << m_statusCode << std::endl;

	if (m_write_ttree)
		getTTree()->Fill();

	clearBaseValues();
	clearChannelValues();
	streamlog_out(MESSAGE) << "cleared. event " << m_nEvt << " processed" << std::endl;
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
	const std::vector<ROOT::Math::PxPyPzEVector> v4_jets,
	std::vector<float> target_masses,
	std::vector<float> target_resolutions,
	std::function<float (
		const std::vector<ROOT::Math::PxPyPzEVector>,
		const std::vector<unsigned short>,
		std::vector<float>&,
		const std::vector<float>,
		const std::vector<float>)> calc_chi2
) {
	const std::vector<std::vector<unsigned short>> *perms;

	if (v4_jets.size() == 4) {
		perms = &dijetPerms4;
	} else if (v4_jets.size() == 6) {
		perms = &dijetPerms6;
	} else
		throw EVENT::Exception("Not implemented dijet pairing case");

	size_t nperm = perms->size();
	unsigned int best_idx = 0;
	float chi2min = 999999.;

	std::vector<float> masses_temp (target_masses.size(), -999);
	std::vector<float> masses (target_masses.size(), -999);
	
	float chi2;
	for (size_t i=0; i < nperm; i++) {
		chi2 = calc_chi2(v4_jets, perms->at(i), masses_temp, target_masses, target_resolutions);

		if (chi2 < chi2min) {
			chi2min = chi2;
			masses = masses_temp;
			best_idx = i;
		}
	}
	
	return { perms->at(best_idx), masses, chi2min };
};

std::tuple<std::vector<unsigned short>, vector<float>, float> EventObservablesBase::pairJetsByMass(
	std::vector<ReconstructedParticle*> jets,
	std::vector<unsigned short> dijet_targets
) {
	return EventObservablesBase::pairJetsByMass(v4(jets), dijet_targets);
}

std::tuple<std::vector<unsigned short>, vector<float>, float> EventObservablesBase::pairJetsByMass(
	std::vector<ROOT::Math::PxPyPzEVector> jets,
	std::vector<unsigned short> dijet_targets
) {
	assert(dijet_targets.size() *2 == jets.size());
	
	std::vector<float> target_masses (dijet_targets.size(), -999);
	std::vector<float> target_resolutions (dijet_targets.size(), -999);

	for (size_t j=0; j < target_masses.size(); j++) {
		switch (dijet_targets[j]) {
			case  6: target_masses[j] = kMassTop; target_resolutions[j] = kSigmaMassTop; break;
			case 23: target_masses[j] = kMassZ  ; target_resolutions[j] = kSigmaMassZ; break;
			case 24: target_masses[j] = kMassW  ; target_resolutions[j] = kSigmaMassW; break;
			case 25: target_masses[j] = kMassH  ; target_resolutions[j] = kSigmaMassH; break;
			
			default: throw EVENT::Exception("Not implemented dijet pairing case");
		}
	}

	return pairJetsByMass(jets, target_masses, target_resolutions, [](
		const std::vector<ROOT::Math::PxPyPzEVector> jets_in,
		const std::vector<unsigned short> perm,
		std::vector<float> &masses,
		std::vector<float> targets,
		std::vector<float> resolutions)
		{
			float result = 0;
			
			for (size_t i=0; i < masses.size(); i++) {
				masses[i] = inv_mass(jets_in[perm[i*2]], jets_in[perm[i*2 + 1]]);
				result += std::pow((masses[i] - targets[i])/resolutions[i], 2);
			}

			return result;
		}
	);
};

ReconstructedParticleVec EventObservablesBase::getElements(LCCollection *collection, std::vector<int> elements) {
	ReconstructedParticleVec vec;
	
	for (size_t i=0; i < elements.size(); i++) {
		ReconstructedParticle* p = dynamic_cast<ReconstructedParticle*>(collection->getElementAt(elements[i]));
		vec.push_back(p);
	}
	
	return vec;
};

#ifdef CALC_ME
// Hint: For debugging and flexibility reasons, the matrix element will be calculated post using Python code
// see zhh/mem/Physsim
void EventObservablesBase::calculateMatrixElements(
	int b1_decay_pdg,
	int b2_decay_pdg,
	TLorentzVector from_z_1, TLorentzVector from_z_2, // from z
	TLorentzVector jet1, TLorentzVector jet2, // from z or h
	TLorentzVector jet3, TLorentzVector jet4, // from h
	bool permute_from_z
){
	unsigned short nperms = 6;
	float default_weight = 1./nperms;
	std::vector<float> weights (nperms, default_weight);

	return calculateMatrixElements(b1_decay_pdg, b2_decay_pdg, from_z_1, from_z_2, jet1, jet2, jet3, jet4, permute_from_z, nperms, weights);
}

void EventObservablesBase::calculateMatrixElements(
	int b1_decay_pdg,
	int b2_decay_pdg,
	TLorentzVector from_z_1, TLorentzVector from_z_2, // from z
	TLorentzVector jet1, TLorentzVector jet2, // from z or h
	TLorentzVector jet3, TLorentzVector jet4, // from h
	bool permute_from_z,
	unsigned short nperms,
	std::vector<float> weights
){
	m_lcmezhh->SetZDecayMode(m_pdg_to_lcme_mode[b1_decay_pdg]);
	m_lcmezzh->SetZDecayMode(m_pdg_to_lcme_mode[b1_decay_pdg], m_pdg_to_lcme_mode[b2_decay_pdg]);

	TLorentzVector vecs_jet [4] = { jet1, jet2, jet3, jet4 };

	double result_zhh = 0.;
	double result_zzh = 0.;
	m_lcme_weights = weights;

	streamlog_out(DEBUG) << "LCME Debug: E, Px, Py, Pz" << std::endl
						 << "  Z_1: " << from_z_1.E() << " " << from_z_1.Px() << " " << from_z_1.Py() << " " << from_z_1.Pz() << std::endl
						 << "  Z_2: " << from_z_2.E() << " " << from_z_2.Px() << " " << from_z_2.Py() << " " << from_z_2.Pz() << std::endl
						 << "  J_1: " << vecs_jet[0].E() << " " << vecs_jet[0].Px() << " " << vecs_jet[0].Py() << " " << vecs_jet[0].Pz() << std::endl
						 << "  J_2: " << vecs_jet[1].E() << " " << vecs_jet[1].Px() << " " << vecs_jet[1].Py() << " " << vecs_jet[1].Pz() << std::endl
						 << "  J_3: " << vecs_jet[2].E() << " " << vecs_jet[2].Px() << " " << vecs_jet[2].Py() << " " << vecs_jet[2].Pz() << std::endl
						 << "  J_4: " << vecs_jet[3].E() << " " << vecs_jet[3].Px() << " " << vecs_jet[3].Py() << " " << vecs_jet[3].Pz() << std::endl;

	/*
	std::cout << "  TOT: " << from_z_1.E() + from_z_2.E() + vecs_jet[0].E() + vecs_jet[1].E() + vecs_jet[2].E() + vecs_jet[3].E() << " "
							<< from_z_1.Px() + from_z_2.Px() + vecs_jet[0].Px() + vecs_jet[1].Px() + vecs_jet[2].Px() + vecs_jet[3].Px() << " "
							<< from_z_1.Py() + from_z_2.Py() + vecs_jet[0].Py() + vecs_jet[1].Py() + vecs_jet[2].Py() + vecs_jet[3].Py() << " "
							<< from_z_1.Pz() + from_z_2.Pz() + vecs_jet[0].Pz() + vecs_jet[1].Pz() + vecs_jet[2].Pz() + vecs_jet[3].Pz()
							<< std::endl;
	*/

	std::vector<std::vector<unsigned short>> perms_from_z = {{ 0, 1 }};
	if (permute_from_z) {
		throw EVENT::Exception("permute_from_z=true encountered. Removed.");
		perms_from_z.push_back({1, 0});
		nperms = nperms * 2;
	}
	assert(nperms == dijetPerms4.size());
	TLorentzVector vecs_from_z [2] = { from_z_1, from_z_2 };

	// TODO: only calculating 12 is necessary, as H -> a+b is symmetric with respect to a <-> b

	int nperm = 0;
	double lcme_zhh = 0.;
	double lcme_zzh = 0.;

	streamlog_out(DEBUG) << "ME^2 (ZHH; ZZH) =" << std::endl;
	for (auto perm_from_z: perms_from_z) {
		for (size_t perm_idx = 0; perm_idx < dijetPerms4.size(); perm_idx++) {
			if (weights[nperm] != 0) {
				TLorentzVector zhh_inputs [4] = { vecs_from_z[perm_from_z[0]], vecs_from_z[perm_from_z[1]], vecs_jet[dijetPerms4[perm_idx][0]] + vecs_jet[dijetPerms4[perm_idx][1]], vecs_jet[dijetPerms4[perm_idx][2]] + vecs_jet[dijetPerms4[perm_idx][3]] };
				TLorentzVector zzh_inputs [5] = { vecs_from_z[perm_from_z[0]], vecs_from_z[perm_from_z[1]], vecs_jet[dijetPerms4[perm_idx][0]] , vecs_jet[dijetPerms4[perm_idx][1]], vecs_jet[dijetPerms4[perm_idx][2]] + vecs_jet[dijetPerms4[perm_idx][3]] };

				m_lcmezhh->SetMomentumFinal(zhh_inputs);
				m_lcmezzh->SetMomentumFinal(zzh_inputs);

				lcme_zhh = m_lcmezhh->GetMatrixElement2();
				lcme_zzh = m_lcmezzh->GetMatrixElement2();

				m_lcme_zhh_raw.push_back(lcme_zhh);
				m_lcme_zzh_raw.push_back(lcme_zzh);
				
				streamlog_out(DEBUG) << " Perm " << (nperm+1) << "/" <<nperms  << ": (" << lcme_zhh << "; " << lcme_zzh << ") | wt=" << weights[nperm] << std::endl;

				result_zhh += lcme_zhh * weights[nperm];
				result_zzh += lcme_zzh * weights[nperm];
			}

			nperm += 1;
		}
	}

	//m_lcme_zhh_log = std::log(result_zhh);
	//m_lcme_zzh_log = std::log(result_zzh);

	//if (m_JMK_ZHH_perm_idx != -1)
	//	m_lcme_jmk_zhh_log = std::log(m_lcme_zhh_raw[m_JMK_ZHH_perm_idx]);

	//if (m_JMK_ZZH_perm_idx != -1)
	//	m_lcme_jmk_zzh_log = std::log(m_lcme_zzh_raw[m_JMK_ZZH_perm_idx]);

	streamlog_out(MESSAGE) << " log(LCMEZHH)=" << m_lcme_zhh_log << std::endl;
	streamlog_out(MESSAGE) << " log(LCMEZZH)=" << m_lcme_zzh_log << std::endl;
};
#endif

std::tuple<int, int, int> EventObservablesBase::nPFOsMinMax(LCCollection *collection) {
	int min_res = 999;
	int max_res = 0;
	int min_idx = -1;
	int n_current;

	for (int i=0; i < collection->getNumberOfElements(); i++) {
		ReconstructedParticle* jet = dynamic_cast<ReconstructedParticle*>(collection->getElementAt(i));
		n_current = jet->getParticles().size();

		if (n_current < min_res) {
			min_res = n_current;
			min_idx = i;
		}

		max_res = std::max(max_res, n_current);
	}

	return std::make_tuple(min_res, max_res, min_idx);
};

std::tuple<float, float> EventObservablesBase::jetCharge(ReconstructedParticle* jet) {
	float charge = 0;
	float dynamic_charge = 0;
	float jet_energy = jet->getEnergy();
	float jet_momentum = v4(jet).P();
	float z_i;

	EVENT::ReconstructedParticleVec jetConstituents = jet->getParticles();
	
	for (size_t i=0; i < jetConstituents.size(); i++) {
		ReconstructedParticle* pfo = dynamic_cast<ReconstructedParticle*>(jetConstituents[i]);

		// only consider hadronic pfos
		//if (abs(pfo->getType()) > 100) {
		if (pfo->getCharge() != 0) {
			z_i = pfo->getEnergy() / jet_energy;

			charge += std::pow(z_i, m_jetChargeKappa) * pfo->getCharge();

			// also calculate the dynamic jet charge, as per https://arxiv.org/pdf/2101.04304
			dynamic_charge += std::pow(z_i, v4(pfo).P()/jet_momentum) * pfo->getCharge();
		}
	}

	return std::make_tuple(charge, dynamic_charge);
}

void EventObservablesBase::setJetCharges() {
	std::tie(m_jet1_q, m_jet1_qdyn) = jetCharge(m_jets[0]);
	std::tie(m_jet2_q, m_jet2_qdyn) = jetCharge(m_jets[1]);
	std::tie(m_jet3_q, m_jet3_qdyn) = jetCharge(m_jets[2]);
	std::tie(m_jet4_q, m_jet4_qdyn) = jetCharge(m_jets[3]);
};

float EventObservablesBase::leadingMomentum(ReconstructedParticleVec jets) {
	float result = 0;

	for (size_t i=0; i < jets.size(); i++) {
		result = std::max(result, (float)v4(jets[i]).Pt());
	}

	return result;
};

std::vector<std::pair<int, float>> EventObservablesBase::sortedTagging(std::vector<float> tags_by_jet_order) {
	std::vector<std::pair<int, float>> result;

	for (size_t i = 0; i < tags_by_jet_order.size(); i++) {
		result.push_back(std::make_pair(i, tags_by_jet_order[i]));
	}

	std::sort(result.begin(), result.end(), jetTaggingComparator);

	return result;
};

std::vector<std::pair<int, float>> EventObservablesBase::sortedTagging(LCCollection* collection, std::string pid_algorithm, std::string pid_parameter_b) {
	PIDHandler pidh(collection);
	int algo_id = pidh.getAlgorithmID(pid_algorithm);
	int btag_id = pidh.getParameterIndex(algo_id, pid_parameter_b);

	std::vector<float> tags_by_jet_order;

	for (int i=0; i < collection->getNumberOfElements(); i++) {
		const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(pidh.getParticleID((ReconstructedParticle*)collection->getElementAt(i), algo_id));
		const FloatVec& FTPara = FTImpl.getParameters();

		tags_by_jet_order.push_back(FTPara[btag_id]);
	}

	return sortedTagging(tags_by_jet_order);
};

float EventObservablesBase::getMatchingByAngularSpace(
	vector<EVENT::ReconstructedParticle*> recoJets,
	vector<int> &reco2truejetindex, // reco jet index -> truejet index
	vector<int> &true2recojetindex, // truejet index -> reco jet index
	vector<int> &trueHadronicJetIndices,
	vector<int> &trueLeptonIndices,
	vector<int> &trueISRIndices )
{
	int njets = this->njets();

	for (int i_jet = 0 ; i_jet < njets ; i_jet++ ) {
		streamlog_out(DEBUG) << "type of jet " << i_jet << ": " << type_jet( i_jet ) << endl;   
		switch (type_jet(i_jet)) {
			case 1:
			case 3: trueHadronicJetIndices.push_back( i_jet ); break;
			case 2: trueLeptonIndices.push_back( i_jet ); break;
			case 4: trueISRIndices.push_back( i_jet ); break;
			default:
				// do nothing
				break;
		}
	}

	if (trueHadronicJetIndices.size() != m_nJets)
		return 999;

	vector<int> arr(m_nJets);
	vector<TVector3> trueJetUnitVectors(m_nJets);
	vector<TVector3> recoJetUnitVectors(m_nJets);

	for (unsigned int i_array = 0; i_array < m_nJets; i_array++) {
		arr[i_array] = i_array;

		TVector3 trueJetMomentumUnit(ptrueseen(trueHadronicJetIndices[i_array])[0], ptrueseen(trueHadronicJetIndices[i_array])[1], ptrueseen(trueHadronicJetIndices[i_array])[2]);
		trueJetMomentumUnit.SetMag(1.0);
		trueJetUnitVectors[i_array] = trueJetMomentumUnit;

		TVector3 recoJetMomentumUnit(recoJets.at(arr[i_array])->getMomentum());
		recoJetMomentumUnit.SetMag(1.0);
		recoJetUnitVectors[i_array] = recoJetMomentumUnit;
	}

	float SmallestSumCosAngle = 99999.0;
	vector<int> matchedRecoJetIndices(m_nJets);
	do {
		float sumcosangle = 0.0;

		for (unsigned int i_Jet = 0 ; i_Jet < m_nJets; i_Jet++ )
			sumcosangle += acos(trueJetUnitVectors[i_Jet].Dot( recoJetUnitVectors[arr[i_Jet]] ));

		if (sumcosangle < SmallestSumCosAngle) {
			SmallestSumCosAngle = sumcosangle;

			for (unsigned int i_array = 0; i_array < m_nJets; i_array++)
				matchedRecoJetIndices[i_array] = arr[i_array];
		}
	} while (next_permutation(arr.begin(), arr.begin() + m_nJets));

	reco2truejetindex = matchedRecoJetIndices;

	for (unsigned int i_jet = 0; i_jet < m_nJets; i_jet++)
		true2recojetindex[reco2truejetindex[i_jet]] = i_jet;
	
	return SmallestSumCosAngle;
}

float EventObservablesBase::getMatchingByAngularSpace(
	vector<EVENT::ReconstructedParticle*> recoJets,
	vector<EVENT::MCParticle*> quarkMCParticles,
	vector<int> &reco2MCPindex,
	vector<int> &true2MCPindex
)
{
	if (quarkMCParticles.size() != m_nJets)
		return 999;

	vector<int> arr(m_nJets);
	vector<TVector3> trueUnitVectors(m_nJets);
	vector<TVector3> recoUnitVectors(m_nJets);

	for (unsigned int i_array = 0; i_array < m_nJets; i_array++) {
		arr[i_array] = i_array;

		TVector3 trueMomentumUnit = quarkMCParticles[i_array]->getMomentum();
		trueMomentumUnit.SetMag(1.0);
		trueUnitVectors[i_array] = trueMomentumUnit;

		TVector3 recoMomentumUnit(recoJets.at(i_array)->getMomentum());
		recoMomentumUnit.SetMag(1.0);
		recoUnitVectors[i_array] = recoMomentumUnit;
	}

	float SmallestSumCosAngle = 99999.0;
	vector<int> matchedRecoJetIndices(m_nJets);
	do {
		float sumcosangle = 0.0;

		for (unsigned int i_Jet = 0 ; i_Jet < m_nJets; i_Jet++ )
			sumcosangle += acos(trueUnitVectors[i_Jet].Dot( recoUnitVectors[arr[i_Jet]] ));

		if (sumcosangle < SmallestSumCosAngle) {
			SmallestSumCosAngle = sumcosangle;

			for (unsigned int i_array = 0; i_array < m_nJets; i_array++)
				matchedRecoJetIndices[i_array] = arr[i_array];
		}
	} while (next_permutation(arr.begin(), arr.begin() + m_nJets));

	reco2MCPindex = matchedRecoJetIndices;

	for (unsigned int i_jet = 0; i_jet < m_nJets; i_jet++)
		true2MCPindex[reco2MCPindex[i_jet]] = i_jet;
	
	return SmallestSumCosAngle;
}
