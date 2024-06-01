// *****************************************************
// e+e- ------> ZHH ------> (l+ l-)(b b-bar)(b b-bar)
// Processor for final selection
//                        ----Junping
// *****************************************************
#include "ZHHll4JAnalysisProcessor.h"
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
#include "UTIL/PIDHandler.h"

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#include "TROOT.h"
#include "TFile.h"
#include "TH1D.h"
#include "TNtupleD.h"
#include "TVector3.h"
#include "TMath.h"
#include "TLorentzVector.h"

#include "Utilities.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;


ZHHll4JAnalysisProcessor aZHHll4JAnalysisProcessor ;


ZHHll4JAnalysisProcessor::ZHHll4JAnalysisProcessor() : Processor("ZHHll4JAnalysisProcessor") {
  
  // modify processor description
  _description = "ZHHll4JAnalysisProcessor does whatever it does ..." ;
  

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

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			   "InputNewPFOsCollection",
			   "Name of the new PFOs collection after some pre-cuts",
			   _colNewPFOs,
			   std::string("NewPFOs_Uncluster") );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			   "InputLeptonsCollection",
			   "Name of collection with the selected leptons",
			   _colLeptons,
			   std::string("Leptons") );

  registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			    "OutputZPFOsCollection",
			    "Name of collection with the selected Z PFOs",
			    _colZPFOs,
			    std::string("ZPFOs") );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			   "InputNewJetsCollection",
			   "Name of the new jets collection",
			   _colNewJets,
			   std::string("RefinedJets_4JetN") );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE, 
			   "InputNewJets2Collection",
			   "Name of the new jets2 collection",
			   _colNewJets2,
			   std::string("Durham_2JetsN") );

}

void ZHHll4JAnalysisProcessor::init() { 

  streamlog_out(DEBUG) << "   init called  " 
		       << std::endl ;
  
  
  // usually a good idea to
  printParameters() ;

  _nRun = 0 ;
  _nEvt = 0 ;

  hStatAnl = 0;
  //  TFile *outRootFile = new TFile("output.root","RECREATE");
  
}

void ZHHll4JAnalysisProcessor::processRunHeader( LCRunHeader* run) { 

  _nRun++ ;
} 

