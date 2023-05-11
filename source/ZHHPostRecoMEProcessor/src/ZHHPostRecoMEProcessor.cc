#include "ZHHPostRecoProcessor.h"
#include <iostream>
#include <vector>
#include <string>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;
using namespace std ;
using namespace lcme;

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

ZHHPostRecoMEProcessor aZHHPostRecoMEProcessor ;

ZHHPostRecoMEProcessor::ZHHPostRecoMEProcessor() :

  Processor("ZHHPostRecoMEProcessor"),
  m_nRun(0),
  m_nEvt(0)

{

	_description = "ZHHPostRecoMEProcessor writes relevant observables to root-file " ;

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				"LepPairCollection",
				"Name of reconstructed lepton pair collection",
				m_inputLepPairCollection,
				std::string("LeptonPair")
				);

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				"HiggsPairCollection",
				"Name of reconstructed higss pair collection",
				m_inputHiggsPairCollection,
				std::string("HiggsPair")
				);

  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				 "PreSelectionCollection",
				 "preselection collection",
				 m_inputPreSelectionCollection,
				 std::string("preselection")
				 );

  registerOutputCollection(LCIO::MCPARTICLE,
				 "MCTrueCollection",
				 "preselection collection",
				 m_inputPreSelectionCollection,
				 std::string("preselection")
				 );

	registerProcessorParameter("ZDecayMode",
        "MEM processor mode of ZDecay",
        m_ZDecayMode,
        int(4)
        );

	registerProcessorParameter("HiggsMass",
        "assumed Hmass",
        m_Hmass,
        float(125.)
        );
	
  registerProcessorParameter("outputFilename",
        "name of output root file",
        m_outputFile,
        std::string("output.root")
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

  m_pTTree->Branch("true_sigma", &m_true_sigma, "true_sigma/F") ;
  m_pTTree->Branch("true_sigmall", &m_true_sigmall, "true_sigmall/F") ;
  m_pTTree->Branch("true_sigmalr", &m_true_sigmalr, "true_sigmalr/F") ;
  m_pTTree->Branch("true_sigmarl", &m_true_sigmarl, "true_sigmarl/F") ;
  m_pTTree->Branch("true_sigmarr", &m_true_sigmarr, "true_sigmarr/F") ;

  m_pTTree->Branch("reco_sigma", &m_reco_sigma, "reco_sigma/F") ;
  m_pTTree->Branch("reco_sigmall", &m_reco_sigmall, "reco_sigmall/F") ;
  m_pTTree->Branch("reco_sigmalr", &m_reco_sigmalr, "reco_sigmalr/F") ;
  m_pTTree->Branch("reco_sigmarl", &m_reco_sigmarl, "reco_sigmarl/F") ;
  m_pTTree->Branch("reco_sigmarr", &m_reco_sigmarr, "reco_sigmarr/F") ;

  streamlog_out(DEBUG) << "   init finished  " << std::endl;

  _zzh = new LCMEZZH("LCMEZHH", "ZHH", m_Hmass,-1.,1.);
  _zhh->SetZDecayMode(m_ZDecayMode); // 4 (internal mapping) -> (13) PDG, muon 
}

void PreSelection::Clear() 
{
  streamlog_out(DEBUG) << "   Clear called  " << std::endl;
  
  m_passed_preselection = 0;
  m_h1_decay_pdg = 0;
	m_h2_decay_pdg = 0;

  m_true_sigma = 0;
  m_true_sigmall = 0;
  m_true_sigmalr = 0;
  m_true_sigmarl = 0;
  m_true_sigmarr = 0;
  
  m_reco_sigma = 0;
  m_reco_sigmall = 0;
  m_reco_sigmalr = 0;
  m_reco_sigmarl = 0;
  m_reco_sigmarr = 0;
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

  // Reco
  LCCollection *inputLepPairCollection{};
  LCCollection *inputHiggsPairCollection{};
  LCCollectionVec *preselectioncol{};
  bool passed;

  // True
  LCCollection *inputMCTrueCollection{};

  try {
    // Reco
    streamlog_out(DEBUG0) << "        getting lepton pair collection: " << m_inputLepPairCollection << std::endl ;
    inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );

    streamlog_out(DEBUG0) << "        getting higgs pair collection: " << m_inputHiggsPairCollection << std::endl ;
    inputHiggsPairCollection = pLCEvent->getCollection( m_inputHiggsPairCollection );

    streamlog_out(DEBUG0) << "        getting preselection_passed collection: " << m_inputPreSelectionCollection << std::endl ;
    preselectioncol = pLCEvent->getCollection( m_inputPreSelectionCollection );

    passed = (bool)(preselectioncol->parameters().getValue("isPassed"));

    // Assume HiggsPair and LepPair collections always have two items
    ReconstructedParticle* l1_reco = (ReconstructedParticle*) inputLepPairCollection->getElementAt(0);
    ReconstructedParticle* l2_reco = (ReconstructedParticle*) inputLepPairCollection->getElementAt(1);
    
    ReconstructedParticle* h1_reco = (ReconstructedParticle*) inputHiggsPairCollection->getElementAt(0);
    ReconstructedParticle* h2_reco = (ReconstructedParticle*) inputHiggsPairCollection->getElementAt(1);

    l1_reco_lortz = TLorentzVector( l1_reco->getMomentum()[ 0 ] , l1_reco->getMomentum()[ 1 ] , l1_reco->getMomentum()[ 2 ] , l1_reco->getEnergy() );
    l2_reco_lortz = TLorentzVector( l2_reco->getMomentum()[ 0 ] , l2_reco->getMomentum()[ 1 ] , l2_reco->getMomentum()[ 2 ] , l2_reco->getEnergy() );
    
    h1_reco_lortz = TLorentzVector( h1_reco->getMomentum()[ 0 ] , h1_reco->getMomentum()[ 1 ] , h1_reco->getMomentum()[ 2 ] , h1_reco->getEnergy() );
    h2_reco_lortz = TLorentzVector( h2_reco->getMomentum()[ 0 ] , h2_reco->getMomentum()[ 1 ] , h2_reco->getMomentum()[ 2 ] , h2_reco->getEnergy() );

    TLorentzVector lortz_reco[4] = {l1_reco_lortz, l2_reco_lortz, h1_reco_lortz, h2_reco_lortz};

    _zhh->SetMomentumFinal(lortz_reco);

    Int_t vHelLL[2] = {-1,-1};
    Int_t vHelLR[2] = {-1,1};
    Int_t vHelRL[2] = {1,-1};
    Int_t vHelRR[2] = {1,1};
    m_reco_sigma_ll = _zhh->GetMatrixElement2(vHelLL);
    m_reco_sigma_lr = _zhh->GetMatrixElement2(vHelLR);
    m_reco_sigma_rl = _zhh->GetMatrixElement2(vHelRL);
    m_reco_sigma_rr = _zhh->GetMatrixElement2(vHelRR);
    m_reco_sigma = _zhh->GetMatrixElement2();

    // True
    streamlog_out(DEBUG0) << "        getting true MC collection: " << m_inputMCTrueCollection << std::endl ;
    inputMCTrueCollection = pLCEvent->getCollection( m_inputMCTrueCollection );

    

    
    
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
  }

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
