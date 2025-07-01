#include "bHadronAnalysis.h"
#include <iostream>
#include <vector>
#include <string>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <IMPL/MCParticleImpl.h>

using namespace lcio ;
using namespace marlin ;
using namespace std ;

bHadronAnalysis abHadronAnalysis ;

bHadronAnalysis::bHadronAnalysis() : Processor("bHadronAnalysis"),
				     m_nRun(0),
				     m_nEvt(0),
				     m_bhadronEnergy(0.),
				     m_bhadronTheta(0.),
				     m_bhadronPhi(0.),
				     m_bhadronfromZEnergy(0.),
				     m_bhadronfromZTheta(0.),
				     m_bhadronfromZPhi(0.),
				     m_bhadronfromHEnergy(0.),
				     m_bhadronfromHTheta(0.),
				     m_bhadronfromHPhi(0.),
				     m_bhadronBeta(0.),
				     m_bhadronGamma(0.),
				     m_bhadronBetaGamma(0.),
				     m_bdecaylength(0.),
				     m_cdecaylength(0.),
				     m_longestbdecaylength(0.),
				     m_2ndlongestbdecaylength(0.),
				     m_3rdlongestbdecaylength(0.),
				     m_4thlongestbdecaylength(0.)
{
  
  _description = "bHadronAnalysis identifies Higgs decay mode and finds ISR in generator level" ;
  
  registerInputCollection( 	LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);
  
  registerProcessorParameter(     "outputFilename",
				  "name of output root file",
				  m_outputFile,
				  std::string("")
				  );
}
  
