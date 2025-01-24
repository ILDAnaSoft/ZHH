#include "ZinvisibleErrorAnalysis.h"
#include <stdlib.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include "TH1F.h"
#include "TH2F.h"
#include "TF1.h"
#include "TPaveStats.h"


// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#ifdef MARLIN_USE_AIDA
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
//#include <AIDA/IHistogram1D.h>
#endif // MARLIN_USE_AIDA


using namespace lcio ;
using namespace marlin ;
using namespace std ;

ZinvisibleErrorAnalysis aZinvisibleErrorAnalysis ;

ZinvisibleErrorAnalysis::ZinvisibleErrorAnalysis() : Processor("ZinvisibleErrorAnalysis"),
						     m_nRun(0),
						     m_nEvt(0),
						     m_nRunSum(0),
						     m_nEvtSum(0),
						     m_nJets(0),
						     m_EnergyZinv(0.),
						     m_ThetaZinv(0.),
						     m_PhiZinv(0.),
						     m_PxZinv(0.),
						     m_PyZinv(0.),
						     m_PzZinv(0.),
						     m_EnergyZvv(0.),
						     m_ThetaZvv(0.),
						     m_PhiZvv(0.),
						     m_PxZvv(0.),
						     m_PyZvv(0.),
						     m_PzZvv(0.),
						     m_EnergyResidual(0.),
						     m_ThetaResidual(0.),
						     m_PhiResidual(0.),
						     m_PxResidual(0.),
						     m_PyResidual(0.),
						     m_PzResidual(0.)


{

  // modify processor description
  _description = "ZinvisibleErrorAnalysis calculates (unnormalised) residuals for Z->invisible fit object in ZHH events" ;


  // register steering parameters: name, description, class-variable, default value


  // Inputs: MC-particles, Reco-particles, the link between the two

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "InputJetCollection" ,
			   "Name of the input Reconstructed Jet collection"  ,
			   m_inputJetCollection,
			   string("inputJets")
			   );

  registerInputCollection( LCIO::MCPARTICLE,
			   "MCParticleCollection" ,
			   "Name of the MCParticle collection"  ,
			   _MCParticleCollectionName ,
			   string("MCParticlesSkimmed")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoParticleCollection" ,
			   "Name of the ReconstructedParticles input collection"  ,
			   _recoParticleCollectionName ,
			   string("PandoraPFOs")
			   );
  
  registerProcessorParameter( "outputFilename",
			      "name of output root file",
			      m_outputFile,
			      std::string("")
			      );
  
 // Outputs: Residuals  (not normalised)                                                                                                                         
 registerOutputCollection( LCIO::LCFLOATVEC,
                           "ZinvisibleResidualsOutputCollection",
                           "Output ZinvisibleResiduals (E, theta, phi)  Collection" ,
                           _OutZinvisibleResidualsCol,
                           string("ZinvisibleResiduals"));

}


