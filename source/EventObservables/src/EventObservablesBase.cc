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
	{0,1,2,3},
	{0,2,1,3},
	{0,3,1,2},
	{1,2,0,3},
	{1,3,0,2},
	{2,3,0,1}
};

const std::vector<std::vector<unsigned short>> EventObservablesBase::dijetPerms6 {
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
			"inputPfoRawCollection",
			"Name of input pfo collection",
			m_inputPfoRawCollection,
			std::string("PandoraPFOsWithoutOverlay")
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
            std::string("weaver")
            );

	registerProcessorParameter("JetTaggingPIDParameterB",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterB,
            std::string("mc_b")
            );

	registerProcessorParameter("JetTaggingPIDParameterC",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterC,
            std::string("mc_c")
            );

	registerProcessorParameter("JetTaggingPIDAlgorithm2",
            "Number of jet should be in the event",
            m_JetTaggingPIDAlgorithm2,
            std::string("lcfiplus")
            );

	registerProcessorParameter("JetTaggingPIDParameterB2",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameterB2,
            std::string("BTag")
            );

	registerProcessorParameter("JetTaggingPIDParameterC2",
            "Number of jet should be in the event",
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
		ttree->Branch("errorCode", &m_statusCode, "errorCode/I");

		ttree->Branch("evis", &m_Evis, "evis/F");
		ttree->Branch("m_miss", &m_missingMass, "m_miss/F");
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

		ttree->Branch("njets",&m_nJets,"njets/I");
		ttree->Branch("nisoleptons",&m_nIsoLeps,"nisoleptons/I");
		ttree->Branch("npfos", &m_npfos, "npfos/I");
		ttree->Branch("lep_types", &m_lep_types);

		//ttree->Branch("mh1", &m_mh1, "mh1/F");
		//ttree->Branch("mh2", &m_mh2, "mh2/F");
		//ttree->Branch("mhh", &m_mhh, "mhh/F");

		ttree->Branch("yminus", &m_yMinus, "yminus/F");
		ttree->Branch("yplus", &m_yPlus, "yplus/F");

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
		ttree->Branch("pxj1", &m_pxj1, "pxj1/F");
		ttree->Branch("pyj1", &m_pyj1, "pyj1/F");
		ttree->Branch("pzj1", &m_pzj1, "pzj1/F");
		ttree->Branch("ej1", &m_ej1, "ej1/F");

		ttree->Branch("pxj2", &m_pxj2, "pxj2/F");
		ttree->Branch("pyj2", &m_pyj2, "pyj2/F");
		ttree->Branch("pzj2", &m_pzj2, "pzj2/F");
		ttree->Branch("ej2", &m_ej2, "ej2/F");

		ttree->Branch("pxj3", &m_pxj3, "pxj3/F");
		ttree->Branch("pyj3", &m_pyj3, "pyj3/F");
		ttree->Branch("pzj3", &m_pzj3, "pzj3/F");
		ttree->Branch("ej3", &m_ej3, "ej3/F");

		ttree->Branch("pxj4", &m_pxj4, "pxj4/F");
		ttree->Branch("pyj4", &m_pyj4, "pyj4/F");
		ttree->Branch("pzj4", &m_pzj4, "pzj4/F");
		ttree->Branch("ej4", &m_ej4, "ej4/F");

		// jet matching
		//ttree->Branch("jet_matching", &m_jet_matching);
		//ttree->Branch("jet_matching_source", &m_jet_matching_source, "jet_matching_source/I");
		//ttree->Branch("using_kinfit", &m_using_kinfit, "using_kinfit/B");
		//ttree->Branch("using_mass_chi2", &m_using_mass_chi2, "using_mass_chi2/B");

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

	m_statusCode = 0;
	
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
	m_npfos = 0;
	m_lep_types.clear();

	m_Evis  = -999.;
	m_missingMass = -999.;

	m_yMinus = 0.;
	m_yPlus = 0.;
	
	// flavor tagging
	m_bTagsSorted.clear();
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

	m_bmax1 = 0.;
	m_bmax2 = 0.;
	m_bmax3 = 0.;
	m_bmax4 = 0.;

	m_cmax1 = 0.;
	m_cmax2 = 0.;
	m_cmax3 = 0.;
	m_cmax4 = 0.;

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

	m_bmax12 = 0.;
	m_bmax22 = 0.;
	m_bmax32 = 0.;
	m_bmax42 = 0.;

	m_cmax12 = 0.;
	m_cmax22 = 0.;
	m_cmax32 = 0.;
	m_cmax42 = 0.;

	//m_jet_matching.clear();
	//m_jet_matching_source = 0;
	//m_using_kinfit = false;
	//m_using_mass_chi2 = false;

	m_jets.clear();
	
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
	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();

	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

	try {
		LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // main jet collection
		LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
		LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
		LCCollection *inputPfoCollection = pLCEvent->getCollection( m_inputPfoCollection );
		LCCollection *inputPfoRawCollection = pLCEvent->getCollection( m_inputPfoRawCollection );

		m_nJets = inputJetCollection->getNumberOfElements();
		if (m_nJets != m_nAskedJets())
			throw EVENT::Exception("Unexpected number of input jets");

		// ---------- NUMBER OF ISOLATED LEPTONS and PFOs ----------
		m_nIsoLeps = inputLeptonCollection->getNumberOfElements();
		m_npfos = inputPfoCollection->getNumberOfElements();

		// ---------- MISSING PT ----------
		// corrected for crossing angle
		float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
		double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);

		ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
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
		ROOT::Math::PxPyPzEVector pmis = ecms - pfosum;
		
		m_missingPT = pmis.Pt();
		m_missingMass = pmis.M();
		m_missingE = pmis.E();
		
		// ---------- VISIBLE ENERGY ----------                                          
		m_Evis = pfosum.E();

		// ---------- THRUST ----------
		const EVENT::LCParameters& raw_pfo_params = inputPfoRawCollection->getParameters();

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

			// ---------- JET MATCHING ----------
			/*
			(void) inputJetCollection->getParameters().getIntVals(m_jetMatchingParameter().c_str(), m_jet_matching);
			m_jet_matching_source = inputJetCollection->getParameters().getIntVal(m_jetMatchingSourceParameter());

			assert(m_jet_matching.size() == m_nJets);
			assert(m_jet_matching_source == 1 || m_jet_matching_source == 2);

			m_using_kinfit = m_jet_matching_source == 2;
			m_using_mass_chi2 = m_jet_matching_source == 1;

			streamlog_out(MESSAGE) << "Jet matching from source " << m_jet_matching_source << " : (" << m_jet_matching[0] << "," << m_jet_matching[1] << ") + (" << m_jet_matching[2] << "," << m_jet_matching[3] << ")" << std::endl;
			*/

			// ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
			for (int i=0; i < m_nJets; ++i) {
				ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
				ROOT::Math::PxPyPzEVector jet_v4 = v4(jet);
				
				float currentJetMomentum = jet_v4.P();

				if (currentJetMomentum > m_pjmax)
					m_cosjmax = jet_v4.Pz() / currentJetMomentum;

				m_ptjmax = std::max(m_ptjmax, (float)jet_v4.Pt());
				m_pjmax = std::max(m_pjmax, currentJetMomentum);
				m_jets.push_back(jet);
			}

			PIDHandler jetPIDh(inputJetCollection);
			int _FTAlgoID = jetPIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm);
			int _FTAlgoID2 = m_use_tags2 ? jetPIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm2) : -1;

			int BTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
			int CTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterC);

			int BTagID2 = m_use_tags2 ? jetPIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterB2) : -1;
			int CTagID2 = m_use_tags2 ? jetPIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterC2) : -1;
			//int OTagID = jetPIDh.getParameterIndex(_FTAlgoID, "OTag");

			streamlog_out(MESSAGE) << "--- FLAVOR TAG ---" << std::endl;
			for (int i=0; i<m_nJets; ++i) {
				const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(jetPIDh.getParticleID(m_jets[i], _FTAlgoID));
				const FloatVec& FTPara = FTImpl.getParameters();

				streamlog_out(MESSAGE) << "Jet " << i << std::endl << " > Algo1 - B,C = " << FTPara[BTagID] << "," << FTPara[CTagID] << std::endl;
				//double oTagValue = FTPara[OTagID];

				m_bTagValues[i] = FTPara[BTagID];
				m_cTagValues[i] = FTPara[CTagID];

				if (m_use_tags2) {
					const ParticleIDImpl& FTImpl2 = dynamic_cast<const ParticleIDImpl&>(jetPIDh.getParticleID(m_jets[i], _FTAlgoID2));
					const FloatVec& FTPara2 = FTImpl2.getParameters();

					m_bTagValues2[i] = FTPara2[BTagID2];
					m_cTagValues2[i] = FTPara2[CTagID2];

					streamlog_out(MESSAGE) << " > Algo2 - B,C = " << FTPara2[BTagID2] << "," << FTPara2[CTagID2] << std::endl;
					//double oTagValue = FTPara[OTagID];
				}
			}

			// calculate bmax1,2,3,4
			for (size_t i = 0; i < m_bTagValues.size(); i++) {
				m_bTagsSorted.push_back(std::make_pair(i, m_bTagValues[i]));
			}

			std::sort (m_bTagsSorted.begin(), m_bTagsSorted.end(), jetTaggingComparator);

			TVector3 pjbmaxA1 (m_jets[m_bTagsSorted[0].first]->getMomentum());
			TVector3 pjbmaxA2 (m_jets[m_bTagsSorted[1].first]->getMomentum());
			m_cosbmax = pjbmaxA1.Dot(pjbmaxA2)/pjbmaxA1.Mag()/pjbmaxA2.Mag();

			m_bmax1 = m_bTagsSorted[0].second;
			m_bmax2 = m_bTagsSorted[1].second;
			m_bmax3 = m_bTagsSorted[2].second;
			m_bmax4 = m_bTagsSorted[3].second;

			std::vector<double> cTagsSorted(m_cTagValues.begin(), m_cTagValues.end());
			std::sort (cTagsSorted.begin(), cTagsSorted.end());

			m_cmax1 = cTagsSorted[0];
			m_cmax2 = cTagsSorted[1];
			m_cmax3 = cTagsSorted[2];
			m_cmax4 = cTagsSorted[3];

			if (m_use_tags2) {
				std::vector<double> bTagsSorted2(m_bTagValues2.begin(), m_bTagValues2.end());
				std::sort (bTagsSorted2.begin(), bTagsSorted2.end());

				m_bmax12 = bTagsSorted2[0];
				m_bmax22 = bTagsSorted2[1];
				m_bmax32 = bTagsSorted2[2];
				m_bmax42 = bTagsSorted2[3];

				std::vector<double> cTagsSorted2(m_cTagValues2.begin(), m_cTagValues2.end());
				std::sort (cTagsSorted2.begin(), cTagsSorted2.end());

				m_cmax12 = cTagsSorted2[0];
				m_cmax22 = cTagsSorted2[1];
				m_cmax32 = cTagsSorted2[2];
				m_cmax42 = cTagsSorted2[3];
			}

			// ---------- YMINUS, YPLUS ----------        
			int algo_y = jetPIDh.getAlgorithmID("yth");
			const ParticleID & ythID = jetPIDh.getParticleID(m_jets[0], algo_y); // same arguments for all jets
	
			FloatVec params_y = ythID.getParameters();
			m_yMinus = params_y[jetPIDh.getParameterIndex(algo_y, m_yMinusParameter())];
			m_yPlus = params_y[jetPIDh.getParameterIndex(algo_y, m_yPlusParameter())];

			if ( inputLepPairCollection->getNumberOfElements() != m_nAskedIsoLeps() ) {
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
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
		m_statusCode = 20;
	} catch (const std::exception &exc) {
		// remove for production
    	streamlog_out(MESSAGE) << exc.what();
		m_statusCode = 30;
	}
};

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

	clearBaseValues();
	clearChannelValues();
};

