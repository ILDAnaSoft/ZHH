#include "LeptonPairingCombined.h"
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

LeptonPairingCombined aLeptonPairingCombined;

LeptonPairingCombined::LeptonPairingCombined():
  Processor("LeptonPairingCombined"),
  m_nRun(0),
  m_nEvt(0),
  m_IsoLepsInvMass{},
  m_RecoLepsInvMass{},
  m_pTFile(NULL)
{
  _description = "LeptonPairingCombined pairs isolated electrons, muons and taus of opposite charge and adds additional leptons to the list of PFOs or rejects the event" ;

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "IsoElectronCollection",
			  "Name of input isolated electron collection",
			  m_inputIsoElectronCollection,
			  string("ISOElectrons")
			  );

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "IsoMuonCollection",
			  "Name of input isolated muon collection",
			  m_inputIsoMuonCollection,
			  string("ISOMuons")
			  );

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "IsoTauCollection",
			  "Name of input isolated tau collection (ignored if not found)",
			  m_inputIsoTauCollection,
			  string("ISOTaus")
			  );
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "PFOsWOIsoLepCollection",
			  "Name of input PFO collection without isolated leptons",
			  m_inputPFOsWOIsoLepCollection,
			  string("PandoraPFOsWithoutIsoLep")
			  );

  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "LepPairCollection",
			   "Name of output collection of BEST lepton pair, including ISR/FSR recovery",
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

void LeptonPairingCombined::init() {
  streamlog_out(DEBUG0) << "   init called  " << endl ;
  printParameters();

  m_IsoLepsInvMass.resize(3, 0.);
  m_RecoLepsInvMass.resize(3, 0.);

  if ( m_fillRootTree ) {
    streamlog_out(DEBUG0) << "      Creating root file/tree/histograms" << endl ;

    if (m_rootFile.size()) {
      m_pTFile = new TFile(m_rootFile.c_str(), "recreate");
      m_pTTree->SetDirectory(m_pTFile);
    }

    m_pTTree->Branch("event", &m_nEvt, "event/I");
    m_pTTree->Branch("IsoLepsInvMass", &m_IsoLepsInvMass);
    m_pTTree->Branch("RecoLepsInvMass", &m_RecoLepsInvMass);
    m_pTTree->Branch("bestLeptonType", &m_bestLeptonType, "bestLeptonType/I");
    m_pTTree->Branch("bestLeptonDelta", &m_bestLeptonDelta, "bestLeptonDelta/F");

    streamlog_out(DEBUG0) << "      Created root file/tree/histograms" << endl ;
  }

  this->Clear();
  m_nRun = 0;
  m_nEvt = 0;
  streamlog_out(DEBUG) << "   init finished  " << endl;
}

void LeptonPairingCombined::Clear() {
  streamlog_out(DEBUG) << "   Clear called  " << endl;

  m_bestLeptonType = 0;
  m_bestLeptonDelta = 99999.;

  m_IsoLepsInvMass = { 9999., 9999., 9999. };
  m_RecoLepsInvMass.clear();
  m_bestLeptonPair.clear();
  m_bestLeptonPairIDx.clear();
}

void LeptonPairingCombined::processRunHeader( LCRunHeader* run ) { 
  (void) run;
}