void bHadronAnalysis::init()
{

  streamlog_out(DEBUG) << "   init called  " << std::endl ;
  this->Clear();
  
  m_nRun = 0;
  m_nEvt = 0;

  m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
  m_pTTree = new TTree("eventTree","eventTree");
  m_pTTree->SetDirectory(m_pTFile);
  
  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("bhadronEnergy", &m_bhadronEnergy, "bhadronEnergy/F");
  m_pTTree->Branch("bhadronTheta", &m_bhadronTheta, "bhadronTheta/F");
  m_pTTree->Branch("bhadronPhi", &m_bhadronPhi, "bhadronPhi/F");	  
  m_pTTree->Branch("bhadronfromZEnergy", &m_bhadronfromZEnergy, "bhadronfromZEnergy/F");
  m_pTTree->Branch("bhadronfromZTheta", &m_bhadronfromZTheta, "bhadronfromZTheta/F");
  m_pTTree->Branch("bhadronfromZPhi", &m_bhadronfromZPhi, "bhadronfromZPhi/F");
  m_pTTree->Branch("bhadronfromHEnergy", &m_bhadronfromHEnergy, "bhadronfromHEnergy/F");
  m_pTTree->Branch("bhadronfromHTheta", &m_bhadronfromHTheta, "bhadronfromHTheta/F");
  m_pTTree->Branch("bhadronfromHPhi", &m_bhadronfromHPhi, "bhadronfromHPhi/F");  
  m_pTTree->Branch("bhadronBeta", &m_bhadronBeta, "bhadronBeta/F");
  m_pTTree->Branch("bhadronGamma", &m_bhadronGamma, "bhadronGamma/F");
  m_pTTree->Branch("bhadronBetaGamma", &m_bhadronBetaGamma, "bhadronBetaGamma/F");	  
  m_pTTree->Branch("bdecaylength", &m_bdecaylength, "bdecaylength/F");
  m_pTTree->Branch("cdecaylength", &m_cdecaylength, "cdecaylength/F");
  m_pTTree->Branch("longestbdecaylength", &m_longestbdecaylength, "longestbdecaylength/F");
  m_pTTree->Branch("2ndlongestbdecaylength", &m_2ndlongestbdecaylength, "2ndlongestbdecaylength/F");
  m_pTTree->Branch("3rdlongestbdecaylength", &m_3rdlongestbdecaylength, "3rdlongestbdecaylength/F");
  m_pTTree->Branch("4thlongestbdecaylength", &m_4thlongestbdecaylength, "4thlongestbdecaylength/F");



  //m_pTTree->Branch("nHdecayTob",&m_nHdecayTob,"nHdecayTob/I") ;
  //m_pTTree->Branch("nZdecayTob",&m_nZdecayTob,"nZdecayTob/I") ;
  
  streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void bHadronAnalysis::Clear()
{
  m_bhadronEnergy = 0;
  m_bhadronTheta = 0;
  m_bhadronPhi = 0;
  m_bhadronfromZEnergy = 0;
  m_bhadronfromZTheta = 0;
  m_bhadronfromZPhi = 0;
  m_bhadronfromHEnergy = 0;
  m_bhadronfromHTheta = 0;
  m_bhadronfromHPhi = 0;
  m_bhadronBeta = 0;
  m_bhadronGamma = 0;
  m_bhadronBetaGamma = 0;
  m_bdecaylength = 0;
  m_cdecaylength = 0;
  m_longestbdecaylength = 0;
  m_2ndlongestbdecaylength = 0;
  m_3rdlongestbdecaylength = 0;
  m_4thlongestbdecaylength = 0;
  
}

void bHadronAnalysis::processRunHeader()
{

}

void bHadronAnalysis::processEvent( LCEvent *pLCEvent ) {
  this->Clear();
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////Processing event: " << m_nEvt << "////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;

  LCCollection *inputMCParticleCollection = NULL;

  try {
    streamlog_out(MESSAGE) << "  getting mc particle collection: " << m_mcParticleCollection << std::endl ;
    inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
    //m_pTTree->Fill();
    return;
  }
  MCParticle* e1 = (MCParticle*) inputMCParticleCollection->getElementAt(4);
  MCParticle* e2 = (MCParticle*) inputMCParticleCollection->getElementAt(5);
  vector<double> bdecaylengths;
  int nHdecayTob = 0;

  for (unsigned int i=0; i<inputMCParticleCollection->getNumberOfElements(); ++i) {
    MCParticle* p = (MCParticle*) inputMCParticleCollection->getElementAt(i);
    MCParticle* bhadron = NULL;
    MCParticle* chadron = NULL;
    bool Zdecay = false;
    bool Hdecay = false;
    
    if ( p->getPDG() == 25 && (p->getDaughters()).size() == 2 ) {
      if ( abs((p->getDaughters()[0])->getPDG()) == 5 && (p->getDaughters()[0])->getPDG() / (p->getDaughters()[1])->getPDG() == -1 ) {
	nHdecayTob++;
      }
    }
    if (floor(fabs(p->getPDG())/100) == 5 || floor(fabs(p->getPDG())/1000) == 5) {
      //cout << "bhadron found: " << p->getPDG() << endl;
      //p->getDaughters() and check if b hadrons are among them
      bool bweakdecay = true;
      MCParticleVec bdaughters = p->getDaughters();
      //cout << "ladida" << endl;
      //cout << "daughters: ";
      //for (auto d : bdaughters) cout << d->getPDG() << " " ;
      //cout << endl;
      for (auto d : bdaughters) {
	if (floor(fabs(d->getPDG())/100) == 5 || floor(fabs(d->getPDG())/1000) == 5) bweakdecay = false;
	if (floor(fabs(d->getPDG())/100) == 4 || floor(fabs(d->getPDG())/1000) == 4) {
	  //cout << "chadron found: " << d->getPDG() << endl; 
	  bool cweakdecay = true;
	  MCParticleVec cdaughters = d->getDaughters();
	  //cout << "grand daughters: "; 
	  //for (auto gd : cdaughters) cout << gd->getPDG() << " " ; 
	  //cout << endl; 
	  for (auto gd : cdaughters) {
	    if (floor(fabs(gd->getPDG())/100) == 4 || floor(fabs(gd->getPDG())/1000) == 4) cweakdecay = false;
	  }
	  if (cweakdecay) chadron = d;
	}
      }
      if (bweakdecay) bhadron = p;
      else continue;
      MCParticleVec bmothers = bhadron->getParents();
      while (!Zdecay && !Hdecay) {
	if (bmothers[0]->getParents().size()==0) break;
	if (bmothers[0]->getParents()[0] == e1 || bmothers[0]->getParents()[0] == e2) {
	  if (bmothers[0]->getPDG() == 25 ) Hdecay = true;
	  else Zdecay = true;
	}
	else bmothers = bmothers[0]->getParents();
      }
    }
    //if (bhadron) cout << "bhadron found dhs" << endl; 
    //if (chadron) cout << "chadron founddsnkja" << endl;
    
    if (!bhadron) continue;
    ROOT::Math::PxPyPzEVector FourMomentum(bhadron->getMomentum()[0], bhadron->getMomentum()[1], bhadron->getMomentum()[2], bhadron->getEnergy());
    m_bhadronEnergy = FourMomentum.E();
    m_bhadronTheta  = FourMomentum.Theta();
    m_bhadronPhi    = fabs(FourMomentum.Phi());
    m_bhadronBeta   = FourMomentum.Beta();
    m_bhadronGamma  = FourMomentum.Gamma();
    m_bhadronBetaGamma  = FourMomentum.Beta()*FourMomentum.Gamma();
    if (Zdecay) {
      m_bhadronfromZEnergy = FourMomentum.E();
      m_bhadronfromZTheta  = FourMomentum.Theta();
      m_bhadronfromZPhi    = fabs(FourMomentum.Phi());
    }
    if (Hdecay) {
      m_bhadronfromHEnergy = FourMomentum.E();
      m_bhadronfromHTheta  = FourMomentum.Theta();
      m_bhadronfromHPhi    = fabs(FourMomentum.Phi());
    }
    
    auto bini = bhadron->getVertex();
    auto bfin = bhadron->getEndpoint();
    double bdecaylength = sqrt((bfin[0]-bini[0])*(bfin[0]-bini[0])+(bfin[1]-bini[1])*(bfin[1]-bini[1])+(bfin[2]-bini[2])*(bfin[2]-bini[2]));
    bdecaylengths.push_back(bdecaylength);
    m_bdecaylength = bdecaylength;
    if (chadron) {
      ROOT::Math::PxPyPzEVector chadronFourMomentum(chadron->getMomentum()[0], chadron->getMomentum()[1], chadron->getMomentum()[2], chadron->getEnergy());
      auto cini = chadron->getVertex();
      auto cfin = chadron->getEndpoint();
      double cdecaylength = sqrt((cfin[0]-cini[0])*(cfin[0]-cini[0])+(cfin[1]-cini[1])*(cfin[1]-cini[1])+(cfin[2]-cini[2])*(cfin[2]-cini[2]));
      if (cdecaylength == 0 ) continue;
      m_cdecaylength = cdecaylength;
    }
  }
  sort(bdecaylengths.begin(), bdecaylengths.end(), greater<double>());
  if (nHdecayTob == 2 && bdecaylengths.size() >= 4 ) {
    m_longestbdecaylength = bdecaylengths[0];
    m_2ndlongestbdecaylength = bdecaylengths[1];
    m_3rdlongestbdecaylength = bdecaylengths[2];
    m_4thlongestbdecaylength = bdecaylengths[3];
  }
  
  m_pTTree->Fill();
}

void bHadronAnalysis::check()
{
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void bHadronAnalysis::end()
{
	m_pTFile->cd();
	m_pTTree->Write();
	m_pTFile->Close();
	delete m_pTFile;
}