void ZHHll4JAnalysisProcessor::processEvent( LCEvent * evt ) { 

    
  // this gets called for every event 
  // usually the working horse ...
  _nEvt++;

  // some constants
  static const Double_t fEnergyCut = 0.05; // minimum energy of each pfo
  static const Double_t kEcm = 500.;    // center-of-mass energy
  static const Double_t kMassW   =  80.4; // W mass by s.y
  static const Double_t kSigmaMw =  4.8; // W mass resolution
  static const Double_t kMassZ   = 91.187; // Z mass
  static const Double_t kSigmaMz =   6.0; // Z mass resolution
  static const Double_t kMassH   = 125.0; // H mass
  static const Double_t kSigmaMh =   7.2; // H mass resolution
  static const Double_t kMassT   = 174.0; // Top mass
  static const Double_t kSigmaMt =  20.0; // Top mass resolution
  static const Double_t kSigmaE  =   7.0; // Top mass resolution
  static const Double_t fMassZCut = 40.;     // mass cut for lepton pair from Z
  static const Double_t fMassHCut = 80.;     // mass cut for lepton pair from Z
  static const Double_t fCosConeCut = 0.98;   // the angle of cone around the direction of pfo

  if (!hStatAnl) hStatAnl = new TH1D("hStatAnl", "Cut Table", 20, 0, 20);
  Double_t selid = -0.5;
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "No Cuts" << ends;

  TDirectory *last = gDirectory;
  gFile->cd("/");

  cerr << endl << "Hello, Analysis!" << " No: " << _nEvt << endl;

  static TNtupleD *hAnl = 0;
  if (isFirstEvent()) {
    cerr << "First Event!" << endl;
    stringstream tupstr;
    tupstr << "evis:mz:mh1:mh2:mhh:pz:ph1:ph2:cosz:cosh1:cosh2:yminus:yplus"    << ":"
	   << "nhbb:njets:chi2:plmax:prob11:prob12:prob21:prob22"                << ":"
	   << "px11:py11:pz11:e11:px12:py12:pz12:e12"                           << ":"
	   << "px21:py21:pz21:e21:px22:py22:pz22:e22"                           << ":"
	   << "npfos:plmin:econe1:econe2:econec1:econec2"                       << ":"
	   << "npfos11:npfos12:npfos21:npfos22"                                 << ":"
	   << "pthrust:majthrust:minthrust"                                     << ":"
	   << "cos1z:cos2z:cos1h1:cos2h1:cos1h2:cos2h2"                         << ":"
	   << "p1st:p2nd:p3rd:cos1st:cos2nd:cos3rd"                             << ":"
	   << "cos1st1:cos1st2:cos2nd1:cos2nd2:cos3rd1:cos3rd2"                 << ":"
	   << "elp1:pxlp1:pylp1:pzlp1:elp2:pxlp2:pylp2:pzlp2"                   << ":"
	   << "cosaxis:cosbmax"                                                 << ":"
	   << "bcbmax1:bcbmax2:bcbmax3:bcbmax4"                                 << ":"
	   << "cbmax1:cbmax2:cbmax3:cbmax4"                                     << ":"
	   << "yminus2:yplus2:p1jets2:p2jets2:cos1jets2:cos2jets2"              << ":"
	   << "cos12jets2:cosj1zjets2:cosj2zjets2:cosjzmax2"                    << ":"
	   << "massb34:massb12:bmax1:bmax2:bmax3:bmax4"                         << ":"
	   << "npfosb1:npfosb2:npfosb3:npfosb4:npfosmin:ltype1:ltype2"          <<":"
	   << "mvalepminus:mvalepplus"
	   << ends;
    hAnl = new TNtupleD("hAnl","",tupstr.str().data());
  }

  // ------------------------------------------------
  // -- read out the MCParticles information
  // ------------------------------------------------
  LCCollection *colMC = evt->getCollection(_colMCP);
  // get the mode in which H decays into b b-bar
  Int_t nHbb=0;
  Int_t nMCP = colMC->getNumberOfElements();
  for (Int_t i=0;i<nMCP;i++) {
    MCParticle *mcPart = dynamic_cast<MCParticle*>(colMC->getElementAt(i));
    Int_t pdg = mcPart->getPDG();
    Int_t ndaughters = mcPart->getDaughters().size();
    Int_t daughterpdg = 0;
    if (ndaughters > 0) {
      MCParticle *daughter = mcPart->getDaughters()[0];
      daughterpdg = daughter->getPDG();
    }
    if (pdg == 25 && abs(daughterpdg) == 5) nHbb++;
  }

  // ------------------------------------------------
  // -- read out the PandoraPFOs information
  // ------------------------------------------------
  LCCollection *colPFO = evt->getCollection(_colPFOs);

  // ------------------------------------------------
  // -- read out the Thrust information
  // ------------------------------------------------
  LCCollection *colSelRecPart = evt->getCollection("SelectedReconstructedParticle");
  Double_t principleThrust = colSelRecPart->parameters().getFloatVal("principleThrustValue");
  Double_t majorThrust = colSelRecPart->parameters().getFloatVal("majorThrustValue");
  Double_t minorThrust = colSelRecPart->parameters().getFloatVal("minorThrustValue");
  FloatVec tAxis;
  FloatVec thrustAxis = colSelRecPart->parameters().getFloatVals("principleThrustAxis",tAxis);
  TVector3 principleAxis = TVector3(thrustAxis[0],thrustAxis[1],thrustAxis[2]);
  Double_t cosThrustAxis = principleAxis.CosTheta();

  // ------------------------------------------------
  // -- read out the NewPFOs information
  // ------------------------------------------------
  LCCollection *colNewPFO = evt->getCollection(_colNewPFOs);
  if (!colNewPFO) {
    cerr << "No NewPFOs Collection Found!" << endl;
    throw marlin::SkipEventException(this);
  }
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "NewPFOs Collection Found" << ends;
  // get the visible energy
  Int_t nPFOs = colNewPFO->getNumberOfElements();
  Double_t eVis = 0.;
  Int_t nParticles = 0;
  for (Int_t i=0;i<nPFOs;i++) {
    ReconstructedParticle *pfo = dynamic_cast<ReconstructedParticle*>(colNewPFO->getElementAt(i));
    eVis += pfo->getEnergy();
    if (pfo->getEnergy() >= fEnergyCut) nParticles++;
  }

  // ------------------------------------------------
  // -- read out the MCTruthLink information
  // ------------------------------------------------
  LCCollection *colMCTL = evt->getCollection(_colMCTL);
  LCRelationNavigator *navMCTL = new LCRelationNavigator(colMCTL);
  
  // ------------------------------------------------
  // -- read out the Leptons information
  // ------------------------------------------------
  LCCollection *colLep = evt->getCollection(_colLeptons);
  if (!colLep) {
    cerr << "No Leptons Collection Found!" << endl;
    throw marlin::SkipEventException(this);
  }
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "Leptons Collection Found" << ends;
  ReconstructedParticle *lepton1 = dynamic_cast<ReconstructedParticle*>(colLep->getElementAt(0));
  ReconstructedParticle *lepton2 = dynamic_cast<ReconstructedParticle*>(colLep->getElementAt(1));
  //  eVis += lepton1->getEnergy();
  //  eVis += lepton2->getEnergy();
  // get the truth information of the two selected leptons
  Int_t mcpdg1,mcpdg2,mcoriginal1,mcoriginal2;
  mcpdg1 = 0;
  mcpdg2 = 0;
  mcoriginal1 = 0;
  mcoriginal2 = 0;
  LCObjectVec vecMCTL1 = navMCTL->getRelatedToObjects(lepton1);
  LCObjectVec vecMCTL2 = navMCTL->getRelatedToObjects(lepton2);
  if (vecMCTL1.size() > 0) {
    MCParticle *mcPart1 = dynamic_cast<MCParticle *>(vecMCTL1[0]);
    mcpdg1 = mcPart1->getPDG();
    mcoriginal1 = getOriginalPDG(mcPart1);
  }
  if (vecMCTL2.size() > 0) {
    MCParticle *mcPart2 = dynamic_cast<MCParticle *>(vecMCTL2[0]);
    mcpdg2 = mcPart2->getPDG();
    mcoriginal2 = getOriginalPDG(mcPart2);
  }
  // get the momentum and cone energy of the two leptons
  //  Double_t massZ = getInvariantMass(lepton1,lepton2);
  TVector3 momentumLepton1,momentumLepton2;
  Double_t pLepton1,pLepton2,eConeLepton1,eConeLepton2,eConeCLepton1,eConeCLepton2;
  Double_t eLepton1,eLepton2;
  // sort the two leptons
  if (lepton1->getEnergy() > lepton2->getEnergy()) {
    momentumLepton1 = TVector3(lepton1->getMomentum());
    pLepton1 = momentumLepton1.Mag();
    eLepton1 = lepton1->getEnergy();
    eConeLepton1 = getConeEnergy(lepton1,colPFO,fCosConeCut,0);
    eConeCLepton1 = getConeEnergy(lepton1,colPFO,fCosConeCut,1);
    momentumLepton2 = TVector3(lepton2->getMomentum());
    pLepton2 = momentumLepton2.Mag();
    eLepton2 = lepton2->getEnergy();
    eConeLepton2 = getConeEnergy(lepton2,colPFO,fCosConeCut,0);
    eConeCLepton2 = getConeEnergy(lepton2,colPFO,fCosConeCut,1);
  }
  else {
    momentumLepton1 = TVector3(lepton2->getMomentum());
    pLepton1 = momentumLepton1.Mag();
    eLepton1 = lepton2->getEnergy();
    eConeLepton1 = getConeEnergy(lepton2,colPFO,fCosConeCut,0);
    eConeCLepton1 = getConeEnergy(lepton2,colPFO,fCosConeCut,1);
    momentumLepton2 = TVector3(lepton1->getMomentum());
    pLepton2 = momentumLepton2.Mag();
    eLepton2 = lepton1->getEnergy();
    eConeLepton2 = getConeEnergy(lepton1,colPFO,fCosConeCut,0);
    eConeCLepton2 = getConeEnergy(lepton1,colPFO,fCosConeCut,1);
  }
  //  TVector3 momentumZ  = momentumLepton1 + momentumLepton2;
  // get the lepton type
  Double_t iLepType1 = getLeptonID(lepton1);
  Double_t iLepType2 = getLeptonID(lepton2);

  // ------------------------------------------------
  // -- read out the ZPFOs information
  // ------------------------------------------------
  LCCollection *colZ = evt->getCollection(_colZPFOs);
  if (!colZ) {
    cerr << "No ZPFOs Collection Found!" << endl;
    throw marlin::SkipEventException(this);
  }
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "ZPFOs Collection Found" << ends;
  Double_t mvalepminus = colZ->getParameters().getFloatVal("MVALepMinus");
  Double_t mvalepplus = colZ->getParameters().getFloatVal("MVALepPlus");

  ReconstructedParticle *z = dynamic_cast<ReconstructedParticle*>(colZ->getElementAt(0));
  Double_t energyZ = z->getEnergy();
  TVector3 momentumZ = TVector3(z->getMomentum());
  Double_t massZ = z->getMass();
  eVis += energyZ;

  // ------------------------------------------------
  // -- read out the NewJets information
  // ------------------------------------------------
  //  LCCollection *colJet_2Jets = evt->getCollection("NewJets_2Jets");
  LCCollection *colJet_2Jets = evt->getCollection(_colNewJets2);
  Int_t nJets_2Jets = colJet_2Jets->getNumberOfElements();
  if (!colJet_2Jets || nJets_2Jets != 2) {
    cerr << "No NewJets_2Jets Collection Found!" << endl;
    throw marlin::SkipEventException(this);
  }
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "NewJets_2Jets Collection Found" << ends;
#if 1 // Durham_Jets
  Double_t yMinus_2Jets = colJet_2Jets->getParameters().getFloatVal("YMinus");
  Double_t yPlus_2Jets = colJet_2Jets->getParameters().getFloatVal("YPlus");
#endif
  ReconstructedParticle *jets_2Jets[2];
  for (Int_t i=0;i<nJets_2Jets;i++) {
    jets_2Jets[i] = dynamic_cast<ReconstructedParticle*>(colJet_2Jets->getElementAt(i));
  }
  TVector3 momentum1_2Jets = jets_2Jets[0]->getMomentum();
  TVector3 momentum2_2Jets = jets_2Jets[1]->getMomentum();
  Double_t pJ1_2Jets = momentum1_2Jets.Mag();
  Double_t pJ2_2Jets = momentum2_2Jets.Mag();
  Double_t cosJ1_2Jets = momentum1_2Jets.CosTheta();
  Double_t cosJ2_2Jets = momentum2_2Jets.CosTheta();
  Double_t cosJ12_2Jets = momentum1_2Jets.Dot(momentum2_2Jets)/pJ1_2Jets/pJ2_2Jets;
  Double_t cosJ1Z_2Jets = momentum1_2Jets.Dot(momentumZ)/pJ1_2Jets/momentumZ.Mag();
  Double_t cosJ2Z_2Jets = momentum2_2Jets.Dot(momentumZ)/pJ2_2Jets/momentumZ.Mag();
  Double_t cosJZMax_2Jets = -1.;
  if (cosJ1Z_2Jets > cosJ2Z_2Jets) {
    cosJZMax_2Jets = cosJ1Z_2Jets;
  }
  else {
    cosJZMax_2Jets = cosJ2Z_2Jets;
  }
#if 0 // RefinedJets
  PIDHandler pidh_2Jets (colJet_2Jets);
  Int_t algo_y_2Jets = pidh_2Jets.getAlgorithmID("yth");
  const ParticleID & ythID_2Jets = pidh_2Jets.getParticleID(jets_2Jets[0], algo_y_2Jets);
  FloatVec params_y_2Jets = ythID_2Jets.getParameters();
  Double_t yMinus_2Jets = params_y_2Jets[pidh_2Jets.getParameterIndex(algo_y_2Jets, "y12")];
  Double_t yPlus_2Jets  = params_y_2Jets[pidh_2Jets.getParameterIndex(algo_y_2Jets, "y23")];
#endif

  // ------------------------------------------------
  // -- read out the NewJets information
  // ------------------------------------------------
  LCCollection *colJet = evt->getCollection(_colNewJets);
  Int_t nJets = colJet->getNumberOfElements();
  if (!colJet || nJets != 4) {
    cerr << "No NewJets Collection Found!" << endl;
    throw marlin::SkipEventException(this);
  }
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "NewJets Collection Found" << ends;
  //  Double_t yMinus = colJet->getParameters().getFloatVal("YMinus");
  //  Double_t yPlus = colJet->getParameters().getFloatVal("YPlus");
  ReconstructedParticle *jets[4];
  // flavor tagging information
  PIDHandler pidh (colJet);
  //  Int_t algo = pidh.getAlgorithmID("LCFIFlavourTag");
  Int_t algo = pidh.getAlgorithmID("lcfiplus");
  Double_t FLV[4][11];
  Int_t nPFOsJMin = 9999.;
  for (Int_t i=0;i<nJets;i++) {
    jets[i] = dynamic_cast<ReconstructedParticle*>(colJet->getElementAt(i));
    const ParticleID & jetID = pidh.getParticleID(jets[i], algo);
    FloatVec params = jetID.getParameters();
    FLV[i][0] = params[pidh.getParameterIndex(algo, "BTag")];
    FLV[i][1] = params[pidh.getParameterIndex(algo, "CTag")];
    FLV[i][2] = params[pidh.getParameterIndex(algo, "BCTag")];
    Int_t npfos_tmp = jets[i]->getParticles().size();
    if (npfos_tmp < nPFOsJMin) nPFOsJMin = npfos_tmp;
  }
  Int_t algo_y = pidh.getAlgorithmID("yth");
  const ParticleID & ythID = pidh.getParticleID(jets[0], algo_y);
  FloatVec params_y = ythID.getParameters();
  Double_t yMinus = params_y[pidh.getParameterIndex(algo_y, "y34")];
  Double_t yPlus  = params_y[pidh.getParameterIndex(algo_y, "y45")];
  // jet pairing for least chi2
  Int_t nj11,nj12,nj21,nj22;
  Double_t chi2Min=99999.;
  Double_t massH1=-1.,massH2=-1.;
  Bool_t kPairing = kFALSE;
  for (Int_t i=1;i<nJets;i++) {
    Double_t massH1_tmp = getInvariantMass(jets[0],jets[i]);
    if (TMath::Abs(massH1_tmp-kMassH) > fMassHCut) continue;  // require |M(H1) - 120| < 80
    for (Int_t j=1;j<nJets;j++) {
      if (j != i) {
	Double_t massH2_tmp = getInvariantMass(jets[j],jets[6-i-j]);
	if (TMath::Abs(massH2_tmp-kMassH) > fMassHCut) continue;  // require |M(H2) - 120| < 80
	Double_t massZ_tmp = massZ;
	Double_t chi2 = TMath::Power((massH1_tmp - kMassH)/kSigmaMh, 2.)      
	  + TMath::Power((massH2_tmp - kMassH)/kSigmaMh, 2.)
	  + TMath::Power((massZ_tmp - kMassZ)/kSigmaMz, 2.);
	if (chi2 < chi2Min) {
	  chi2Min = chi2;
	  nj11 = 0;
	  nj12 = i;
	  nj21 = j;
	  nj22 = 6-i-j;
	  massH1 = massH1_tmp;
	  massH2 = massH2_tmp;
	  kPairing = kTRUE;
	}
      }
    }
  }
  if (!kPairing) throw marlin::SkipEventException(this);
  hStatAnl->Fill(++selid);
  gCutName[(Int_t)selid] << "|M(H)-125| < 80" << ends;

  // ------------------------------------------------
  // -- get the useful physical quantities and save them to ntuple
  // ------------------------------------------------
  // get the 4-momenta of jets
  ReconstructedParticle *jet11 = jets[nj11];
  ReconstructedParticle *jet12 = jets[nj12];
  ReconstructedParticle *jet21 = jets[nj21];
  ReconstructedParticle *jet22 = jets[nj22];
  Int_t nPFOsJ11 = jet11->getParticles().size();
  Int_t nPFOsJ12 = jet12->getParticles().size();
  Int_t nPFOsJ21 = jet21->getParticles().size();
  Int_t nPFOsJ22 = jet22->getParticles().size();
  TVector3 momentum_j11 = TVector3(jet11->getMomentum());
  TVector3 momentum_j12 = TVector3(jet12->getMomentum());
  TVector3 momentum_j21 = TVector3(jet21->getMomentum());
  TVector3 momentum_j22 = TVector3(jet22->getMomentum());
  Double_t px_j11 = momentum_j11[0];
  Double_t py_j11 = momentum_j11[1];
  Double_t pz_j11 = momentum_j11[2];
  Double_t p_j11  = momentum_j11.Mag();
  Double_t e_j11  = jet11->getEnergy();
  Double_t px_j12 = momentum_j12[0];
  Double_t py_j12 = momentum_j12[1];
  Double_t pz_j12 = momentum_j12[2];
  Double_t p_j12  = momentum_j12.Mag();
  Double_t e_j12  = jet12->getEnergy();
  Double_t px_j21 = momentum_j21[0];
  Double_t py_j21 = momentum_j21[1];
  Double_t pz_j21 = momentum_j21[2];
  Double_t p_j21  = momentum_j21.Mag();
  Double_t e_j21  = jet21->getEnergy();
  Double_t px_j22 = momentum_j22[0];
  Double_t py_j22 = momentum_j22[1];
  Double_t pz_j22 = momentum_j22[2];
  Double_t p_j22  = momentum_j22.Mag();
  Double_t e_j22  = jet22->getEnergy();
  // get the invariant mass of two Higgs
  Double_t massHH = sqrt((e_j11+e_j12+e_j21+e_j22)*(e_j11+e_j12+e_j21+e_j22)-
			 (px_j11+px_j12+px_j21+px_j22)*(px_j11+px_j12+px_j21+px_j22)-
			 (py_j11+py_j12+py_j21+py_j22)*(py_j11+py_j12+py_j21+py_j22)-
			 (pz_j11+pz_j12+pz_j21+pz_j22)*(pz_j11+pz_j12+pz_j21+pz_j22));
  // get the ploar angular distribution of Z, H1, H2
  //  TVector3 momentumZ  = momentumLepton1 + momentumLepton2;
  TVector3 momentumH1 = momentum_j11 + momentum_j12;
  TVector3 momentumH2 = momentum_j21 + momentum_j22;
  Double_t cosThetaZ  = momentumZ.CosTheta();
  Double_t cosThetaH1 = momentumH1.CosTheta();
  Double_t cosThetaH2 = momentumH2.CosTheta();
  // get the decay angular distribution of Z, H1, H2
  Double_t pZ  = momentumZ.Mag();
  Double_t pH1 = momentumH1.Mag();
  Double_t pH2 = momentumH2.Mag();
  //  Double_t energyZ = lepton1->getEnergy() + lepton2->getEnergy();
  Double_t energyH1 = e_j11 + e_j12;
  Double_t energyH2 = e_j21 + e_j22;
  TLorentzVector lortzZ     = TLorentzVector(momentumZ,energyZ);
  TLorentzVector lortzLep1  = TLorentzVector(momentumLepton1,eLepton1);
  TLorentzVector lortzLep2  = TLorentzVector(momentumLepton2,eLepton2);
  TLorentzVector lortzH1    = TLorentzVector(momentumH1,energyH1);
  TLorentzVector lortzH2    = TLorentzVector(momentumH2,energyH2);
  TLorentzVector lortz_j11  = TLorentzVector(momentum_j11,e_j11);
  TLorentzVector lortz_j12  = TLorentzVector(momentum_j12,e_j12);
  TLorentzVector lortz_j21  = TLorentzVector(momentum_j21,e_j21);
  TLorentzVector lortz_j22  = TLorentzVector(momentum_j22,e_j22);
  TVector3 boostZ  = lortzZ.BoostVector();
  lortzLep1.Boost(-boostZ);
  lortzLep2.Boost(-boostZ);
  TVector3 pLep1 = lortzLep1.Vect();
  TVector3 pLep2 = lortzLep2.Vect();
  Double_t cos1Z = pLep1.Dot(boostZ)/pLep1.Mag()/boostZ.Mag();
  Double_t cos2Z = pLep2.Dot(boostZ)/pLep2.Mag()/boostZ.Mag();
  TVector3 boostH1 = lortzH1.BoostVector();
  lortz_j11.Boost(-boostH1);
  lortz_j12.Boost(-boostH1);
  TVector3 pN_j11 = lortz_j11.Vect();
  TVector3 pN_j12 = lortz_j12.Vect();
  Double_t cos1H1 = pN_j11.Dot(boostH1)/pN_j11.Mag()/boostH1.Mag();
  Double_t cos2H1 = pN_j12.Dot(boostH1)/pN_j12.Mag()/boostH1.Mag();
  TVector3 boostH2 = lortzH2.BoostVector();
  lortz_j21.Boost(-boostH2);
  lortz_j22.Boost(-boostH2);
  TVector3 pN_j21 = lortz_j21.Vect();
  TVector3 pN_j22 = lortz_j22.Vect();
  Double_t cos1H2 = pN_j21.Dot(boostH2)/pN_j21.Mag()/boostH2.Mag();
  Double_t cos2H2 = pN_j22.Dot(boostH2)/pN_j22.Mag()/boostH2.Mag();
  // get the flavor tagging information
  Double_t bProb_j11 = FLV[nj11][0];
  Double_t bProb_j12 = FLV[nj12][0];
  Double_t bProb_j21 = FLV[nj21][0];
  Double_t bProb_j22 = FLV[nj22][0];
  // get the two most like b-jet
  Int_t nbmax1,nbmax2,nbmax3,nbmax4;
  Double_t bmax1=0.,bmax2=0.,bmax3=0.,bmax4=0.;
  for (Int_t i=0;i<4;i++) {
    if (FLV[i][0] >= bmax1) {
      nbmax1 = i;
      bmax1 = FLV[i][0];
    }
  }
  for (Int_t j=0;j<4;j++) {
    if (j != nbmax1 && FLV[j][0] >= bmax2) {
      nbmax2 = j;
      bmax2 = FLV[j][0];
    }
  }
  for (Int_t k=0;k<4;k++) {
    if (k != nbmax1 && k != nbmax2 && FLV[k][0] >= bmax3) {
      nbmax3 = k;
      bmax3 = FLV[k][0];
    }
  }
  nbmax4 = 6-nbmax1-nbmax2-nbmax3;
  bmax4 = FLV[nbmax4][0];
  Double_t bcbmax1=0.,bcbmax2=0.,bcbmax3=0.,bcbmax4=0.;
  bcbmax1 = FLV[nbmax1][2];
  bcbmax2 = FLV[nbmax2][2];
  bcbmax3 = FLV[nbmax3][2];
  bcbmax4 = FLV[nbmax4][2];
  Double_t cbmax1=0.,cbmax2=0.,cbmax3=0.,cbmax4=0.;
  cbmax1 = FLV[nbmax1][1];
  cbmax2 = FLV[nbmax2][1];
  cbmax3 = FLV[nbmax3][1];
  cbmax4 = FLV[nbmax4][1];
  ReconstructedParticle *jetbmax1 = jets[nbmax1];
  ReconstructedParticle *jetbmax2 = jets[nbmax2];
  ReconstructedParticle *jetbmax3 = jets[nbmax3];
  ReconstructedParticle *jetbmax4 = jets[nbmax4];
  Int_t nPFOsBmax1 = jetbmax1->getParticles().size();
  Int_t nPFOsBmax2 = jetbmax2->getParticles().size();
  Int_t nPFOsBmax3 = jetbmax3->getParticles().size();
  Int_t nPFOsBmax4 = jetbmax4->getParticles().size();
  Double_t massBmax34 = getInvariantMass(jetbmax3,jetbmax4);
  Double_t massBmax12 = getInvariantMass(jetbmax1,jetbmax2);
  TVector3 pjbmax1 = TVector3(jetbmax1->getMomentum());
  TVector3 pjbmax2 = TVector3(jetbmax2->getMomentum());
  Double_t cosBmax12 = pjbmax1.Dot(pjbmax2)/pjbmax1.Mag()/pjbmax2.Mag();
  // sort with momentum
  Double_t p1st,p2nd,p3rd,cos1st,cos2nd,cos3rd;
  Double_t cos1st1,cos1st2,cos2nd1,cos2nd2,cos3rd1,cos3rd2;
  if (pZ >= pH1 && pH1 >= pH2) {
    p1st = pZ;
    p2nd = pH1;
    p3rd = pH2;
    cos1st = cosThetaZ;
    cos2nd = cosThetaH1;
    cos3rd = cosThetaH2;
    cos1st1 = cos1Z;
    cos1st2 = cos2Z;
    cos2nd1 = cos1H1;
    cos2nd2 = cos2H1;
    cos3rd1 = cos1H2;
    cos3rd2 = cos2H2;
  }
  else if (pZ >= pH2 && pH2 >= pH1) {
    p1st = pZ;
    p2nd = pH2;
    p3rd = pH1;
    cos1st = cosThetaZ;
    cos2nd = cosThetaH2;
    cos3rd = cosThetaH1;
    cos1st1 = cos1Z;
    cos1st2 = cos2Z;
    cos2nd1 = cos1H2;
    cos2nd2 = cos2H2;
    cos3rd1 = cos1H1;
    cos3rd2 = cos2H1;
  }
  else if (pH1 >= pH2 && pH2 >= pZ) {
    p1st = pH1;
    p2nd = pH2;
    p3rd = pZ;
    cos1st = cosThetaH1;
    cos2nd = cosThetaH2;
    cos3rd = cosThetaZ;
    cos1st1 = cos1H1;
    cos1st2 = cos2H1;
    cos2nd1 = cos1H2;
    cos2nd2 = cos2H2;
    cos3rd1 = cos1Z;
    cos3rd2 = cos2Z;
  }
  else if (pH1 >= pZ && pZ >= pH2) {
    p1st = pH1;
    p2nd = pZ;
    p3rd = pH2;
    cos1st = cosThetaH1;
    cos2nd = cosThetaZ;
    cos3rd = cosThetaH2;
    cos1st1 = cos1H1;
    cos1st2 = cos2H1;
    cos2nd1 = cos1Z;
    cos2nd2 = cos2Z;
    cos3rd1 = cos1H2;
    cos3rd2 = cos2H2;
  }
  else if (pH2 >= pH1 && pH1 >= pZ) {
    p1st = pH2;
    p2nd = pH1;
    p3rd = pZ;
    cos1st = cosThetaH2;
    cos2nd = cosThetaH1;
    cos3rd = cosThetaZ;
    cos1st1 = cos1H2;
    cos1st2 = cos2H2;
    cos2nd1 = cos1H1;
    cos2nd2 = cos2H1;
    cos3rd1 = cos1Z;
    cos3rd2 = cos2Z;
  }
  else if (pH2 >= pZ && pZ >= pH1) {
    p1st = pH2;
    p2nd = pZ;
    p3rd = pH1;
    cos1st = cosThetaH2;
    cos2nd = cosThetaZ;
    cos3rd = cosThetaH1;
    cos1st1 = cos1H2;
    cos1st2 = cos2H2;
    cos2nd1 = cos1Z;
    cos2nd2 = cos2Z;
    cos3rd1 = cos1H1;
    cos3rd2 = cos2H1;
  }

  // fill the ntuple
  Double_t data[200];
  data[ 0] = eVis;
  data[ 1] = massZ;
  data[ 2] = massH1;
  data[ 3] = massH2;
  data[ 4] = massHH;
  data[ 5] = pZ;
  data[ 6] = pH1;
  data[ 7] = pH2;
  data[ 8] = cosThetaZ;
  data[ 9] = cosThetaH1;
  data[10] = cosThetaH2;
  data[11] = yMinus;
  data[12] = yPlus;
  data[13] = nHbb;
  data[14] = nJets;
  data[15] = chi2Min;
  data[16] = pLepton1;
  data[17] = bProb_j11;
  data[18] = bProb_j12;
  data[19] = bProb_j21;
  data[20] = bProb_j22;
  data[21] = px_j11;
  data[22] = py_j11;
  data[23] = pz_j11;
  data[24] = e_j11;
  data[25] = px_j12;
  data[26] = py_j12;
  data[27] = pz_j12;
  data[28] = e_j12;
  data[29] = px_j21;
  data[30] = py_j21;
  data[31] = pz_j21;
  data[32] = e_j21;
  data[33] = px_j22;
  data[34] = py_j22;
  data[35] = pz_j22;
  data[36] = e_j22;
  data[37] = nParticles;
  data[38] = pLepton2;
  data[39] = eConeLepton1;
  data[40] = eConeLepton2;
  data[41] = eConeCLepton1;
  data[42] = eConeCLepton2;
  data[43] = nPFOsJ11;
  data[44] = nPFOsJ12;
  data[45] = nPFOsJ21;
  data[46] = nPFOsJ22;
  data[47] = principleThrust;
  data[48] = majorThrust;
  data[49] = minorThrust;
  data[50] = cos1Z;
  data[51] = cos2Z;
  data[52] = cos1H1;
  data[53] = cos2H1;
  data[54] = cos1H2;
  data[55] = cos2H2;
  data[56] = p1st;
  data[57] = p2nd;
  data[58] = p3rd;
  data[59] = cos1st;
  data[60] = cos2nd;
  data[61] = cos3rd;
  data[62] = cos1st1;
  data[63] = cos1st2;
  data[64] = cos2nd1;
  data[65] = cos2nd2;
  data[66] = cos3rd1;
  data[67] = cos3rd2;
  data[68] = eLepton1;
  data[69] = momentumLepton1.X();
  data[70] = momentumLepton1.Y();
  data[71] = momentumLepton1.Z();
  data[72] = eLepton2;
  data[73] = momentumLepton2.X();
  data[74] = momentumLepton2.Y();
  data[75] = momentumLepton2.Z();
  data[76] = cosThrustAxis;
  data[77] = cosBmax12;
  data[78] = bcbmax1;
  data[79] = bcbmax2;
  data[80] = bcbmax3;
  data[81] = bcbmax4;
  data[82] = cbmax1;
  data[83] = cbmax2;
  data[84] = cbmax3;
  data[85] = cbmax4;
  data[86] = yMinus_2Jets;
  data[87] = yPlus_2Jets;
  data[88] = pJ1_2Jets;
  data[89] = pJ2_2Jets;
  data[90] = cosJ1_2Jets;
  data[91] = cosJ2_2Jets;
  data[92] = cosJ12_2Jets;
  data[93] = cosJ1Z_2Jets;
  data[94] = cosJ2Z_2Jets;
  data[95] = cosJZMax_2Jets;
  data[96] = massBmax34;
  data[97] = massBmax12;
  data[98] = bmax1;
  data[99] = bmax2;
  data[100]= bmax3;
  data[101]= bmax4;
  data[102]= nPFOsBmax1;
  data[103]= nPFOsBmax2;
  data[104]= nPFOsBmax3;
  data[105]= nPFOsBmax4;
  data[106]= nPFOsJMin;
  data[107]= iLepType1;
  data[108]= iLepType2;
  data[109]= mvalepminus;
  data[110]= mvalepplus,
  hAnl->Fill(data);

  //-- note: this will not be printed if compiled w/o MARLINDEBUG=1 !

  //  streamlog_out(DEBUG) << "   processing event: " << evt->getEventNumber() 
  //		       << "   in run:  " << evt->getRunNumber() 
  //		       << std::endl ;

  //  _nEvt ++ ;

  last->cd();
}



void ZHHll4JAnalysisProcessor::check( LCEvent * evt ) { 
  // nothing to check here - could be used to h checkplots in reconstruction processor
}


void ZHHll4JAnalysisProcessor::end(){ 

  cerr << "ZHHll4JAnalysisProcessor::end()  " << name() 
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
         << "  " << setw(10) << static_cast<int>(hStatAnl->GetBinContent(id+1))
         << "  : " << gCutName[id].str().data() << endl;
  }
  cerr << "  -----------------------------------------------------------" << endl;
  
}