void ZinvisibleErrorAnalysis::init()
{

  streamlog_out(MESSAGE6) << "   init called  " << endl ;
  this->Clear();
  m_nRun = 0;
  m_nEvt = 0;
  m_nRunSum = 0;
  m_nEvtSum = 0;

  m_pTFile = new TFile(m_outputFile.c_str(),"recreate");

  m_pTTree = new TTree("eventTree","eventTree");
  m_pTTree->SetDirectory(m_pTFile);
  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("nJets",&m_nJets,"nJets/I") ;
  m_pTTree->Branch("EnergyZinv",&m_EnergyZinv,"EnergyZinv/F") ;
  m_pTTree->Branch("ThetaZinv",&m_ThetaZinv,"ThetaZinv/F") ;
  m_pTTree->Branch("PhiZinv",&m_PhiZinv,"PhiZinv/F") ;
  m_pTTree->Branch("PxZinv",&m_PxZinv,"PxZinv/F") ;
  m_pTTree->Branch("PyZinv",&m_PyZinv,"PyZinv/F") ;
  m_pTTree->Branch("PzZinv",&m_PzZinv,"PzZinv/F") ;
  m_pTTree->Branch("EnergyZvv",&m_EnergyZvv,"EnergyZvv/F") ;
  m_pTTree->Branch("ThetaZvv",&m_ThetaZvv,"ThetaZvv/F") ;
  m_pTTree->Branch("PhiZvv",&m_PhiZvv,"PhiZvv/F") ;
  m_pTTree->Branch("PxZvv",&m_PxZvv,"PxZvv/F") ;
  m_pTTree->Branch("PyZvv",&m_PyZvv,"PyZvv/F") ;
  m_pTTree->Branch("PzZvv",&m_PzZvv,"PzZvv/F") ;
  m_pTTree->Branch("EnergyResidual",&m_EnergyResidual,"EnergyResidual/F") ;
  m_pTTree->Branch("ThetaResidual",&m_ThetaResidual,"ThetaResidual/F") ;
  m_pTTree->Branch("PhiResidual",&m_PhiResidual,"PhiResidual/F") ;
  m_pTTree->Branch("PxResidual",&m_PxResidual,"PxResidual/F") ;
  m_pTTree->Branch("PyResidual",&m_PyResidual,"PyResidual/F") ;
  m_pTTree->Branch("PzResidual",&m_PzResidual,"PzResidual/F") ;
  
  streamlog_out(MESSAGE) << "   init finished  " << std::endl;
	
}

void ZinvisibleErrorAnalysis::Clear()
{
  streamlog_out(MESSAGE) << "   Clear called  " << endl;
  m_nJets = 0;
  m_EnergyZinv = 0;
  m_ThetaZinv = 0;
  m_PhiZinv = 0;
  m_PxZinv = 0;
  m_PyZinv = 0;
  m_PzZinv = 0;
  m_EnergyZvv = 0;
  m_ThetaZvv = 0;
  m_PhiZvv = 0;
  m_PxZvv = 0;
  m_PyZvv = 0;
  m_PzZvv = 0;
  m_EnergyResidual = 0;
  m_ThetaResidual = 0;
  m_PhiResidual = 0;
  m_PxResidual = 0;
  m_PyResidual = 0;
  m_PzResidual = 0;
 

}

void ZinvisibleErrorAnalysis::processRunHeader()
{
  m_nRun++ ;
}


