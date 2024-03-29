#include "PreSelection.h"
#include <iostream>
#include <vector>
#include <string>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCIntVec.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;
using namespace std ;

template<class T>
double inv_mass(T* p1, T* p2){
  double e = p1->getEnergy()+p2->getEnergy() ;
  double px = p1->getMomentum()[0]+p2->getMomentum()[0];
  double py = p1->getMomentum()[1]+p2->getMomentum()[1];
  double pz = p1->getMomentum()[2]+p2->getMomentum()[2];
  return( sqrt( e*e - px*px - py*py - pz*pz  ) );
}

template<class T>
TLorentzVector v4(T* p){
  return TLorentzVector( p->getMomentum()[0],p->getMomentum()[1], p->getMomentum()[2],p->getEnergy());
}

PreSelection aPreSelection ;

PreSelection::PreSelection() :

  Processor("PreSelection"),
  m_nRun(0),
  m_nEvt(0)

{

	_description = "PreSelection writes relevant observables to root-file " ;

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
				   "Which set of cuts to use in the preselection",
				   m_whichPreselection,
				   std::string("custom")
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

	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				 "PreSelectionCollection",
				 "preselection collection",
				 m_PreSelectionCollection,
				 std::string("preselection")
				 );

	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				 "HiggsCollection",
				 "Reconstructed Higgs collection",
				 m_HiggsCollection,
				 std::string("HiggsPair")
				 );

	registerOutputCollection( LCIO::LCINTVEC,
				  "isPassed",
				  "Output for whether preselection is passed" ,
				  m_isPassed,
				  std::string("ispassed")
				  );

}

