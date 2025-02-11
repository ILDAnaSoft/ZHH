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
  m_errorCode(0),
  m_bTagValues(m_nJets, -1.),
  m_cTagValues(m_nJets, -1.)
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

	registerProcessorParameter("PIDAlgorithmBTag",
            "Number of jet should be in the event",
            m_PIDAlgorithmBTag,
            std::string("lcfiplus")
            );

	registerProcessorParameter("nJets",
            "Number of jet should be in the event",
            m_nAskedJets,
            int(4)
            );

	registerProcessorParameter("nIsoLeps",
            "Number of Isolated Leptons should be in the event",
            m_nAskedIsoLeps,
            int(2)
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
		ttree->Branch("nJets",&m_nJets,"nJets/I");
		ttree->Branch("nIsoLeptons",&m_nIsoLeps,"nIsoLeptons/I");
		ttree->Branch("lepTypes", &m_lepTypes);
		ttree->Branch("lepTypesPaired", &m_lepTypesPaired, "lepTypesPaired/I");
		ttree->Branch("missingPT", &m_missingPT, "missingPT/F");
		
		ttree->Branch("missingEnergy", &m_missingE, "missingEnergy/F");
		
		ttree->Branch("thrust", &m_thrust, "thrust/F");
		ttree->Branch("dileptonMassPrePairing", &m_dileptonMassPrePairing, "dileptonMassPrePairing/F");
		ttree->Branch("dileptonMass", &m_dileptonMass, "dileptonMass/F");
		ttree->Branch("dileptonMassDiff", &m_dileptonMassDiff, "dileptonMassDiff/F");
		ttree->Branch("dijetChi2min", &m_chi2min, "dijetChi2min/F");
		ttree->Branch("dijetPairing", &m_dijetPairing);
		ttree->Branch("dijetMass", &m_dijetMass);
		ttree->Branch("dijetMassDiff", &m_dijetMassDiff);
		ttree->Branch("bTags", &m_bTagValues);
		ttree->Branch("dihiggsMass", &m_dihiggsMass, "dihiggsMass/F");
		ttree->Branch("nbjets", &m_nbjets, "nbjets/I");

		// bmax1:bmax2:bmax3:bmax4:pj1jets2:pj2jets2
		ttree->Branch("bmax1", &m_bmax1, "bmax1/F");
		ttree->Branch("bmax2", &m_bmax2, "bmax2/F");
		ttree->Branch("bmax3", &m_bmax3, "bmax3/F");
		ttree->Branch("bmax4", &m_bmax4, "bmax4/F");

		ttree->Branch("cmax1", &m_cmax1, "cmax1/F");
		ttree->Branch("cmax2", &m_cmax2, "cmax2/F");
		ttree->Branch("cmax3", &m_cmax3, "cmax3/F");
		ttree->Branch("cmax4", &m_cmax4, "cmax4/F");

		ttree->Branch("preselsPassedVec", &m_preselsPassedVec);
		ttree->Branch("preselsPassedAll", &m_preselsPassedAll);
		ttree->Branch("preselsPassedConsec", &m_preselsPassedConsec);
		ttree->Branch("preselPassed", &m_isPassed);
	}

	streamlog_out(DEBUG) << "   init finished  " << std::endl;

	if (m_cutDefinitionsJSONFile.length() > 0) {
		streamlog_out(DEBUG) << "Reading preselection cuts from file " << m_cutDefinitionsJSONFile << std::endl;

		std::ifstream ifs(m_cutDefinitionsJSONFile);
		jsonf cuts = jsonf::parse(ifs);
		
		std::string preselection_key = m_whichPreselection.substr(0, 2);

		m_nAskedJets = cuts[preselection_key]["nAskedJets"];
		m_nAskedIsoLeps = cuts[preselection_key]["nAskedIsoLeps"];
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
			m_nAskedJets = 4;
			m_nAskedIsoLeps = 2;
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
			m_nAskedJets = 4;
			m_nAskedIsoLeps = 0;
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
			m_nAskedJets = 6;
			m_nAskedIsoLeps = 0;
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
	m_lepTypes.clear();
	m_lepTypesPaired = 0;
	m_missingPT = -999.;
	m_missingMass = -999.;
	m_missingE = -999.;
	m_Evis  = -999.;
	m_thrust = -999.;
	m_dileptonMassPrePairing = -999.;
	m_dileptonMass = -999.;
	m_dileptonMassDiff = -999.;
	m_dijetMass.clear();
	m_dijetMassDiff.clear();
	std::fill(m_bTagValues.begin(), m_bTagValues.end(), -1.);
	std::fill(m_cTagValues.begin(), m_cTagValues.end(), -1.);
	m_dihiggsMass = -999;
	m_nbjets = 0;
	m_chi2min = 99999.;
	m_dijetPairing.clear();

	m_preselsPassedVec.clear();
	m_preselsPassedAll = 0;
	m_preselsPassedConsec = 0;
	m_isPassed = 0;
}