void ZinvisibleErrorAnalysis::processEvent( LCEvent* pLCEvent)
{
  this->Clear();
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////Processing event: " << m_nEvt << "////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;

  LCCollection *inputJetCollection = NULL;
  LCCollection *inputMCParticleCollection = NULL;
  IMPL::LCCollectionVec* OutZinvisibleResidualsCol = NULL;
  OutZinvisibleResidualsCol = new IMPL::LCCollectionVec(LCIO::LCFLOATVEC);
  OutZinvisibleResidualsCol->setSubset( true );

  try {
    streamlog_out(MESSAGE) << "     getting jet collection: " << m_inputJetCollection << std::endl;
    inputJetCollection= pLCEvent->getCollection( m_inputJetCollection );
    streamlog_out(MESSAGE) << "  getting mc particle collection: " << _MCParticleCollectionName << std::endl ;
    inputMCParticleCollection = pLCEvent->getCollection( _MCParticleCollectionName );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
    return;
  }
  
  m_nJets = inputJetCollection->getNumberOfElements();
  if ( m_nJets != 4 ) {
    m_pTTree->Fill();
    return;
  }

  //calculate 4-momentum of Z->invisible (reco)
  ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
  std::vector<ReconstructedParticle*> Jets{};
  for (unsigned int i_jet = 0; i_jet < m_nJets; i_jet++) {
    ReconstructedParticle* jet = dynamic_cast<ReconstructedParticle*>( inputJetCollection->getElementAt( i_jet ) );
    seenFourMomentum += ROOT::Math::PxPyPzEVector(jet->getMomentum()[0],jet->getMomentum()[1],jet->getMomentum()[2], jet->getEnergy());
  }
  ROOT::Math::PxPyPzMVector ZinvFourMomentum(-seenFourMomentum.Px(), -seenFourMomentum.Py(), -seenFourMomentum.Pz(),91.1880); // PDG average in 2024 review
  
  //calculate 4-momentum of Z->vv (true)
  ROOT::Math::PxPyPzEVector ZvvFourMomentum(0.,0.,0.,0.);
  for (int i_nu = 8; i_nu < 10; i_nu++) {
    MCParticle* nu = (MCParticle*) inputMCParticleCollection->getElementAt(i_nu);
    int pdg = nu->getPDG();
    if (std::abs(pdg) != 12 && std::abs(pdg) != 14 && std::abs(pdg) != 16) {
      streamlog_out(MESSAGE) << "MCParticle is not a neutrino but has PDG: " << pdg << endl;
      m_pTTree->Fill();
      return;
    }
    ZvvFourMomentum += ROOT::Math::PxPyPzEVector(nu->getMomentum()[0],nu->getMomentum()[1],nu->getMomentum()[2], nu->getEnergy());
  }  
  //calculate residuals
  LCFloatVec *ZinvisibleResidual = new LCFloatVec;

  m_EnergyZinv = ZinvFourMomentum.E();
  m_ThetaZinv = ZinvFourMomentum.Theta();  
  m_PhiZinv = ZinvFourMomentum.Phi();  
  m_PxZinv = ZinvFourMomentum.Px();  
  m_PyZinv = ZinvFourMomentum.Py();  
  m_PzZinv = ZinvFourMomentum.Pz();  
  m_EnergyZvv = ZvvFourMomentum.E();
  m_ThetaZvv = ZvvFourMomentum.Theta(); 
  m_PhiZvv = ZvvFourMomentum.Phi();
  m_PxZvv = ZvvFourMomentum.Px();
  m_PyZvv = ZvvFourMomentum.Py();
  m_PzZvv = ZvvFourMomentum.Pz();

  m_EnergyResidual = m_EnergyZinv-m_EnergyZvv;
  m_ThetaResidual = m_ThetaZinv-m_ThetaZvv;
  m_PhiResidual = m_PhiZinv-m_PhiZvv;
  m_PxResidual = m_PxZinv-m_PxZvv;
  m_PyResidual = m_PyZinv-m_PyZvv;
  m_PzResidual = m_PzZinv-m_PzZvv;

  ZinvisibleResidual->push_back(m_EnergyResidual);
  ZinvisibleResidual->push_back(m_ThetaResidual);
  ZinvisibleResidual->push_back(m_PhiResidual);
  ZinvisibleResidual->push_back(m_PxResidual);
  ZinvisibleResidual->push_back(m_PyResidual);
  ZinvisibleResidual->push_back(m_PzResidual);

  OutZinvisibleResidualsCol->addElement(ZinvisibleResidual);
  pLCEvent->addCollection(OutZinvisibleResidualsCol, _OutZinvisibleResidualsCol.c_str() );
      
  m_nEvtSum++;
  streamlog_out(MESSAGE) << "Energy residual: " << m_EnergyZinv << " - " << m_EnergyZvv << " = " << m_EnergyResidual << endl;
  streamlog_out(MESSAGE) << "Theta residual: " << m_ThetaZinv << " - " << m_ThetaZvv << " = " << m_ThetaResidual << endl;
  streamlog_out(MESSAGE) << "Phi residual: " << m_PhiZinv << " - " << m_PhiZvv << " = " << m_PhiResidual << endl;
  streamlog_out(MESSAGE) << "Px residual: " << m_PxZinv << " - " << m_PxZvv << " = " << m_PxResidual << endl;
  streamlog_out(MESSAGE) << "Py residual: " << m_PyZinv << " - " << m_PyZvv << " = " << m_PyResidual << endl;
  streamlog_out(MESSAGE) << "Pz residual: " << m_PzZinv << " - " << m_PzZvv << " = " << m_PzResidual << endl;
  
  m_pTTree->Fill();
}

void ZinvisibleErrorAnalysis::check()
{

}

void ZinvisibleErrorAnalysis::end()
{
streamlog_out(MESSAGE) << "writing root file" << endl;
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;
}
