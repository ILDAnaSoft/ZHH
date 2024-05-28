// *****************************************************
// e+e- ------> ZHH ------> (l+ l-)(b b-bar)(b b-bar)
// Processor for leptons selection
//                        ----Junping
// *****************************************************
#include "ZHHll4JLeptonSelectionMVAProcessor.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <IMPL/LCCollectionVec.h>
#include <EVENT/ReconstructedParticle.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <EVENT/Cluster.h>
#include <UTIL/LCTypedVector.h>
#include <EVENT/Track.h>
#include <UTIL/LCRelationNavigator.h>
#include <EVENT/ParticleID.h>
#include <marlin/Exceptions.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#include "TROOT.h"
#include "TFile.h"
#include "TH1D.h"
#include "TNtupleD.h"
#include "TVector3.h"
#include "TMath.h"
#include "TLorentzVector.h"

#include "TMVA/Tools.h"
#include "TMVA/Reader.h"

#include "Utilities.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

using namespace TMVA;

ZHHll4JLeptonSelectionMVAProcessor aZHHll4JLeptonSelectionMVAProcessor ;


ZHHll4JLeptonSelectionMVAProcessor::ZHHll4JLeptonSelectionMVAProcessor() : Processor("ZHHll4JLeptonSelectionMVAProcessor") {
  
  // modify processor description
  _description = "ZHHll4JLeptonSelectionMVAProcessor does whatever it does ..." ;
  

  // register steering parameters: name, description, class-variable, default value

  registerInputCollection( LCIO::MCPARTICLE,
			   "InputMCParticlesCollection" , 
			   "Name of the MCParticle collection"  ,
			   _colMCP ,
			   std::string("MCParticlesSkimmed") ) ;

  registerInputCollection( LCIO::LCRELATION,
			   "InputMCTruthLinkCollection" , 
			   "Name of the MCTruthLink collection"  ,
			   _colMCTL ,
			   std::string("RecoMCTruthLink") ) ;

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "InputPandoraPFOsCollection" , 
			   "Name of the PandoraPFOs collection"  ,
			   _colPFOs ,
			   std::string("PandoraPFOs") ) ;

  registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			    "OutputNewPFOsCollection",
			    "Name of the new PFOs collection after some pre-cuts",
			    _colNewPFOs,
			    std::string("newpfos") );

  registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			    "OutputLeptonsCollection",
			    "Name of collection with the selected leptons",
			    _colLeptons,
			    std::string("leptons") );

  registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			    "OutputZPFOsCollection",
			    "Name of collection with the selected Z PFOs",
			    _colZPFOs,
			    std::string("zpfos") );

  registerProcessorParameter("IsLepTune",
			   "Is lepton tune?"  ,
			   _is_lep_tune ,
			   bool(false) ) ;

  registerProcessorParameter("IsTrainingDone",
			   "Is isolated lepton training done?"  ,
			   _is_training_done ,
			   bool(false) ) ;

  registerProcessorParameter("DirOfISOElectronWeights",
			   "Directory of Weights for the Isolated Electron MVA Classification"  ,
			   _isolated_electron_weights ,
			   std::string("isolated_electron_weights") ) ;

  registerProcessorParameter("DirOfISOMuonWeights",
			   "Directory of Weights for the Isolated Muon MVA Classification"  ,
			   _isolated_muon_weights ,
			   std::string("isolated_muon_weights") ) ;

  registerProcessorParameter("CutOnTheISOElectronMVA",
			   "Cut on the mva output of isolated electron selection"  ,
			   _mvacut_electron ,
			   float(0.5) ) ;

  registerProcessorParameter("CutOnTheISOMuonMVA",
			   "Cut on the mva output of isolated muon selection"  ,
			   _mvacut_muon ,
			   float(0.7) ) ;

  registerProcessorParameter("MethodForISOLeptonSelection",
			   "Which mehtod for isolated lepton selection: 1--MVA; 2-traditonal cone energy"  ,
			   _method_iso ,
			   int(1) ) ;
}

void ZHHll4JLeptonSelectionMVAProcessor::init() { 

  streamlog_out(DEBUG) << "   init called  " 
		       << std::endl ;
  
  
  // usually a good idea to
  printParameters() ;

  _nRun = 0 ;
  _nEvt = 0 ;

  hStat = 0;

  // This loads the library
   TMVA::Tools::Instance();
   
  //  TFile *outRootFile = new TFile("output.root","RECREATE");

  for (Int_t i=0;i<2;i++) {
    TMVA::Reader *reader = new TMVA::Reader( "Color:Silent" );    
    // add variables
    if (i == 0) {  // electron
      reader->AddVariable( "coneec",          &_coneec );
      reader->AddVariable( "coneen",          &_coneen );
      reader->AddVariable( "momentum",        &_momentum );
      reader->AddVariable( "coslarcon",       &_coslarcon );
      reader->AddVariable( "energyratio",     &_energyratio );
      reader->AddVariable( "ratioecal",       &_ratioecal );
      reader->AddVariable( "ratiototcal",     &_ratiototcal );
      reader->AddVariable( "nsigd0",          &_nsigd0 );
      reader->AddVariable( "nsigz0",          &_nsigz0 );
    }
    else {
      reader->AddVariable( "coneec",          &_coneec );
      reader->AddVariable( "coneen",          &_coneen );
      reader->AddVariable( "momentum",        &_momentum );
      reader->AddVariable( "coslarcon",       &_coslarcon );
      reader->AddVariable( "energyratio",     &_energyratio );
      reader->AddVariable( "yokeenergy",      &_yokeenergy );
      reader->AddVariable( "nsigd0",          &_nsigd0 );
      reader->AddVariable( "nsigz0",          &_nsigz0 );
      reader->AddVariable( "totalcalenergy",  &_totalcalenergy );
    }
    
    // book the reader (method, weights)
    TString dir    = _isolated_electron_weights;
    if (i == 1) dir = _isolated_muon_weights;
    TString prefix = "TMVAClassification";
    TString methodName = "MLP method";
    TString weightfile = dir + "/" + prefix + "_" + "MLP.weights.xml";
    reader->BookMVA( methodName, weightfile ); 
    _readers.push_back(reader);
  }
  
}

