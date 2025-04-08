#include "LeptonPairing.h"
#include <stdlib.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include "ZHHUtilities.h"
#include "TH2F.h"
#include "TF1.h"
#include "TTree.h"

#include <streamlog/streamlog.h>
#include "marlin/VerbosityLevels.h"

#include <algorithm>

#ifdef MARLIN_USE_AIDA
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
//#include <AIDA/IHistogram1D.h>                                                                                                                              
#endif // MARLIN_USE_AIDA  

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

LeptonPairing aLeptonPairing;

LeptonPairing::LeptonPairing():
  Processor("LeptonPairing"),
  m_nRun(0),
  m_nEvt(0),
  m_IsoLepsInvMass{},
  m_RecoLepsInvMass{},
  m_pTFile(NULL)
{
  _description = "LeptonPairing pairs isolated leptons of opposite charge and adds additional leptons to the list of PFOs or rejects the event" ;

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "IsoLepCollection",
			  "Name of input Isolated Lepton collection",
			  m_inputIsoLepCollection,
			  string("ISOLeptons")
			  );
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "PFOsWOIsoLepCollection",
			  "Name of input PFO collection without isolated leptons",
			  m_inputPFOsWOIsoLepCollection,
			  string("PandoraPFOsWithoutIsoLep")
			  );

  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "LepPairCollection",
			   "Name of output lepton pair collection",
			   m_outputLepPairCollection,
			   string("LeptonPair")
			   );

  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "PFOsWOLepPairCollection",
			   "Name of output PFO collection without lepton pair",
			   m_outputPFOsWOLepPairCollection,
			   string("PandoraPFOsWithoutLepPair")
			   );

  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "IsoLepPairCollection",
			   "Name of output paired isolated lepton collection",
			   m_outputIsoLepPairCollection,
			   string("IsoLeptonPair")
			   );

  registerProcessorParameter("diLepInvMass",
			     "Invariant mass of di-lepton system in Isolated Leptons [GeV]",
			     m_diLepInvMass,
			     double(91.2)
			     );

  registerProcessorParameter("doPhotonRecovery",
			     "Do photon recovery",
			     m_doPhotonRecovery,
			     bool(true)
			     );


  registerProcessorParameter("fillRootTree",
			     "Fill root tree to check processor performance",
			     m_fillRootTree,
			     bool(true)
			     );

  registerProcessorParameter("RootFile",
			     "Name of the output root file",
			     m_rootFile,
			     string("")
			     );
}

void LeptonPairing::init() {
  streamlog_out(DEBUG0) << "   init called  " << endl ;
  printParameters();
  if ( m_fillRootTree ) {
    streamlog_out(DEBUG0) << "      Creating root file/tree/histograms" << endl ;

    if (m_rootFile.size()) {
      m_pTFile = new TFile(m_rootFile.c_str(), "recreate");
      m_pTTree->SetDirectory(m_pTFile);
    }

    m_pTTree->Branch("event", &m_nEvt, "event/I");
    m_pTTree->Branch("IsoLepsInvMass", &m_IsoLepsInvMass);
    m_pTTree->Branch("RecoLepsInvMass", &m_RecoLepsInvMass);
    streamlog_out(DEBUG0) << "      Created root file/tree/histograms" << endl ;
  }

  this->Clear();
  m_nRun = 0;
  m_nEvt = 0;
  streamlog_out(DEBUG) << "   init finished  " << endl;
}

void LeptonPairing::Clear() {
  streamlog_out(DEBUG) << "   Clear called  " << endl;
  m_IsoLepsInvMass.clear();
  m_RecoLepsInvMass.clear();
}

void LeptonPairing::processRunHeader( LCRunHeader* run ) { 
  (void) run;
} 

