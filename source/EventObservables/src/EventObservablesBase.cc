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

EventObservablesBase::EventObservablesBase() :
  m_nRun(0),
  m_nEvt(0),
  m_errorCode(0),
  m_bTagValues(m_nJets, -1.),
  m_cTagValues(m_nJets, -1.)
{
};

void EventObservablesBase::prepareBaseTree()
{
	if (m_outputFile.size()) {
		m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
		m_pTTree->SetDirectory(m_pTFile);
	}

	if (m_write_ttree) {
		m_pTTree->Branch("run", &m_nRun, "run/I");
		m_pTTree->Branch("event", &m_nEvt, "event/I");

		// evis:mm:mh1:mh2:mhh:pz:ph1:ph2:cosz:cosh1:cosh2:yminus:yplus
		m_pTTree->Branch("evis", &m_Evis, "evis/F");
		m_pTTree->Branch("m_miss", &m_missingMass, "m_miss/F");
		m_pTTree->Branch("mh1", &m_mh1, "mh1/F");
		m_pTTree->Branch("mh2", &m_mh2, "mh2/F");
		m_pTTree->Branch("mhh", &m_mhh, "mhh/F");

		m_pTTree->Branch("pz", &m_pz, "pz/F");
		m_pTTree->Branch("ph1", &m_ph1, "ph1/F");
		m_pTTree->Branch("ph2", &m_ph2, "ph2/F");
		m_pTTree->Branch("cosz", &m_cosz, "cosz/F");
		m_pTTree->Branch("cosh1", &m_cosh1, "cosh1/F");
		m_pTTree->Branch("cosh2", &m_cosh2, "cosh2/F");
		m_pTTree->Branch("yminus", &m_todo, "yminus/F");
		m_pTTree->Branch("yplus", &m_yplus, "yplus/F");

		// nhbb:njet:chi2:mpt:prob11:prob12:prob21:prob22
		m_pTTree->Branch("nJets",&m_nJets,"nJets/I");
		m_pTTree->Branch("nIsoLeptons",&m_nIsoLeps,"nIsoLeptons/I");
		m_pTTree->Branch("lepTypes", &m_lepTypes);
		m_pTTree->Branch("lepTypesPaired", &m_lepTypesPaired, "lepTypesPaired/I");
		m_pTTree->Branch("missingPT", &m_missingPT, "missingPT/F");
		
		m_pTTree->Branch("missingEnergy", &m_missingE, "missingEnergy/F");
		
		m_pTTree->Branch("thrust", &m_thrust, "thrust/F");
		m_pTTree->Branch("dileptonMassPrePairing", &m_dileptonMassPrePairing, "dileptonMassPrePairing/F");
		m_pTTree->Branch("dileptonMass", &m_dileptonMass, "dileptonMass/F");
		m_pTTree->Branch("dileptonMassDiff", &m_dileptonMassDiff, "dileptonMassDiff/F");
		m_pTTree->Branch("dijetChi2min", &m_chi2min, "dijetChi2min/F");
		m_pTTree->Branch("dijetPairing", &m_dijetPairing);
		m_pTTree->Branch("dijetMass", &m_dijetMass);
		m_pTTree->Branch("dijetMassDiff", &m_dijetMassDiff);
		m_pTTree->Branch("bTags", &m_bTagValues);
		m_pTTree->Branch("dihiggsMass", &m_dihiggsMass, "dihiggsMass/F");
		m_pTTree->Branch("nbjets", &m_nbjets, "nbjets/I");

		// bmax1:bmax2:bmax3:bmax4:pj1jets2:pj2jets2
		m_pTTree->Branch("bmax1", &m_bmax1, "bmax1/F");
		m_pTTree->Branch("bmax2", &m_bmax2, "bmax2/F");
		m_pTTree->Branch("bmax3", &m_bmax3, "bmax3/F");
		m_pTTree->Branch("bmax4", &m_bmax4, "bmax4/F");

		m_pTTree->Branch("cmax1", &m_cmax1, "cmax1/F");
		m_pTTree->Branch("cmax2", &m_cmax2, "cmax2/F");
		m_pTTree->Branch("cmax3", &m_cmax3, "cmax3/F");
		m_pTTree->Branch("cmax4", &m_cmax4, "cmax4/F");

		m_pTTree->Branch("preselsPassedVec", &m_preselsPassedVec);
		m_pTTree->Branch("preselsPassedAll", &m_preselsPassedAll);
		m_pTTree->Branch("preselsPassedConsec", &m_preselsPassedConsec);
		m_pTTree->Branch("preselPassed", &m_isPassed);
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

void EventObservablesBase::baseClear() 
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

void EventObservablesBase::updateValues(EVENT::LCEvent *pLCEvent) {
	this->baseClear();

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

		LCCollectionVec* preselectioncol = new LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
		LCCollectionVec *ispassedcol = new LCCollectionVec(LCIO::LCINTVEC);
		LCIntVec *ispassedvec = new LCIntVec;

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

		// ---------- SAVE OUTPUT ----------
		ReconstructedParticleImpl* ispassedparticle = new ReconstructedParticleImpl;
		ispassedparticle->setType(m_isPassed);
		preselectioncol->addElement(ispassedparticle);
		preselectioncol->parameters().setValue("preselectionPassed", m_isPassed);
		ispassedvec->push_back(m_isPassed);
		ispassedcol->addElement(ispassedvec);
		pLCEvent->removeCollection(m_HiggsCollection);
		pLCEvent->addCollection(preselectioncol, m_EventObservablesBaseCollection);
		// std::vector<int> jetMatchingByMass = EventObservablesBase::pairJetsByMass(jets);
		// pLCEvent->addCollection(higgscol, m_HiggsCollection);
		pLCEvent->addCollection(ispassedcol, m_isPassedCollection);

	} catch(DataNotAvailableException &e) {
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
	}

	if (m_write_ttree) {
		m_pTTree->Fill();
	}
};

std::vector<int> EventObservablesBase::pairJetsByMass(std::vector<ReconstructedParticle*> jets, IMPL::LCCollectionVec* higgsCandidates) {
	vector<vector<int>> perms;
	if (m_nAskedJets == 4 || (m_nAskedJets == 6 && m_nbjets == 4)) {
		vector<vector<int>> temp {
		{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}
		};
		perms = temp;
	}

	if (m_nAskedJets == 6 && m_nbjets == 5) {
		vector<vector<int>> temp {
		{1,2,3,4}, {1,3,2,4}, {1,4,2,3},
		{0,2,3,4}, {0,3,2,4}, {0,4,2,3},
		{0,1,3,4}, {0,3,1,4}, {0,4,1,3},
		{0,1,2,4}, {0,2,1,4}, {0,4,1,2},
		{0,1,2,3}, {0,2,1,3}, {0,3,1,2}
		};
		perms = temp;
	}

	if (m_nAskedJets == 6 && m_nbjets == 6) {
		vector<vector<int>> temp {
		{2,3,4,5}, {2,4,3,5}, {2,5,3,4}, // 0,1
		{1,3,4,5}, {1,4,3,5}, {1,5,3,4}, // 0,2
		{1,2,4,5}, {1,4,2,5}, {1,5,2,4}, // 0,3
		{1,2,3,5}, {1,3,2,5}, {1,5,2,3}, // 0,4
		{1,2,3,4}, {1,3,2,4}, {1,4,2,3}, // 0,5
		{0,3,4,5}, {0,4,3,5}, {0,5,3,4}, // 1,2
		{0,2,4,5}, {0,4,2,5}, {0,5,2,4}, // 1,3
		{0,2,3,5}, {0,3,2,5}, {0,5,2,3}, // 1,4
		{0,2,3,4}, {0,3,2,4}, {0,4,2,3}, // 1,5
		{0,1,4,5}, {0,4,1,5}, {0,5,1,4}, // 2,3
		{0,1,3,5}, {0,3,1,5}, {0,5,1,3}, // 2,4
		{0,1,3,4}, {0,3,1,4}, {0,4,1,3}, // 2,5
		{0,1,2,5}, {0,2,1,5}, {0,5,1,2}, // 3,4
		{0,1,2,4}, {0,2,1,4}, {0,4,1,2}, // 3,5
		{0,1,2,3}, {0,2,1,3}, {0,3,1,2}, // 4,5
		};
		perms = temp;
	}

	size_t nperm = perms.size();
	unsigned int best_idx = 0;

	if (nperm != 0)  {
		m_ndijets = 2;
		vector<float> dijetmass{-999., -999.};
		
		for (size_t i=0; i < nperm; i++) {
			float m1 = inv_mass(jets[perms[i][0]], jets[perms[i][1]]);
			float m2 = inv_mass(jets[perms[i][2]], jets[perms[i][3]]);
			float chi2 = (m1-125)*(m1-125)+(m2-125)*(m2-125);
			if (chi2 < m_chi2min) {
				m_chi2min = chi2;
				dijetmass[0] = m1;
				dijetmass[1] = m2;
				best_idx = i;
			}
		}

		// Save dijet pairing
		for (size_t i=0; i < 4; i++) {
			m_dijetPairing.push_back(perms[best_idx][i]);
		}

		TLorentzVector vdijet[2];
		vdijet[0] = v4(jets[perms[best_idx][0]]) + v4(jets[perms[best_idx][1]]);
		vdijet[1] = v4(jets[perms[best_idx][2]]) + v4(jets[perms[best_idx][3]]);

		for (int i=0; i < m_ndijets; i++) {
			ReconstructedParticleImpl* higgs = new ReconstructedParticleImpl;
			float momentum[3];
			momentum[0]= vdijet[i].Px();
			momentum[1]= vdijet[i].Py();
			momentum[2]= vdijet[i].Pz();
			higgs->setMomentum(momentum);
			higgs->setEnergy(vdijet[i].E());
			higgs->setType(25);
			higgs->setCharge(0);
			higgs->setMass(vdijet[i].M());
			higgsCandidates->addElement(higgs);
		}

		// Save mapping of jets to HiggsPair
		higgsCandidates->parameters().setValue("h1jet1id", perms[best_idx][0]);
		higgsCandidates->parameters().setValue("h1jet2id", perms[best_idx][1]);
		higgsCandidates->parameters().setValue("h2jet1id", perms[best_idx][2]);
		higgsCandidates->parameters().setValue("h2jet2id", perms[best_idx][3]);

		for (int i=0; i < m_ndijets; i++) {
			m_dijetMass.push_back(dijetmass[i]);
			m_dijetMassDiff.push_back(fabs( dijetmass[i] - 125. ));
		}
		m_dihiggsMass = (vdijet[0]+vdijet[1]).M();
	}

	return perms[best_idx];
};