void EventObservablesBase::updateBaseValues(EVENT::LCEvent *pLCEvent) {
	this->clearBaseValues();

	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();
	streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

	LCCollection *inputJetCollection{};
	LCCollection *inputLeptonCollection{};
	LCCollection *inputLepPairCollection{};
	LCCollection *inputPfoCollection{};

	try {
		streamlog_out(DEBUG0) << "        getting jet collection: " << m_inputJetCollection << std::endl ;
		inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
		streamlog_out(DEBUG0) << "        getting isolated lepton collection: " << m_inputIsolatedleptonCollection << std::endl ;
		inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
		streamlog_out(DEBUG0) << "        getting lepton pair collection: " << m_inputLepPairCollection << std::endl ;
		inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
		streamlog_out(DEBUG0) << "        getting pfo collection: " << m_inputPfoCollection << std::endl ;
		inputPfoCollection = pLCEvent->getCollection( m_inputPfoCollection );

		m_nJets = inputJetCollection->getNumberOfElements();
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

		//-----------------  REQUIRE CORRECT NUMBER OF SIGNATURE PARTICLES  -----------------
		if (inputLepPairCollection->getNumberOfElements() == 2 ) {
			m_dileptonMass = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
			m_dileptonMassPrePairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");

			m_dileptonMassDiff = fabs( m_dileptonMass - 91.2 );

			// ---------- SAVE TYPES OF PAIRED ISOLATED LEPTONS ----------
			ReconstructedParticle* first_iso_lepton_from_pair = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(0));
			m_lepTypesPaired = first_iso_lepton_from_pair->getType();
		}

		// ---------- SAVE TYPES OF ALL ISOLATED LEPTONS ----------
		for (int j = 0; j < inputLeptonCollection->getNumberOfElements(); j++) {
			ReconstructedParticle* iso_lepton = dynamic_cast<ReconstructedParticle*>( inputLeptonCollection->getElementAt( j ) );
			m_lepTypes.push_back( iso_lepton->getType() );
		}

		// ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
		if ( m_nJets == m_nAskedJets ) {
			vector<ReconstructedParticle*> jets;
			for (int i=0; i<m_nJets; ++i) {
				ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
				jets.push_back(jet);
			}

			PIDHandler FTHan(inputJetCollection);
			int _FTAlgoID = FTHan.getAlgorithmID(m_PIDAlgorithmBTag);
			int BTagID = FTHan.getParameterIndex(_FTAlgoID, "BTag");
			int CTagID = FTHan.getParameterIndex(_FTAlgoID, "CTag");
			//int OTagID = FTHan.getParameterIndex(_FTAlgoID, "OTag");

			for (int i=0; i<m_nJets; ++i) {
				const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(FTHan.getParticleID(jets[i], _FTAlgoID));
				const FloatVec& FTPara = FTImpl.getParameters();
				double bTagValue = FTPara[BTagID];
				double cTagValue = FTPara[CTagID];
				//double oTagValue = FTPara[OTagID];

				m_bTagValues[i] = bTagValue;
				m_cTagValues[i] = cTagValue;

				if (bTagValue > m_minblikeliness)
					m_nbjets++;
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
		m_preselsPassedVec.push_back(m_nJets == m_nAskedJets);
		m_preselsPassedVec.push_back(m_nIsoLeps == m_nAskedIsoLeps);
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
		
		// Compile outputs
		m_preselsPassedAll = std::accumulate(m_preselsPassedVec.begin(), m_preselsPassedVec.end(), 0); // 
		m_isPassed = m_preselsPassedAll == m_preselsPassedVec.size(); // Passed if passed all

		// Check how many presels passed consecutively
		for (size_t i=0; i < m_preselsPassedVec.size(); i++) {
			if (m_preselsPassedVec[i]) {
				m_preselsPassedConsec++;
			} else {
				break;
			}
		}

	} catch(DataNotAvailableException &e) {
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
	}
};

void EventObservablesBase::init(){
    printParameters();

	prepareBaseTree();
	prepareChannelTree();
};

void EventObservablesBase::processEvent( LCEvent* evt ){
	clearBaseValues();
	updateBaseValues(evt);

	clearChannelValues();
	updateChannelValues(evt);

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