void LeptonPairing::processEvent( EVENT::LCEvent *pLCEvent ) {
  streamlog_out(DEBUG0) << "      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<       processEvent Called     >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl ;
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(DEBUG4) << "" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////    Processing event        " << m_nEvt << "        ////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;
  this->Clear();

  EVENT::LCCollection *IsoLepCollection{};
  EVENT::LCCollection *PFOsWOIsoLepCollection{};
  try {
    IsoLepCollection = pLCEvent->getCollection(m_inputIsoLepCollection);
    PFOsWOIsoLepCollection = pLCEvent->getCollection(m_inputPFOsWOIsoLepCollection);
    streamlog_out(DEBUG7) << "Input collections successfully found in event " << m_nEvt << endl; 
  }
  catch( DataNotAvailableException &e ) {
    streamlog_out(MESSAGE) << "     Input collection not found in event " << m_nEvt << endl;
  }
  IMPL::LCCollectionVec* m_LepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  //m_LepPairCol->setSubset( true );
  IMPL::LCCollectionVec* m_PFOsWOLepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  m_PFOsWOLepPairCol->setSubset( true );
  IMPL::LCCollectionVec* m_IsoLepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  m_IsoLepPairCol->setSubset( true );

  int InIsoLeps = IsoLepCollection->getNumberOfElements();
  streamlog_out(DEBUG7) << "Number of iso leptons = " << InIsoLeps << endl;
  vector< ReconstructedParticle* > LeptonPair = {};
  int _lep_type = 0;
  Double_t fCosFSRCut; // the angle of BS and FSR around the direction of charged lepton
  if (m_doPhotonRecovery) fCosFSRCut = 0.99;
  else fCosFSRCut = 99.;

  IntVec paired_lep_idx(2, 0);

  if (InIsoLeps == 2) {
    ReconstructedParticle* lepton1 = static_cast<ReconstructedParticle*>( IsoLepCollection->getElementAt( 0 ) );
    ReconstructedParticle* lepton2 = static_cast<ReconstructedParticle*>( IsoLepCollection->getElementAt( 1 ) );
    //Check if same type and have opposite charge
    streamlog_out(DEBUG7) << "pre check" << endl;
    streamlog_out(DEBUG7) << lepton1->getType() << " " << lepton2->getType()<< " " << lepton1->getCharge() << " " << lepton2->getCharge() << endl;
    if (lepton1->getType() + lepton2->getType() == 0) {
      LeptonPair = {lepton1, lepton2};
      _lep_type = lepton1->getType();
      streamlog_out(DEBUG7) << "mass = " << inv_mass(lepton1, lepton2)  << endl;
      streamlog_out(DEBUG7) << "same type, opposite charge" << endl;
    }
    streamlog_out(DEBUG7) << "post check" << endl;
  } else if (InIsoLeps > 2) {
    float mindelta = 99999.;
    for ( int i_lep1 = 0 ; i_lep1 < InIsoLeps - 1 ; ++i_lep1 ) {
      ReconstructedParticle* lepton1 = static_cast<ReconstructedParticle*>( IsoLepCollection->getElementAt( i_lep1 ) );
      for ( int i_lep2 = i_lep1 + 1 ; i_lep2 < InIsoLeps ; ++i_lep2 ) {
	ReconstructedParticle* lepton2 = static_cast<ReconstructedParticle*>( IsoLepCollection->getElementAt( i_lep2 ) );
	//Check if same type and have opposite charge 
	if (lepton1->getType() + lepton2->getType() == 0) {
	  float pairmass = inv_mass(lepton1, lepton2);
	  streamlog_out(DEBUG7) << "mass = " << pairmass  << endl;
	  float delta = abs(pairmass-m_diLepInvMass);
	  if (delta > mindelta) continue;
	  mindelta = delta;  
	  LeptonPair = {lepton1, lepton2};
    paired_lep_idx[0] = i_lep1;
    paired_lep_idx[1] = i_lep2;
	  _lep_type = lepton1->getType();
	} 
      }
    }
  }
  //Select lepton pair
  //(add pre invmass to root tree)
  //Do FSR and BS recovery
  //(add post invmass to root tree)
  //Add reco lepton pair to output collection
  //Add all other pfos (including additional leptons) that are not from lepton pair or photons to output

  vector<lcio::ReconstructedParticle*> photons;
  if (LeptonPair.size() == 2) {
    // recovery of FSR and BS
    m_IsoLepsInvMass.push_back(inv_mass(LeptonPair[0], LeptonPair[1]));
    ReconstructedParticleImpl * recoLepton1 = new ReconstructedParticleImpl();
    ZHH::doPhotonRecovery(LeptonPair[0],PFOsWOIsoLepCollection,recoLepton1,fCosFSRCut,_lep_type,photons);
    ReconstructedParticleImpl * recoLepton2 = new ReconstructedParticleImpl();
    ZHH::doPhotonRecovery(LeptonPair[1],PFOsWOIsoLepCollection,recoLepton2,fCosFSRCut,_lep_type,photons);
    m_RecoLepsInvMass.push_back(inv_mass(recoLepton1,recoLepton2));
    
    m_IsoLepPairCol->addElement(LeptonPair[0]);
    m_IsoLepPairCol->addElement(LeptonPair[1]);
    m_LepPairCol->addElement(recoLepton1);
    m_LepPairCol->addElement(recoLepton2);

    m_LepPairCol->parameters().setValues("PairedLeptonIDx", paired_lep_idx);
    m_LepPairCol->parameters().setValue("IsoLepsInvMass", m_IsoLepsInvMass[0]);
    m_LepPairCol->parameters().setValue("RecoLepsInvMass", m_RecoLepsInvMass[0]);
  }
  for ( int i_lep = 0 ; i_lep < InIsoLeps ; ++i_lep ) {
    bool isFromPair = false;
    ReconstructedParticle* lepton = static_cast<ReconstructedParticle*>(IsoLepCollection->getElementAt(i_lep));
    for (auto leptonfrompair: LeptonPair) {
      if (lepton == leptonfrompair) isFromPair = true;
    }
    if (!isFromPair) m_PFOsWOLepPairCol->addElement(lepton);
  }
  for (int i=0; i<PFOsWOIsoLepCollection->getNumberOfElements(); i++) {
    bool isPhoton = false;
    ReconstructedParticle* pfo =static_cast<ReconstructedParticle*>(PFOsWOIsoLepCollection->getElementAt(i));
    for (auto photon: photons) {
      if (pfo == photon) isPhoton = true;
    }
    if (!isPhoton)  m_PFOsWOLepPairCol->addElement(pfo);
  }
  streamlog_out(DEBUG7) << "nphotons " << photons.size() << endl;
  streamlog_out(DEBUG7) << "nselected " << m_LepPairCol->getNumberOfElements() << endl;
  streamlog_out(DEBUG7) << "npfos " << PFOsWOIsoLepCollection->getNumberOfElements() << endl;
  streamlog_out(DEBUG7) << "nnewpfos " << m_PFOsWOLepPairCol->getNumberOfElements() << endl;
  pLCEvent->addCollection(m_LepPairCol, m_outputLepPairCollection.c_str());
  pLCEvent->addCollection(m_PFOsWOLepPairCol, m_outputPFOsWOLepPairCollection.c_str());
  pLCEvent->addCollection(m_IsoLepPairCol, m_outputIsoLepPairCollection.c_str());
  if ( m_fillRootTree ) m_pTTree->Fill();
}

void LeptonPairing::check(EVENT::LCEvent *pLCEvent) {
  (void) pLCEvent;
}


void LeptonPairing::end() {
  if ( m_fillRootTree ) {
    if (m_pTFile != NULL) {
      m_pTFile->cd();
    }
    m_pTTree->Write();

    if (m_pTFile != NULL) {
      m_pTFile->Close();
      delete m_pTFile;
    }
  }

}
