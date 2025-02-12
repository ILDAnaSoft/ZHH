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

template<class T>
TLorentzVector v4(T* p){
  return TLorentzVector( p->getMomentum()[0],p->getMomentum()[1], p->getMomentum()[2],p->getEnergy());
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

		// bmax1:bmax2:bmax3:bmax4:pj1jets2:pj2jets2
		ttree->Branch("bmax1", &m_bmax1, "bmax1/F");
		ttree->Branch("bmax2", &m_bmax2, "bmax2/F");
		ttree->Branch("bmax3", &m_bmax3, "bmax3/F");
		ttree->Branch("bmax4", &m_bmax4, "bmax4/F");

		ttree->Branch("cmax1", &m_cmax1, "cmax1/F");
		ttree->Branch("cmax2", &m_cmax2, "cmax2/F");
		ttree->Branch("cmax3", &m_cmax3, "cmax3/F");
		ttree->Branch("cmax4", &m_cmax4, "cmax4/F");

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

	m_jet_matching.clear();
	m_jet_matching_source = 0;
	
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

			std::cerr << "VEC SIZE of "<< m_jetMatchingParameter().c_str() << " : " << m_jet_matching.size() << std::endl;
			std::cerr << std::string("TEST_").append("STRING") << std::endl;
			std::cerr << "Reading parameters " << m_jet_matching[0] << " " << m_jet_matching[1] << " " << m_jet_matching[2] << " " << m_jet_matching[3] << std::endl;

			// ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
		
			vector<ReconstructedParticle*> jets;
			for (int i=0; i<m_nJets; ++i) {
				ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
				jets.push_back(jet);
			}

			PIDHandler FTHan(inputJetCollection);
			int _FTAlgoID = FTHan.getAlgorithmID(m_JetTaggingPIDAlgorithm);
			int BTagID = FTHan.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
			int CTagID = FTHan.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterC);
			//int OTagID = FTHan.getParameterIndex(_FTAlgoID, "OTag");

			for (int i=0; i<m_nJets; ++i) {
				const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(FTHan.getParticleID(jets[i], _FTAlgoID));
				const FloatVec& FTPara = FTImpl.getParameters();
				std::cerr << "Reading parameters " << BTagID << " " << CTagID << " of vector with " << FTPara.size() << " elements at " << &FTPara << std::endl;
				double bTagValue = FTPara[BTagID];
				double cTagValue = FTPara[CTagID];
				//double oTagValue = FTPara[OTagID];

				std::cerr << "Read parameters " << bTagValue << " and " << cTagValue << std::endl;

				m_bTagValues[i] = bTagValue;
				m_cTagValues[i] = cTagValue;
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
    	std::cerr << exc.what();
	}
};

void EventObservablesBase::init(){
    printParameters();

	m_bTagValues = std::vector<double>(m_nAskedJets(), -1.);
  	m_cTagValues = std::vector<double>(m_nAskedJets(), -1.);

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