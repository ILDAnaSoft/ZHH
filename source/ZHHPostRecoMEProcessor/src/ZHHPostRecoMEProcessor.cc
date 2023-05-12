#include "ZHHPostRecoMEProcessor.h"
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
				 m_inputMCTrueCollection,
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

void ZHHPostRecoMEProcessor::init()
{
  streamlog_out(DEBUG) << "   init called  " << std::endl;
  this->Clear();

  Double_t pol_e = this->parameters()->getFloatVal("beamPol1");
  Double_t pol_p = this->parameters()->getFloatVal("beamPol2");

  m_nRun = 0;
  m_nEvt = 0;

  m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
  m_pTTree = new TTree("eventTree","eventTree");
  m_pTTree->SetDirectory(m_pTFile);

  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("passed_preselection", &m_passed_preselection, "passed_preselection/I");
  m_pTTree->Branch("m_true_h1_decay_pdg", &m_true_h1_decay_pdg, "m_true_h1_decay_pdg/I");
  m_pTTree->Branch("m_true_h2_decay_pdg", &m_true_h2_decay_pdg, "m_true_h2_decay_pdg/I");

  m_pTTree->Branch("true_sigma", &m_true_sigma, "true_sigma/F");
  m_pTTree->Branch("true_sigmall", &m_true_sigmall, "true_sigmall/F");
  m_pTTree->Branch("true_sigmalr", &m_true_sigmalr, "true_sigmalr/F");
  m_pTTree->Branch("true_sigmarl", &m_true_sigmarl, "true_sigmarl/F");
  m_pTTree->Branch("true_sigmarr", &m_true_sigmarr, "true_sigmarr/F");
  m_pTTree->Branch("true_mz", &m_true_mz, "true_mz/F");
  m_pTTree->Branch("true_mhh", &m_true_mhh, "true_mhh/F");
  m_pTTree->Branch("true_mzhh", &m_true_mzhh, "true_mzhh/F");
  m_pTTree->Branch("true_phi", &m_true_phi, "true_phi/F");
  m_pTTree->Branch("true_costheta", &m_true_costheta, "true_costheta/F");

  m_pTTree->Branch("reco_sigma", &m_reco_sigma, "reco_sigma/F");
  m_pTTree->Branch("reco_sigmall", &m_reco_sigmall, "reco_sigmall/F");
  m_pTTree->Branch("reco_sigmalr", &m_reco_sigmalr, "reco_sigmalr/F");
  m_pTTree->Branch("reco_sigmarl", &m_reco_sigmarl, "reco_sigmarl/F");
  m_pTTree->Branch("reco_sigmarr", &m_reco_sigmarr, "reco_sigmarr/F");
  m_pTTree->Branch("reco_mz", &m_reco_mz, "reco_mz/F");
  m_pTTree->Branch("reco_mhh", &m_reco_mhh, "reco_mhh/F");
  m_pTTree->Branch("reco_mzhh", &m_reco_mzhh, "reco_mzhh/F");
  m_pTTree->Branch("reco_phi", &m_reco_phi, "reco_phi/F");
  m_pTTree->Branch("reco_costheta", &m_reco_costheta, "reco_costheta/F");

  streamlog_out(DEBUG) << "   init finished  " << std::endl;

  _zhh = new LCMEZHH("LCMEZHH", "ZHH", m_Hmass, pol_e, pol_p);
  _zhh->SetZDecayMode(m_ZDecayMode); // 4 (internal mapping) -> (13) PDG, muon 
}

void ZHHPostRecoMEProcessor::Clear() 
{
  streamlog_out(DEBUG) << "   Clear called  " << std::endl;
  
  m_passed_preselection = 0;
  m_true_h1_decay_pdg = 0;
	m_true_h2_decay_pdg = 0;

  m_true_sigma    = 0.;
  m_true_sigmall  = 0.;
  m_true_sigmalr  = 0.;
  m_true_sigmarl  = 0.;
  m_true_sigmarr  = 0.;
  m_true_mz       = 0.;
  m_true_mhh      = 0.;
  m_true_mzhh     = 0.;
  m_true_phi      = 0.;
  m_true_costheta = 0.;
  
  m_reco_sigma    = 0.;
  m_reco_sigmall  = 0.;
  m_reco_sigmalr  = 0.;
  m_reco_sigmarl  = 0.;
  m_reco_sigmarr  = 0.;
  m_reco_mz       = 0.;
  m_reco_mhh      = 0.;
  m_reco_mzhh     = 0.;
  m_reco_phi      = 0.;
  m_reco_costheta = 0.;
}
void ZHHPostRecoMEProcessor::processRunHeader( LCRunHeader*  /*run*/) { 
  m_nRun++ ;
} 