void ZHHll4JLeptonSelectionMVAProcessor::processRunHeader( LCRunHeader* run) { 

  _nRun++ ;
} 

void ZHHll4JLeptonSelectionMVAProcessor::processEvent( LCEvent * evt ) { 

    
  // this gets called for every event 
  // usually the working horse ...
  _nEvt++;

#if 1
  Double_t fEtrackCut = -1.;        // lower edge of each PFO energy 
#else
  Double_t fEtrackCut = 0.05;        // lower edge of each PFO energy 
#endif
  Double_t fElectronCut1 = 0.5;      // lower edge of totalCalEnergy/momentum
  Double_t fElectronCut2 = 1.3;      // upper edge of totalCalEnergy/momentum
  Double_t fElectronCut3 = 0.9;     // lower edge of ecalEnergy/totalCalEnergy
  Double_t fMuonCut1 = 0.3;      // upper edge of totalCalEnergy/momentum
  Double_t fMuonCut2 = 0.5;      // upper edge of ecalEnergy/totalCalEnergy
  Double_t fMuonCut3 = 1.2;      // lower edge of yoke energy
  Double_t fCosConeCut = 0.98;   // the angle of cone around the direction of pfo
  Double_t fCosLargeConeCut = 0.95; // angel of large cone around the pfo
  Double_t fCosFSRCut = 0.999;   // the angle of BS and FSR around the direction of charged lepton
  Double_t kMassZ = 91.187;     // Z mass
  Double_t fMassZCut = 40.;     // mass cut for lepton pair from Z
  Double_t fEpsilon = 1.E-10;
  // Fisher coefficients
  Double_t c0Electron = 12.2; 
  Double_t c1Electron = 0.87; 
  Double_t c0Muon = 12.6; 
  Double_t c1Muon = 4.62; 
  Double_t fPElectronCut = 5.;
  Double_t fPMuonCut = 5.;
  // cut table
  if (!hStat) hStat = new TH1D("hStat", "Cut Table", 20, 0, 20);
  Double_t selid = -0.5;
  hStat->Fill(++selid);
  gCutName[(Int_t)selid] << "No Cuts" << ends;

  TDirectory *last = gDirectory;
  gFile->cd("/");

  cerr << endl << "Hello, MVA Lepton Selection!" << endl;

  static TNtupleD *hEvt = 0;
  if (!hEvt) {
    cerr << "First Event!" << endl;
    stringstream tupstr;
    tupstr << "nMCP:nPFO:nelectron:nmuon:ltype:zmass"          << ":"
	   << "zmassr"                                         << ":"
	   << "elep1f:elep2f:elep1r:elep2r"                    << ":"
	   << "elep1mc:elep2mc:cos1mc:cos2mc:mcorig1:mcorig2"  << ":"
	   << "nhbb:zmassf:nsplit"
	   << ends;
    hEvt = new TNtupleD("hEvt","",tupstr.str().data());
  }

  // -- Get the MCTruth Linker --
  LCCollection *colMCTL = evt->getCollection(_colMCTL);
  LCRelationNavigator *navMCTL = new LCRelationNavigator(colMCTL);

  // -- Read out MC information --  
  LCCollection *colMC = evt->getCollection(_colMCP);
  if (!colMC) {
    std::cerr << "No MC Collection Found!" << std::endl;
    throw marlin::SkipEventException(this);
  }
  Int_t nMCP = colMC->getNumberOfElements();
  static TNtupleD *hMc = 0;
  if (!hMc) {
    stringstream tupstr_mc;
    tupstr_mc << "id:pdg:motherpdg:mass:nparents:energy:ndaughters:original:daughterpdg" << ":"
	      << "charge"
	      << ends;
    hMc = new TNtupleD("hMc","",tupstr_mc.str().data());
  }
  static TNtupleD *hGen = 0;
  if (!hGen) {
    stringstream tupstr_gen;
    tupstr_gen << "nhbb"
	       << ends;
    hGen = new TNtupleD("hGen","",tupstr_gen.str().data());
  }
  Double_t energyLep1MC=0.,energyLep2MC=0.;
  Double_t cosLep1MC=0.,cosLep2MC=0.;
  Int_t nHbb = 0;  // tag H---> b b
  Bool_t iMCDebug = kFALSE;
  //  Bool_t iMCDebug = kTRUE;
  if (iMCDebug) {
    cerr << "Serial" << "    " << "PDG" << " " << "Mother" << "    " << "Charge" << "      " << "Mass" << "     " << "Energy" << " " 
         << "NumberOfDaughters" << " " << "NumberOfParents" << "  " << "Original" << endl;
  }
  for (Int_t i=0;i<nMCP;i++) {
    MCParticle *mcPart = dynamic_cast<MCParticle*>(colMC->getElementAt(i));
    Int_t pdg = mcPart->getPDG();
    Int_t nparents = mcPart->getParents().size();
    Int_t motherpdg = 0;
    if (nparents > 0) {
      MCParticle *mother = mcPart->getParents()[0];
      motherpdg = mother->getPDG();
    }
    Double_t charge = mcPart->getCharge();
    Double_t mass = mcPart->getMass();
    Double_t energy = mcPart->getEnergy();
    TVector3 pv = TVector3(mcPart->getMomentum());
    Int_t ndaughters = mcPart->getDaughters().size();
    Int_t daughterpdg = 0;
    if (ndaughters > 0) {
      MCParticle *daughter = mcPart->getDaughters()[0];
      daughterpdg = daughter->getPDG();
    }
    TLorentzVector lortz = TLorentzVector(pv,energy);
    Int_t originalPDG = getOriginalPDG(mcPart);
    Double_t datamc[100];
    datamc[0] = i;
    datamc[1] = pdg;
    datamc[2] = motherpdg;
    datamc[3] = mass;
    datamc[4] = nparents;
    datamc[5] = energy;
    datamc[6] = ndaughters;
    datamc[7] = originalPDG;
    datamc[8] = daughterpdg;
    datamc[9] = charge;
    //    hMc->Fill(datamc);
    if (iMCDebug) {
      cerr << setw(6) << i << setw(7) << pdg << setw(7) << motherpdg << setw(10) << charge << setw(10) << mass 
	   << setw(11) << energy << setw(18) << ndaughters << setw(16) << nparents << setw(10) << originalPDG << endl;
    }
    // get the information of original leptons
    LCObjectVec vecMCTL = navMCTL->getRelatedFromObjects(mcPart);
    FloatVec vecWgtMCTL = navMCTL->getRelatedFromWeights(mcPart);
    Int_t nMCTLRec = vecMCTL.size();
    Double_t chargeRec = 99;
    Double_t wgtRec = 0;
    if (pdg == 25 && abs(daughterpdg) == 5) {
      nHbb +=1 ;
    }
    if (i == 6) {
      energyLep1MC = energy;
      cosLep1MC    = pv.CosTheta();
    }
    if (i == 7) {
      energyLep2MC = energy;
      cosLep2MC    = pv.CosTheta();
    }
  }
  Double_t data_gen[10];
  data_gen[0] = nHbb;
  hGen->Fill(data_gen);

  // -- Read out PFO information --
  LCCollection *colPFO = evt->getCollection(_colPFOs);
  if (!colPFO) {
    std::cerr << "No PFO Collection Found!" << std::endl;
    throw marlin::SkipEventException(this);
  }
  hStat->Fill(++selid);
  gCutName[(Int_t)selid] << "MCParticle and PandoraPFOs Collections found!" << ends;
  Int_t nPFOs = colPFO->getNumberOfElements();
  //  cerr << "Number of PFOs: " << nPFOs << endl;
  LCCollectionVec *pNewPFOsCollection = new LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  LCCollectionVec *pLeptonsCollection = new LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  pNewPFOsCollection->setSubset(true);
  pLeptonsCollection->setSubset(true);
  std::vector<lcio::ReconstructedParticle*> newPFOs;
  std::vector<lcio::ReconstructedParticle*> leptons;
  std::vector<lcio::ReconstructedParticle*> electrons;
  std::vector<lcio::ReconstructedParticle*> muons;

  static TNtupleD *hPfo = 0;
  if (!hPfo) {
    stringstream tupstr_pfo;
    tupstr_pfo << "ntracks:charge:mcpdg:motherpdg:deltae:mmotherpdg:ndaughters" << ":"
	       << "mcoriginal:energy:type:pid"                                  << ":"
	       << "totalcalenergy:momentum:ecalenergy:hcalenergy:coneenergy"    << ":"
	       << "nmctl:mcwgt:ievt:irun"                                       << ":"
	       << "nhits:ncones:nconechg:nconeneu:coneec:coneen:energylink"     << ":"
	       << "costheta:yokeenergy:energycor:momentumcor"                   << ":"
	       << "d0:z0:r0:deltad0:deltaz0:nsigd0:nsigz0:nsigr0:iov"           << ":"
	       << "coslarcon:energyratio:nphoton:ratioecal:ratiototcal"         << ":"
	       << "mvaelectron:mvamuon"
	       << ends;
    hPfo = new TNtupleD("hPfo","",tupstr_pfo.str().data());
  }

  // loop all the PFOs
  const Int_t nLepMax = 20;
  ReconstructedParticle *leps[nLepMax];
  Double_t mva_outs[nLepMax];
  Int_t nLeps = 0;
  float _mva_lep_minus = -1.;
  float _mva_lep_plus = -1.;
  for (Int_t i=0;i<nPFOs;i++) {
    ReconstructedParticle *recPart = dynamic_cast<ReconstructedParticle*>(colPFO->getElementAt(i));
    LCObjectVec vecMCTL = navMCTL->getRelatedToObjects(recPart);
    FloatVec vecWgtMCTL = navMCTL->getRelatedToWeights(recPart);
    Int_t mcpdg,motherpdg,mmotherpdg;
    Double_t mcwgt=0.;
    mcpdg = 0;
    motherpdg = -99999;
    mmotherpdg = -99999;
    Double_t deltaE = -99999.;
    Double_t energyLink = -99999.;
    Int_t mcoriginal = 0;
    Int_t mcndaughters = 0;
    Int_t nMCTL = vecMCTL.size();
    Int_t iOverlay = 0;
    if (vecMCTL.size() > 0) {
      MCParticle *mcPart = dynamic_cast<MCParticle *>(vecMCTL[0]);
      if (mcPart->isOverlay()) iOverlay = 1;
      mcpdg = mcPart->getPDG();
      mcwgt = vecWgtMCTL[0];
      deltaE = mcPart->getEnergy()-recPart->getEnergy();
      energyLink = mcPart->getEnergy();
      mcoriginal = getOriginalPDG(mcPart);
      motherpdg = 0;
      mcndaughters = mcPart->getDaughters().size();
      if (mcPart->getParents().size() != 0) {
	MCParticle *motherPart = mcPart->getParents()[0];
	motherpdg = motherPart->getPDG();
	mmotherpdg = 0;
	if (motherPart->getParents().size() != 0) {
	  MCParticle *mmotherPart = motherPart->getParents()[0];
	  mmotherpdg = mmotherPart->getPDG();
	}
      }
    }
    Double_t energy = recPart->getEnergy();
    Double_t charge = recPart->getCharge();
    Int_t itype = recPart->getType();
    Int_t pid = 0;
    TrackVec tckvec = recPart->getTracks();
    Int_t ntracks = tckvec.size();
    Double_t d0=0.,z0=0.,deltad0=0.,deltaz0=0.,nsigd0=0.,nsigz0=0.;
    if (ntracks > 0) {
      d0 = tckvec[0]->getD0();
      z0 = tckvec[0]->getZ0();
      deltad0 = TMath::Sqrt(tckvec[0]->getCovMatrix()[0]);
      deltaz0 = TMath::Sqrt(tckvec[0]->getCovMatrix()[9]);
      nsigd0 = d0/deltad0;
      nsigz0 = z0/deltaz0;
    }
    Double_t r0 = TMath::Sqrt(d0*d0+z0*z0);
    Double_t nsigr0 = TMath::Sqrt(nsigd0*nsigd0+nsigz0*nsigz0);
    Double_t data[100];
    data[0] = ntracks;
    data[1] = charge;
    data[2] = mcpdg;
    data[3] = motherpdg;
    data[4] = deltaE;
    data[5] = mmotherpdg;
    data[6] = mcndaughters;
    data[7] = mcoriginal;
    data[8] = energy;
    data[9] = itype;
    data[10]= pid;
    if (energy > fEtrackCut) {
      newPFOs.push_back(recPart);
      Double_t ecalEnergy = 0;
      Double_t hcalEnergy = 0;
      Double_t yokeEnergy = 0;
      Double_t totalCalEnergy = 0;
      Int_t nHits = 0;
      std::vector<lcio::Cluster*> clusters = recPart->getClusters();
      for (std::vector<lcio::Cluster*>::const_iterator iCluster=clusters.begin();iCluster!=clusters.end();++iCluster) {
	ecalEnergy += (*iCluster)->getSubdetectorEnergies()[0];
	hcalEnergy += (*iCluster)->getSubdetectorEnergies()[1];
	yokeEnergy += (*iCluster)->getSubdetectorEnergies()[2];
	ecalEnergy += (*iCluster)->getSubdetectorEnergies()[3];
	hcalEnergy += (*iCluster)->getSubdetectorEnergies()[4];
	CalorimeterHitVec calHits = (*iCluster)->getCalorimeterHits();
	//	nHits += (*iCluster)->getCalorimeterHits().size();
	nHits = calHits.size();
      }
      totalCalEnergy = ecalEnergy + hcalEnergy;
      TVector3 momentum = TVector3(recPart->getMomentum());
      Double_t momentumMagnitude = momentum.Mag();
      Double_t cosTheta = momentum.CosTheta();
      //get cone information
      //      std::vector<lcio::ReconstructedParticle*> conePFOs;
      //      Double_t coneEnergy = getConeEnergy(recPart,colPFO,fCosConeCut,conePFOs);
      Bool_t woFSR = kTRUE;
      Double_t coneEnergy0[3] = {0.,0.,0.};
      Double_t pFSR[4] = {0.,0.,0.,0.};
      Double_t pLargeCone[4]  = {0.,0.,0.,0.};
      Int_t nConePhoton = 0;
      //      getConeEnergy(recPart,colPFO,fCosConeCut,woFSR,coneEnergy0,pFSR);
      getConeEnergy(recPart,colPFO,fCosConeCut,woFSR,coneEnergy0,pFSR,fCosLargeConeCut,pLargeCone,nConePhoton);
      Double_t coneEnergy = coneEnergy0[0];
      Double_t coneEN     = coneEnergy0[1];
      Double_t coneEC     = coneEnergy0[2];
      TLorentzVector lortzFSR = TLorentzVector(pFSR[0],pFSR[1],pFSR[2],pFSR[3]);
      TLorentzVector lortzLargeCone = TLorentzVector(pLargeCone[0],pLargeCone[1],pLargeCone[2],pLargeCone[3]);
      TVector3 momentumLargeCone = lortzLargeCone.Vect();
      Double_t cosThetaWithLargeCone = 1.;
      if (momentumLargeCone.Mag() > 0.0000001) {
	cosThetaWithLargeCone = momentum.Dot(momentumLargeCone)/momentumMagnitude/momentumLargeCone.Mag();
      }
      Double_t energyRatioWithLargeCone = energy/(energy+lortzLargeCone.E());
      Double_t energyCorr = energy + lortzFSR.E();
      TVector3 momentumCorr = momentum + TVector3(lortzFSR.Px(),lortzFSR.Py(),lortzFSR.Pz());
      Double_t momentumMagCorr = momentumCorr.Mag();
      Double_t ratioECal = 0., ratioTotalCal = 0.;
      if (ecalEnergy > 0.) ratioECal = ecalEnergy/totalCalEnergy;
      ratioTotalCal = totalCalEnergy/momentumMagnitude;
      //      Int_t nConePFOs    = conePFOs.size();
      Int_t nConePFOs    = 0;
      Int_t nConeCharged = 0;
      Int_t nConeNeutral = 0;
      // evaluate the neural-net output of isolated-lepton classfication
      Double_t mva_electron = -1.,mva_muon = -1.;
      _coneec      = coneEC;
      _coneen      = coneEN;
      _momentum    = momentumMagnitude;
      _coslarcon   = cosThetaWithLargeCone;
      _energyratio = energyRatioWithLargeCone;
      _ratioecal   = ratioECal;
      _ratiototcal = ratioTotalCal;
      _nsigd0      = nsigd0;
      _nsigz0      = nsigz0;
      _yokeenergy  = yokeEnergy;
      _totalcalenergy = totalCalEnergy;
      if (charge != 0 && 
	  totalCalEnergy/momentumMagnitude > fElectronCut1 && totalCalEnergy/momentumMagnitude < fElectronCut2 &&
	  ecalEnergy/(totalCalEnergy + fEpsilon) > fElectronCut3 && 
	  (momentumMagnitude > fPElectronCut)) {
	if (nsigd0 < 50 && nsigz0 < 5) {   // contraint to primary vertex
	  if (_is_training_done) {
	    mva_electron = _readers[0]->EvaluateMVA( "MLP method"           );
	    leps[nLeps] = recPart;
	    mva_outs[nLeps] = mva_electron;
	    nLeps++;
	  }
	}
      }
      if (charge != 0 && 
	  totalCalEnergy/momentumMagnitude < fMuonCut1 && 
	  yokeEnergy > fMuonCut3 && 
	  (momentumMagnitude > fPMuonCut)) {
	if (nsigd0 < 5 && nsigz0 < 5) {
	  if (_is_training_done) {
	    mva_muon = _readers[1]->EvaluateMVA( "MLP method"           );
	    leps[nLeps] = recPart;
	    mva_outs[nLeps] = mva_muon;
	    nLeps++;
	  }
	}
      }
      // save the pfo information
      data[11] = totalCalEnergy;
      data[12] = momentumMagnitude;
      data[13] = ecalEnergy;
      data[14] = hcalEnergy;
      data[15] = coneEnergy;
      data[16] = nMCTL;
      data[17] = mcwgt;
      data[18] = _nEvt;
      data[19] = _nRun;
      data[20] = nHits;
      data[21] = nConePFOs;
      data[22] = nConeCharged;
      data[23] = nConeNeutral;
      data[24] = coneEC;
      data[25] = coneEN;
      data[26] = energyLink;
      data[27] = cosTheta;
      data[28] = yokeEnergy;
      data[29] = energyCorr;
      data[30] = momentumMagCorr;
      data[31] = d0;
      data[32] = z0;
      data[33] = r0;
      data[34] = deltad0;
      data[35] = deltaz0;
      data[36] = nsigd0;
      data[37] = nsigz0;
      data[38] = nsigr0;
      data[39] = iOverlay;
      data[40] = cosThetaWithLargeCone;
      data[41] = energyRatioWithLargeCone;
      data[42] = nConePhoton;
      data[43] = ratioECal;
      data[44] = ratioTotalCal;
      data[45] = mva_electron;
      data[46] = mva_muon;
      if (_is_lep_tune) {
	hPfo->Fill(data);
      }
      // select the leptons
      if (_method_iso == 1) {  // MVA
	if (mva_electron > _mvacut_electron) {
	  leptons.push_back(recPart);
	  electrons.push_back(recPart);
	}
	if (mva_muon > _mvacut_muon) {
	  leptons.push_back(recPart);
	  muons.push_back(recPart);
	}
      }
      else if (_method_iso == 2) {  // Econe.vs.P
	if (charge != 0 && 
	    totalCalEnergy/momentumMagnitude > fElectronCut1 && totalCalEnergy/momentumMagnitude < fElectronCut2 &&
	    ecalEnergy/(totalCalEnergy + fEpsilon) > fElectronCut3 && 
	    (momentumMagnitude > c0Electron + c1Electron*coneEC)) {
	  if (nsigd0 > 50 || nsigz0 > 5) continue;   // contraint to primary vertex
	  leptons.push_back(recPart);
	  electrons.push_back(recPart);
	}
	if (charge != 0 && 
	    totalCalEnergy/momentumMagnitude < fMuonCut1 && 
	    yokeEnergy > fMuonCut3 && 
	    (momentumMagnitude > c0Muon + c1Muon*coneEC)) {
	  if (nsigd0 > 5 || nsigz0 > 5) continue;  // contraint to primary vertex
	  leptons.push_back(recPart);
	  muons.push_back(recPart);
	}
      }
      else {
	std::cerr << "Not proper method specified for isolated lepton selection!" << std::endl;
	return;
      }
    }
  }
  Int_t nelectrons = electrons.size();
  Int_t nmuons = muons.size();
  static TNtupleD *hLep = 0;
  if (!hLep) {
    stringstream tupstr_lep;
    tupstr_lep << "nelectron:nmuon:nhbb"
	       << ends;
    hLep = new TNtupleD("hLep","",tupstr_lep.str().data());
  }
  Double_t data_lep[20];
  data_lep[1] = nelectrons;
  data_lep[2] = nmuons;
  data_lep[3] = nHbb;
  hLep->Fill(data_lep);

  if (_is_lep_tune) return;
  //  cerr << "nelectrons: " << nelectrons << "  nmuons: " << nmuons << endl;
  if (nelectrons < 2 && nmuons < 2) throw marlin::SkipEventException(this);
  hStat->Fill(++selid);
  gCutName[(Int_t)selid] << "nElectrons > 2 || nMuons > 2" << ends;

  // find the lepton pair nearest to the Z mass
  std::vector<lcio::ReconstructedParticle*> electronZ;
  std::vector<lcio::ReconstructedParticle*> muonZ;
  std::vector<lcio::ReconstructedParticle*> leptonZ;
  std::vector<lcio::ReconstructedParticle*> photonSplit;
  Double_t deltaMassZ = fMassZCut;
  Double_t massZ=0.;
  if (electrons.size() > 1) {
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iElectron=electrons.begin();iElectron<electrons.end()-1;iElectron++) {
      for (std::vector<lcio::ReconstructedParticle*>::const_iterator jElectron=iElectron+1;jElectron<electrons.end();jElectron++) {
	if ((*iElectron)->getCharge() != (*jElectron)->getCharge()) {
	  Double_t mass = getInvariantMass((*iElectron),(*jElectron));
	  if (TMath::Abs(mass-kMassZ) < deltaMassZ+20) {
	    deltaMassZ = TMath::Abs(mass-kMassZ);
	    massZ = mass;
	    electronZ.clear();
	    electronZ.push_back(*iElectron);
	    electronZ.push_back(*jElectron);
	  }
	}
      }
    }
  }
  if (muons.size() > 1) {
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iMuon=muons.begin();iMuon<muons.end()-1;iMuon++) {
      for (std::vector<lcio::ReconstructedParticle*>::const_iterator jMuon=iMuon+1;jMuon<muons.end();jMuon++) {
	if ((*iMuon)->getCharge() != (*jMuon)->getCharge()) {
	  Double_t mass = getInvariantMass((*iMuon),(*jMuon));
	  if (TMath::Abs(mass-kMassZ) < deltaMassZ) {
	    deltaMassZ = TMath::Abs(mass-kMassZ);
	    massZ = mass;
	    muonZ.clear();
	    muonZ.push_back(*iMuon);
	    muonZ.push_back(*jMuon);
	  }
	}
      }
    }
  }
  Int_t iLeptonType = 0;
  Double_t energyLep1Rec=0.,energyLep2Rec=0.;
  Double_t energyLep1FSR=0.,energyLep2FSR=0.;
  Int_t mcOriginalLep1=0,mcOriginalLep2=0;
  if (muonZ.size() == 0 && electronZ.size() == 0) {
    cerr << "no lepton pair candidate found!" << endl;
    throw marlin::SkipEventException(this);
  }
  else if (muonZ.size() > 0) {
    iLeptonType = 2;
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iMuon=muonZ.begin();iMuon<muonZ.end();iMuon++) {
      leptonZ.push_back(*iMuon);
      Int_t mcoriginal = 0;
      LCObjectVec vecMCTL = navMCTL->getRelatedToObjects((*iMuon));
      Int_t nMCTL = vecMCTL.size();
      if (vecMCTL.size() > 0) {
	MCParticle *mcPart = dynamic_cast<MCParticle *>(vecMCTL[0]);
	mcoriginal = getOriginalPDG(mcPart);
      }
      if ((*iMuon)->getCharge() < 0) {
	energyLep1FSR += (*iMuon)->getEnergy();
	energyLep1Rec += (*iMuon)->getEnergy();
	mcOriginalLep1 = mcoriginal;
	for (Int_t i=0;i<nLeps;i++) {
	  if (leps[i] == (*iMuon)) _mva_lep_minus = mva_outs[i];
	}
      }
      else {
	energyLep2FSR += (*iMuon)->getEnergy();
	energyLep2Rec += (*iMuon)->getEnergy();
	mcOriginalLep2 = mcoriginal;
	for (Int_t i=0;i<nLeps;i++) {
	  if (leps[i] == (*iMuon)) _mva_lep_plus = mva_outs[i];
	}
      }
      for (Int_t i=0;i<nPFOs;i++) { // recover the FSR for muon
	ReconstructedParticle *recPart = dynamic_cast<ReconstructedParticle*>(colPFO->getElementAt(i));
	if (recPart == (*iMuon)) continue;
	Bool_t isLep = kFALSE;
	for (std::vector<lcio::ReconstructedParticle*>::const_iterator iLep=leptonZ.begin();iLep<leptonZ.end();++iLep) {
	  if (recPart == (*iLep)) isLep = kTRUE;
	}
	if (isLep) continue;
	Bool_t isFSR = getFSRTag((*iMuon),recPart);
	if (! isFSR) continue;
	leptonZ.push_back(recPart);
	if ((*iMuon)->getCharge() < 0) {
	  energyLep1FSR += recPart->getEnergy();
	}
	else {
	  energyLep2FSR += recPart->getEnergy();
	}
      }
      pLeptonsCollection->addElement(*iMuon);
    }
  }
  else {
    iLeptonType = 1;
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iElectron=electronZ.begin();iElectron<electronZ.end();iElectron++) {
      leptonZ.push_back(*iElectron);
      Int_t mcoriginal = 0;
      LCObjectVec vecMCTL = navMCTL->getRelatedToObjects((*iElectron));
      Int_t nMCTL = vecMCTL.size();
      if (vecMCTL.size() > 0) {
	MCParticle *mcPart = dynamic_cast<MCParticle *>(vecMCTL[0]);
	mcoriginal = getOriginalPDG(mcPart);
      }
      if ((*iElectron)->getCharge() < 0) {
	energyLep1FSR += (*iElectron)->getEnergy();
	energyLep1Rec += (*iElectron)->getEnergy();
	mcOriginalLep1 = mcoriginal;
	for (Int_t i=0;i<nLeps;i++) {
	  if (leps[i] == (*iElectron)) _mva_lep_minus = mva_outs[i];
	}
      }
      else {
	energyLep2FSR += (*iElectron)->getEnergy();
	energyLep2Rec += (*iElectron)->getEnergy();
	mcOriginalLep2 = mcoriginal;
	for (Int_t i=0;i<nLeps;i++) {
	  if (leps[i] == (*iElectron)) _mva_lep_plus = mva_outs[i];
	}
      }
      for (Int_t i=0;i<nPFOs;i++) {  // recover the FSR for electron
	ReconstructedParticle *recPart = dynamic_cast<ReconstructedParticle*>(colPFO->getElementAt(i));
	if (recPart == (*iElectron)) continue;
	Bool_t isLep = kFALSE;
	for (std::vector<lcio::ReconstructedParticle*>::const_iterator iLep=leptonZ.begin();iLep<leptonZ.end();++iLep) {
	  if (recPart == (*iLep)) isLep = kTRUE;
	}
	if (isLep) continue;
	Bool_t isFSR = getFSRTag((*iElectron),recPart);
	if (! isFSR) continue;
	leptonZ.push_back(recPart);
	Bool_t isSplit = getSplitTag((*iElectron),recPart);
	if (isSplit) photonSplit.push_back(recPart);
	if ((*iElectron)->getCharge() < 0) {
	  energyLep1FSR += recPart->getEnergy();
	}
	else {
	  energyLep2FSR += recPart->getEnergy();
	}
      }
      pLeptonsCollection->addElement(*iElectron);
    }
  }
  hStat->Fill(++selid);
  gCutName[(Int_t)selid] << "at least one lepton pair satisfies |Mass - MassZ| < 40" << ends;

  // create the ZPFOs collection
  LCCollectionVec * pZCollection = new LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  ReconstructedParticleImpl * recoZ = new ReconstructedParticleImpl();
  TLorentzVector lortzZ_FSR = TLorentzVector(0.,0.,0.,0.);
  TLorentzVector lortzZ_FSR_Split = TLorentzVector(0.,0.,0.,0.);
  for (std::vector<lcio::ReconstructedParticle*>::const_iterator iLep=leptonZ.begin();iLep<leptonZ.end();++iLep) {
    lortzZ_FSR_Split += TLorentzVector((*iLep)->getMomentum(),(*iLep)->getEnergy());
    // do not add the energy and momentum if it is splitted cluster
    Bool_t isSplit = kFALSE;
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iSp=photonSplit.begin();iSp<photonSplit.end();++iSp) {
      if ((*iLep) == (*iSp)) isSplit = kTRUE;
    }
    if (isSplit) continue;
    recoZ->addParticle(*iLep);
    lortzZ_FSR += TLorentzVector((*iLep)->getMomentum(),(*iLep)->getEnergy());
  }
  Double_t energyZ_FSR = lortzZ_FSR.E();
  Double_t massZ_FSR = lortzZ_FSR.M();
  Double_t momentumZ_FSR[3] = {lortzZ_FSR.Px(),lortzZ_FSR.Py(),lortzZ_FSR.Pz()};
  recoZ->setMomentum(momentumZ_FSR);
  recoZ->setEnergy(energyZ_FSR);
  recoZ->setMass(massZ_FSR);
  recoZ->setCharge(0.);
  recoZ->setType(94);
  pZCollection->addElement(recoZ);
  pZCollection->parameters().setValue( "MVALepMinus", _mva_lep_minus );
  pZCollection->parameters().setValue( "MVALepPlus", _mva_lep_plus );

  Double_t massZ_Split = lortzZ_FSR_Split.M();
  Int_t nSplit = photonSplit.size();

  //save the quantities to the hEvt
  Double_t vdata[100];
  vdata[ 0] = nMCP;
  vdata[ 1] = nPFOs;
  vdata[ 2] = nelectrons;
  vdata[ 3] = nmuons;
  vdata[ 4] = iLeptonType;
  vdata[ 5] = massZ;
  vdata[ 6] = massZ_FSR;
  vdata[ 7] = energyLep1FSR;
  vdata[ 8] = energyLep2FSR;
  vdata[ 9] = energyLep1Rec;
  vdata[10] = energyLep2Rec;
  vdata[11] = energyLep1MC;
  vdata[12] = energyLep2MC;
  vdata[13] = cosLep1MC;
  vdata[14] = cosLep2MC;
  vdata[15] = mcOriginalLep1;
  vdata[16] = mcOriginalLep2;
  vdata[17] = nHbb;
  vdata[18] = massZ_Split;
  vdata[19] = nSplit;
  hEvt->Fill(vdata);

  if (TMath::Abs(massZ_FSR-kMassZ) > fMassZCut) throw marlin::SkipEventException(this);
  hStat->Fill(++selid);
  gCutName[(Int_t)selid] << "|M(ll) - M(Z)| < 40 GeV" << ends;

  // save the other PFOs to a new collection
  for (std::vector<lcio::ReconstructedParticle*>::const_iterator iObj=newPFOs.begin();iObj<newPFOs.end();++iObj) {
    Bool_t isLep=kFALSE;
    for (std::vector<lcio::ReconstructedParticle*>::const_iterator iLep=leptonZ.begin();iLep<leptonZ.end();++iLep) {
      if ((*iObj) == (*iLep)) isLep = kTRUE;
    }
    if (!isLep) pNewPFOsCollection->addElement(*iObj);
  }
  evt->addCollection(pNewPFOsCollection,_colNewPFOs.c_str());
  evt->addCollection(pLeptonsCollection,_colLeptons.c_str());
  evt->addCollection(pZCollection,_colZPFOs.c_str());

  //-- note: this will not be printed if compiled w/o MARLINDEBUG=1 !

  streamlog_out(DEBUG) << "   processing event: " << evt->getEventNumber() 
  		       << "   in run:  " << evt->getRunNumber() 
  		       << std::endl ;

  //  _nEvt ++ ;

  last->cd();
}