void PreSelection::init()
{
  streamlog_out(DEBUG) << "   init called  " << std::endl;
  this->Clear();
  m_nRun = 0;
  m_nEvt = 0;

  m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
  m_pTTree = new TTree("eventTree","eventTree");
  m_pTTree->SetDirectory(m_pTFile);

  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("nJets",&m_nJets,"nJets/I") ;
  m_pTTree->Branch("nIsoLeptons",&m_nIsoLeps,"nIsoLeptons/I") ;
  m_pTTree->Branch("missingPT", &m_missingPT, "missingPT/F") ;
  m_pTTree->Branch("Evis", &m_Evis, "Evis/F") ;
  m_pTTree->Branch("thrust", &m_thrust, "thrust/F") ;
  m_pTTree->Branch("dileptonMass", &m_dileptonMass, "dileptonMass/F") ;
  m_pTTree->Branch("dileptonMassDiff", &m_dileptonMassDiff, "dileptonMassDiff/F") ;
  m_pTTree->Branch("dijetMass", &m_dijetMass);
  m_pTTree->Branch("dijetMassDiff", &m_dijetMassDiff);
  m_pTTree->Branch("dihiggsMass", &m_dihiggsMass, "dihiggsMass/F");
  m_pTTree->Branch("nbjets", &m_nbjets, "nbjets/I");
  m_pTTree->Branch("blikelihoodness", &m_blikelihoodness);
  streamlog_out(DEBUG) << "   init finished  " << std::endl;

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
  }
  else if (m_whichPreselection == "vvbbbb") {
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
  }
  else if (m_whichPreselection == "qqbbbb") {
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

void PreSelection::Clear() 
{
  streamlog_out(DEBUG) << "   Clear called  " << std::endl;
  
  m_nJets = 0;
  m_nIsoLeps = 0;
  m_missingPT = -999.;
  m_Evis  = -999.;
  m_thrust = -999.;
  m_dileptonMass = -999.;
  m_dileptonMassDiff = -999.;
  m_dijetMass.clear();
  m_dijetMassDiff.clear();
  m_dihiggsMass = -999;
  m_nbjets = 0;
  m_blikelihoodness.clear();
}
void PreSelection::processRunHeader( LCRunHeader*  /*run*/) { 
  m_nRun++ ;
} 

void PreSelection::processEvent( EVENT::LCEvent *pLCEvent )
{
  this->Clear();
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

  LCCollection *inputJetCollection{};
  LCCollection *inputLeptonCollection{};
  LCCollection *inputLepPairCollection{};
  LCCollection *inputPfoCollection{};
  bool passed = true;

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
    LCCollectionVec* higgscol = new LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
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
    // ---------- VISIBLE ENERGY ----------                                                
    m_Evis = pfosum.E();
    // ---------- THRUST ----------                                                        
    const EVENT::LCParameters& pfoparams = inputPfoCollection->getParameters() ;
    m_thrust = pfoparams.getFloatVal("principleThrustValue");
    //-----------------  REQUIRE CORRECT NUMBER OF SIGNATURE PARTICLES  -----------------
    if (inputLepPairCollection->getNumberOfElements() == 2 ) {
      // ---------- CALCULATE DILEPTON MASS AND DIFFERENCE ----------
      std::vector< ReconstructedParticle* > Leptons{};
      TLorentzVector dileptonFourMomentum = TLorentzVector(0.,0.,0.,0.);
      for ( int i_lep = 0 ; i_lep < 2 ; ++i_lep ) {
        ReconstructedParticle* lepton = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt( i_lep ) );
        Leptons.push_back( lepton );
        dileptonFourMomentum += TLorentzVector( lepton->getMomentum()[ 0 ] , lepton->getMomentum()[ 1 ] , lepton->getMomentum()[ 2 ] , lepton->getEnergy() );
      }
      m_dileptonMass = dileptonFourMomentum.M();
      m_dileptonMassDiff = fabs( m_dileptonMass - 91.2 );
    }

    if ( m_nJets == m_nAskedJets ) {
      // ---------- JET PROPERTIES AND FLAVOUR TAGGING ----------
      vector<ReconstructedParticle*> jets;
      for (int i=0; i<m_nJets; ++i) {
        ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
	jets.push_back(jet);
      }
      std::string _FTAlgoName = "lcfiplus"; // FT = Flavor Tag
      PIDHandler FTHan(inputJetCollection);
      int _FTAlgoID = FTHan.getAlgorithmID(_FTAlgoName);
      int BTagID = FTHan.getParameterIndex(_FTAlgoID, "BTag");
      //int CTagID = FTHan.getParameterIndex(_FTAlgoID, "CTag");
      //int OTagID = FTHan.getParameterIndex(_FTAlgoID, "OTag");

      for (int i=0; i<m_nJets; ++i) {
	const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(FTHan.getParticleID(jets[i],_FTAlgoID));
        const FloatVec& FTPara = FTImpl.getParameters();
        double bTagValue = FTPara[BTagID];
        //double cTagValue = FTPara[CTagID];
        //double oTagValue = FTPara[OTagID];
	if (bTagValue>m_minblikeliness) m_nbjets++;
	m_blikelihoodness.push_back(bTagValue);
      }	
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

      int nperm = perms.size();
      int ndijets = 0;
      if (nperm != 0)  {
	
	float chi2min = 99999. ;
	ndijets = 2;
	vector<float> dijetmass{-999., -999.};
	unsigned int best_idx;
	
	for (int i=0; i<nperm; i++) {
	  float m1 = inv_mass(jets[perms[i][0]], jets[perms[i][1]]);
	  float m2 = inv_mass(jets[perms[i][2]], jets[perms[i][3]]);
	  float chi2 = (m1-125)*(m1-125)+(m2-125)*(m2-125);
	  if (chi2 < chi2min) {
	    chi2min = chi2;
	    dijetmass[0] = m1;
	    dijetmass[1] = m2;
	    best_idx = i;
	  }
	}
	TLorentzVector vdijet[2];
	vdijet[0] = v4(jets[perms[best_idx][0]]) + v4(jets[perms[best_idx][1]]);
	vdijet[1] = v4(jets[perms[best_idx][2]]) + v4(jets[perms[best_idx][3]]);
	for (int i=0; i<ndijets; i++) {
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
	  higgscol->addElement(higgs);
	}
	for (int i=0; i<ndijets; i++) {
	  m_dijetMass.push_back(dijetmass[i]);
	  m_dijetMassDiff.push_back(fabs( dijetmass[i] - 125. ));
	}
	m_dihiggsMass = (vdijet[0]+vdijet[1]).M();
      }
    } //if ( m_nJets == m_nAskedJets )

    // ---------- PRESELECTION ----------
    if (m_nJets != m_nAskedJets) passed = false;
    if (m_nIsoLeps < 2 && m_nAskedIsoLeps == 2) passed = false;
    if (m_nIsoLeps > 0 && m_nAskedIsoLeps == 0) passed = false;
    if (m_dileptonMassDiff > m_maxdileptonmassdiff ) passed = false;
    for (unsigned int i=0; i<m_dijetMassDiff.size(); i++) {
      if (m_dijetMassDiff[i] > m_maxdijetmassdiff) passed = false;
      if (m_dijetMass[i] > m_maxdijetmass || m_dijetMass[i] < m_mindijetmass) passed = false;
    }
    if (m_missingPT > m_maxmissingPT || m_missingPT < m_minmissingPT) passed = false;
    if (m_thrust > m_maxthrust) passed = false;
    if (m_Evis > m_maxEvis) passed = false;
    if (m_dihiggsMass < m_minHHmass) passed = false;
    if (m_nbjets < m_minnbjets) passed = false;
    ReconstructedParticleImpl* ispassedparticle = new ReconstructedParticleImpl;
    ispassedparticle->setType(passed);
    preselectioncol->addElement(ispassedparticle);
    preselectioncol->parameters().setValue("isPassed", (bool)passed);
    ispassedvec->push_back(passed);
    ispassedcol->addElement(ispassedvec);
    pLCEvent->addCollection(preselectioncol, m_PreSelectionCollection);
    pLCEvent->addCollection(higgscol, m_HiggsCollection);
    pLCEvent->addCollection(ispassedcol, m_isPassed);
  }
  catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
  }
  if (passed) setReturnValue("GoodEvent", true);
  else setReturnValue("GoodEvent", false);
  
  m_pTTree->Fill();


}

void PreSelection::check()
{
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void PreSelection::end()
{
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;
}