void EventObservablesBase::processEvent( LCEvent* evt ){
	streamlog_out(MESSAGE) << "START updateBaseValues";
	updateBaseValues(evt);
	streamlog_out(MESSAGE) << "END updateBaseValues -> status code " << m_statusCode << std::endl;

	// all channel specific processors require that at least acquiring the base values succeeds
	if (m_statusCode == 0) {
		streamlog_out(MESSAGE) << "START updateChannelValues" << std::endl;
		updateChannelValues(evt);
		streamlog_out(MESSAGE) << "END updateChannelValues" << std::endl;
	} else
		streamlog_out(MESSAGE) << "skipping updateChannelValues due to non-zero state " << m_statusCode << std::endl;

	if (m_write_ttree)
		getTTree()->Fill();

	streamlog_out(MESSAGE) << "clearing values... ";
	clearBaseValues();
	clearChannelValues();
	streamlog_out(MESSAGE) << "done. event " << m_nEvt << " processed" << std::endl;
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

	return pairJetsByMass(toFourVectors(jets), target_masses, target_resolutions, [](
		const std::vector<ROOT::Math::PxPyPzEVector> jets,
		const std::vector<unsigned short> perm,
		std::vector<float> &masses,
		std::vector<float> targets,
		std::vector<float> resolutions)
		{
			float result = 0;
			
			for (size_t i=0; i < masses.size(); i++) {
				masses[i] = inv_mass(jets[perm[i*2]], jets[perm[i*2 + 1]]);
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

void EventObservablesBase::setJetMomenta() {
	m_pxj1 = m_jets[0]->getMomentum()[0];
	m_pyj1 = m_jets[0]->getMomentum()[1];
	m_pzj1 = m_jets[0]->getMomentum()[2];
	m_ej1  = m_jets[0]->getEnergy();

	m_pxj2 = m_jets[1]->getMomentum()[0];
	m_pyj2 = m_jets[1]->getMomentum()[1];
	m_pzj2 = m_jets[1]->getMomentum()[2];
	m_ej2  = m_jets[1]->getEnergy();

	m_pxj3 = m_jets[2]->getMomentum()[0];
	m_pyj3 = m_jets[2]->getMomentum()[1];
	m_pzj3 = m_jets[2]->getMomentum()[2];
	m_ej3  = m_jets[2]->getEnergy();

	m_pxj4 = m_jets[3]->getMomentum()[0];
	m_pyj4 = m_jets[3]->getMomentum()[1];
	m_pzj4 = m_jets[3]->getMomentum()[2];
	m_ej4  = m_jets[3]->getEnergy();
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