float LeptonPairingCombined::evaluateIsoLeptons(EVENT::LCCollection *isoLepCollection,EVENT::LCCollection *pfosWOIsoLepCollection, int leptonType){
  float bestDelta = 99999.;
  int massEntryIndex;
  int InIsoLeps;

  switch (abs(leptonType)) {
    case 11: massEntryIndex = 0; break; // electron
    case 13: massEntryIndex = 1; break; // muon
    case 15: massEntryIndex = 2; break; // tau
  }

  InIsoLeps = isoLepCollection->getNumberOfElements();
  streamlog_out(DEBUG7) << "Pairing iso (type=" << leptonType << ") leptons. Checking n=" << InIsoLeps << " objects" << endl;

  if (InIsoLeps >= 2) {
    float bestMass = 0;
    float pairmass = 0;
    float massDelta;

    Double_t fCosFSRCut = m_doPhotonRecovery ? 0.99 : 99.; // the angle of BS and FSR around the direction of charged lepton
    vector<lcio::ReconstructedParticle*> photons;

    for ( int i_lep1 = 0 ; i_lep1 < InIsoLeps - 1 ; ++i_lep1 ) {
      ReconstructedParticle* lepton1 = static_cast<ReconstructedParticle*>( isoLepCollection->getElementAt( i_lep1 ) );
      ReconstructedParticleImpl *recoLepton1 = new ReconstructedParticleImpl();
      ZHH::doPhotonRecovery(lepton1, pfosWOIsoLepCollection, recoLepton1, fCosFSRCut, lepton1->getType(), photons);
      
      for ( int i_lep2 = i_lep1 + 1 ; i_lep2 < InIsoLeps ; ++i_lep2 ) {
        ReconstructedParticle* lepton2 = static_cast<ReconstructedParticle*>( isoLepCollection->getElementAt( i_lep2 ) );
	ReconstructedParticleImpl *recoLepton2 = new ReconstructedParticleImpl();
	ZHH::doPhotonRecovery(lepton2, pfosWOIsoLepCollection, recoLepton2, fCosFSRCut, lepton2->getType(), photons);
	
        //Check if same type and have opposite charge 
        streamlog_out(DEBUG7) << "Pairing item [" << i_lep1 << "] with [" << i_lep2 << "]" << endl;
        
        if (lepton1->getType() + lepton2->getType() == 0) {
          pairmass = inv_mass(recoLepton1, recoLepton2);
          massDelta = abs(pairmass - m_diLepInvMass);

          streamlog_out(MESSAGE) << "same type, opposite charge; recolep mass = " << pairmass << endl;
          streamlog_out(MESSAGE) << "same type, opposite charge; isolep mass = " << inv_mass(lepton1, lepton2) << endl;

          if (massDelta > bestDelta)
            continue;

          bestMass = pairmass;
          bestDelta = massDelta;

          streamlog_out(DEBUG7) << "new best pairing: deltaM = " << bestDelta << endl;

          if (massDelta < abs(m_IsoLepsInvMass[massEntryIndex] - m_diLepInvMass))
            m_IsoLepsInvMass[massEntryIndex] = bestMass;

          if (massDelta < m_bestLeptonDelta) {
            m_bestLeptonDelta = massDelta;
            m_bestLeptonType = leptonType;
            m_bestLeptonPair = { lepton1, lepton2 };
            m_bestLeptonPairIDx = { i_lep1, i_lep2 };
          }
        }
      }
    }
  }

  return bestDelta;
}