void ZHHPostRecoMEProcessor::processEvent( EVENT::LCEvent *pLCEvent )
{
  this->Clear();
  
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(DEBUG) << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << endl;

  // Helicity combinations
  Int_t vHelLL[2] = {-1,-1};
  Int_t vHelLR[2] = {-1, 1};
  Int_t vHelRL[2] = { 1,-1};
  Int_t vHelRR[2] = { 1, 1};

  // Reco
  LCCollection *inputLepPairCollection{};
  LCCollection *inputHiggsPairCollection{};
  LCCollection *preselectioncol{};

  // True
  LCCollection *inputMCTrueCollection{};

  try {
    // Reco
    streamlog_out(DEBUG) << "        getting lepton pair collection: " << m_inputLepPairCollection << std::endl ;
    inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );

    streamlog_out(DEBUG) << "        getting higgs pair collection: " << m_inputHiggsPairCollection << std::endl ;
    inputHiggsPairCollection = pLCEvent->getCollection( m_inputHiggsPairCollection );

    streamlog_out(DEBUG) << "        getting preselection_passed collection: " << m_inputPreSelectionCollection << std::endl ;
    preselectioncol = pLCEvent->getCollection( m_inputPreSelectionCollection );

    m_passed_preselection = preselectioncol->parameters().getIntVal("isPassed");

    // Can only reconstruct if exactly two reco leptons and higgses
    if (inputLepPairCollection->getNumberOfElements() == 2 && inputHiggsPairCollection->getNumberOfElements() == 2) {
      ReconstructedParticle* l1_reco = static_cast<ReconstructedParticle*>(inputLepPairCollection->getElementAt(0));
      ReconstructedParticle* l2_reco = static_cast<ReconstructedParticle*>(inputLepPairCollection->getElementAt(1));
      
      ReconstructedParticle* h1_reco = static_cast<ReconstructedParticle*>(inputHiggsPairCollection->getElementAt(0));
      ReconstructedParticle* h2_reco = static_cast<ReconstructedParticle*>(inputHiggsPairCollection->getElementAt(1));

      TLorentzVector l1_reco_lortz = TLorentzVector( l1_reco->getMomentum()[ 0 ] , l1_reco->getMomentum()[ 1 ] , l1_reco->getMomentum()[ 2 ] , l1_reco->getEnergy() );
      TLorentzVector l2_reco_lortz = TLorentzVector( l2_reco->getMomentum()[ 0 ] , l2_reco->getMomentum()[ 1 ] , l2_reco->getMomentum()[ 2 ] , l2_reco->getEnergy() );
      
      TLorentzVector h1_reco_lortz = TLorentzVector( h1_reco->getMomentum()[ 0 ] , h1_reco->getMomentum()[ 1 ] , h1_reco->getMomentum()[ 2 ] , h1_reco->getEnergy() );
      TLorentzVector h2_reco_lortz = TLorentzVector( h2_reco->getMomentum()[ 0 ] , h2_reco->getMomentum()[ 1 ] , h2_reco->getMomentum()[ 2 ] , h2_reco->getEnergy() );

      TLorentzVector reco_lortz[4] = {l1_reco_lortz, l2_reco_lortz, h1_reco_lortz, h2_reco_lortz};

      _zhh->SetMomentumFinal(reco_lortz);

      m_reco_sigmall  = _zhh->GetMatrixElement2(vHelLL);
      m_reco_sigmalr  = _zhh->GetMatrixElement2(vHelLR);
      m_reco_sigmarl  = _zhh->GetMatrixElement2(vHelRL);
      m_reco_sigmarr  = _zhh->GetMatrixElement2(vHelRR);
      m_reco_sigma    = _zhh->GetMatrixElement2();

      m_reco_mz       = TMath::Sqrt(_zhh->GetQ2Z());
      m_reco_mhh      = TMath::Sqrt(_zhh->GetQ2HH());
      m_reco_mzhh     = TMath::Sqrt(_zhh->GetQ2ZHH());

      m_reco_phi      = _zhh->GetPhi();
      m_reco_costheta = _zhh->GetCosTheta();
    }

    // True
    streamlog_out(DEBUG) << "        getting true MC collection: " << m_inputMCTrueCollection << std::endl ;
    inputMCTrueCollection = pLCEvent->getCollection( m_inputMCTrueCollection );

    MCParticle *mcPart_l1 = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt( 8));
    MCParticle *mcPart_l2 = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt( 9));
    
    MCParticle *mcPart_h1 = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt(10));
    MCParticle *mcPart_h2 = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt(11));

    TLorentzVector l1_true_lortz = TLorentzVector(mcPart_l1->getMomentum(), mcPart_l1->getEnergy());
    TLorentzVector l2_true_lortz = TLorentzVector(mcPart_l2->getMomentum(), mcPart_l2->getEnergy());
    TLorentzVector h1_true_lortz = TLorentzVector(mcPart_h1->getMomentum(), mcPart_h1->getEnergy());
    TLorentzVector h2_true_lortz = TLorentzVector(mcPart_h2->getMomentum(), mcPart_h2->getEnergy());

    TLorentzVector true_lortz[4] = {l1_true_lortz, l2_true_lortz, h1_true_lortz, h2_true_lortz};

    _zhh->SetMomentumFinal(true_lortz);

    m_true_sigmall = _zhh->GetMatrixElement2(vHelLL);
    m_true_sigmalr = _zhh->GetMatrixElement2(vHelLR);
    m_true_sigmarl = _zhh->GetMatrixElement2(vHelRL);
    m_true_sigmarr = _zhh->GetMatrixElement2(vHelRR);
    m_true_sigma   = _zhh->GetMatrixElement2();

    m_true_mz   = TMath::Sqrt(_zhh->GetQ2Z());
    m_true_mhh  = TMath::Sqrt(_zhh->GetQ2HH());
    m_true_mzhh = TMath::Sqrt(_zhh->GetQ2ZHH());

    m_true_phi      = _zhh->GetPhi();
    m_true_costheta = _zhh->GetCosTheta();

    MCParticle *mcPart_h1_decay = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt(12));
    m_true_h1_decay_pdg = abs(mcPart_h1_decay->getPDG());

    MCParticle *mcPart_h2_decay = dynamic_cast<MCParticle*>(inputMCTrueCollection->getElementAt(14));
    m_true_h2_decay_pdg = abs(mcPart_h2_decay->getPDG());
    
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
  }

  m_pTTree->Fill();


}

void ZHHPostRecoMEProcessor::check()
{
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void ZHHPostRecoMEProcessor::end()
{
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;
}