void ZHHll4JLeptonSelectionMVAProcessor::check( LCEvent * evt ) { 
  // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void ZHHll4JLeptonSelectionMVAProcessor::end(){ 

  //  _readers.clear();
  for (std::vector<TMVA::Reader*>::const_iterator ireader=_readers.begin();ireader!=_readers.end();++ireader) {
    delete *ireader;
  }
  //  delete hStat;
  //delete gCutName;

  cerr << "ZHHll4JLeptonSelectionMVAProcessor::end()  " << name() 
       << " processed " << _nEvt << " events in " << _nRun << " runs "
       << endl ;
  //  cerr << endl;
  cerr << "  =============" << endl;
  cerr << "   Cut Summary " << endl;
  cerr << "  =============" << endl;
  cerr << "   ll+4 Jet    " << endl;
  cerr << "  =============" << endl;
  cerr << endl
       << "  -----------------------------------------------------------" << endl
       << "   ID   No.Events    Cut Description                         " << endl
       << "  -----------------------------------------------------------" << endl;
  for (int id=0; id<20 && gCutName[id].str().data()[0]; id++) {
    cerr << "  " << setw( 3) << id
         << "  " << setw(10) << static_cast<int>(hStat->GetBinContent(id+1))
         << "  : " << gCutName[id].str().data() << endl;
  }
  cerr << "  -----------------------------------------------------------" << endl;
  
}