void LeptonPairingCombined::processEvent( EVENT::LCEvent *pLCEvent ) {
  streamlog_out(DEBUG0) << "      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<       processEvent Called     >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl ;
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(DEBUG4) << "" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////    Processing event        " << m_nEvt << "        ////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;
  this->Clear();

  EVENT::LCCollection *IsoElectronCollection;
  EVENT::LCCollection *IsoMuonCollection;
  EVENT::LCCollection *PFOsWOIsoLepCollection;
  EVENT::LCCollection *IsoTauCollection = nullptr;

  try {
    IsoElectronCollection = pLCEvent->getCollection(m_inputIsoElectronCollection);
    IsoMuonCollection = pLCEvent->getCollection(m_inputIsoMuonCollection);
    PFOsWOIsoLepCollection = pLCEvent->getCollection(m_inputPFOsWOIsoLepCollection);
  
    streamlog_out(DEBUG7) << "Isolated electron and muon collections successfully found in event " << m_nEvt << endl;
  } catch( DataNotAvailableException &e ) {
    cerr << "Critical error: One of the given input collections do not exist in event " << m_nEvt << ": " << m_inputIsoElectronCollection << ", " << m_inputIsoMuonCollection << " or " << m_inputPFOsWOIsoLepCollection;
    throw EVENT::Exception("Required collection does not exist");
  }
  streamlog_out(MESSAGE) << " test message " <<endl;
  
  try {
    IsoTauCollection = pLCEvent->getCollection(m_inputIsoTauCollection);
    streamlog_out(DEBUG7) << "Isolated tau collection successfully found in event " << m_nEvt << endl;
  } catch( DataNotAvailableException &e ) {
    streamlog_out(MESSAGE) << "No tau collection found. Taus will not be considered in event " << m_nEvt << endl;
  }

  IMPL::LCCollectionVec* m_LepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  //m_LepPairCol->setSubset( true );
  IMPL::LCCollectionVec* m_IsoLepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  m_IsoLepPairCol->setSubset( true );

  IMPL::LCCollectionVec* m_PFOsWOLepPairCol = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  m_PFOsWOLepPairCol->setSubset( true );
  
  vector<int> leptonTypes;

  if (IsoElectronCollection)
    leptonTypes.push_back(11); // electron
  if (IsoMuonCollection)
    leptonTypes.push_back(13); // muon
  if (IsoTauCollection)
    leptonTypes.push_back(15); // tau

  EVENT::LCCollection *IsoLepCollection = nullptr;

  // from photon ISR and FSR recovery when handling iso electrons
  vector<lcio::ReconstructedParticle*> photons;

  for (int leptonType: leptonTypes) {
    switch (leptonType) {
      case 11: IsoLepCollection = IsoElectronCollection; break;
      case 13: IsoLepCollection = IsoMuonCollection; break;
      case 15: IsoLepCollection = IsoTauCollection; break;
    }

    (void) evaluateIsoLeptons(IsoLepCollection, PFOsWOIsoLepCollection, leptonType);
  }

  // check if a best pairing was found among all isolated leptons for all lepton species
  IsoLepCollection = nullptr;

  if (m_bestLeptonType > 0) {
    int massEntryIndex;
    switch (abs(m_bestLeptonType)) {
      case 11: massEntryIndex = 0; IsoLepCollection = IsoElectronCollection; break; // electron
      case 13: massEntryIndex = 1; IsoLepCollection = IsoMuonCollection; break; // muon
      case 15: massEntryIndex = 2; IsoLepCollection = IsoTauCollection; break; // tau
    }

    Double_t fCosFSRCut = m_doPhotonRecovery ? 0.99 : 99.; // the angle of BS and FSR around the direction of charged lepton

    ReconstructedParticleImpl *recoLepton1 = new ReconstructedParticleImpl();
    ZHH::doPhotonRecovery(m_bestLeptonPair[0], PFOsWOIsoLepCollection, recoLepton1, fCosFSRCut, m_bestLeptonPair[0]->getType(), photons);
    ReconstructedParticleImpl *recoLepton2 = new ReconstructedParticleImpl();
    ZHH::doPhotonRecovery(m_bestLeptonPair[1], PFOsWOIsoLepCollection, recoLepton2, fCosFSRCut, m_bestLeptonPair[1]->getType(), photons);

    m_RecoLepsInvMass[massEntryIndex] = inv_mass(recoLepton1, recoLepton2);

    float E_ph_tot = 0.;
    for (size_t i_ph = 0; i_ph < photons.size(); i_ph++) {
      E_ph_tot += photons[i_ph]->getEnergy();
    }

    streamlog_out(MESSAGE) << "Best lepton pair (type=" << m_bestLeptonType << ") with invariant mass = " << m_IsoLepsInvMass[massEntryIndex] << " (before pairing: " << m_RecoLepsInvMass[massEntryIndex] <<")" << endl;      
    streamlog_out(MESSAGE) << "  Recovered " << photons.size() << " photons with an energy of " << E_ph_tot << endl;

    m_IsoLepPairCol->addElement(m_bestLeptonPair[0]);
    m_IsoLepPairCol->addElement(m_bestLeptonPair[1]);

    m_LepPairCol->addElement(recoLepton1);
    m_LepPairCol->addElement(recoLepton2);

    m_LepPairCol->parameters().setValue("PairedType", m_bestLeptonType);
    m_LepPairCol->parameters().setValues("PairedLeptonIDx", m_bestLeptonPairIDx);
    m_LepPairCol->parameters().setValue("IsoLepsInvMass", m_IsoLepsInvMass[massEntryIndex]);
    m_LepPairCol->parameters().setValue("RecoLepsInvMass", m_RecoLepsInvMass[massEntryIndex]);
  } else {
    streamlog_out(DEBUG7) << "No lepton pair found" << endl;
  }

  //Select lepton pair
  //(add pre invmass to root tree)
  //Do FSR and BS recovery
  //(add post invmass to root tree)
  //Add reco lepton pair to output collection
  //Add all other pfos (including additional leptons) that are not from lepton pair or photons to output

  for (int leptonType: leptonTypes) {
    switch (leptonType) {
      case 11: IsoLepCollection = IsoElectronCollection; break;
      case 13: IsoLepCollection = IsoMuonCollection; break;
      case 15: IsoLepCollection = IsoTauCollection; break;
    }

    int InIsoLeps = IsoLepCollection->getNumberOfElements();

    for (int i_lep = 0 ; i_lep < InIsoLeps; ++i_lep) {
      bool isFromPair = false;
      ReconstructedParticle* lepton = static_cast<ReconstructedParticle*>(IsoLepCollection->getElementAt(i_lep));

      if (m_bestLeptonType == leptonType) {
        for (auto leptonfrompair: m_bestLeptonPair) {
          if (lepton == leptonfrompair)
            isFromPair = true;
        }
      }

      if (!isFromPair) {
        m_PFOsWOLepPairCol->addElement(lepton);
        streamlog_out(DEBUG7) << "Adding lepton (type=" << lepton->getType() << ") [" << i_lep << "] to PFOsWOLepPair because it couldn't be paired" << endl;
      }
    }
  }

  for (int i = 0; i < PFOsWOIsoLepCollection->getNumberOfElements(); i++) {
    bool isBkgPhoton = false;
    ReconstructedParticle* pfo = static_cast<ReconstructedParticle*>(PFOsWOIsoLepCollection->getElementAt(i));
    for (auto photon: photons) {
      if (pfo == photon)
        isBkgPhoton = true;
    }
    if (!isBkgPhoton)
      m_PFOsWOLepPairCol->addElement(pfo);
  }

  streamlog_out(DEBUG7) << "nphotons " << photons.size() << endl;
  streamlog_out(DEBUG7) << "nselected " << m_LepPairCol->getNumberOfElements() << endl;
  streamlog_out(DEBUG7) << "npfos " << PFOsWOIsoLepCollection->getNumberOfElements() << endl;
  streamlog_out(DEBUG7) << "nnewpfos " << m_PFOsWOLepPairCol->getNumberOfElements() << endl;

  pLCEvent->addCollection(m_LepPairCol, m_outputLepPairCollection.c_str());
  pLCEvent->addCollection(m_IsoLepPairCol, m_outputIsoLepPairCollection.c_str());
  pLCEvent->addCollection(m_PFOsWOLepPairCol, m_outputPFOsWOLepPairCollection.c_str());

  if ( m_fillRootTree ) m_pTTree->Fill();

  setReturnValue("pairFound", m_bestLeptonType > 0);
}

void LeptonPairingCombined::check(EVENT::LCEvent *pLCEvent) {
  (void) pLCEvent;
}


void LeptonPairingCombined::end() {
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
