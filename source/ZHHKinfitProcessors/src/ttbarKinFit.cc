#include "ttbarKinFit.h"
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;
using namespace std ;
using namespace CLHEP ;

template<class T>
double inv_mass(T* p1, T* p2){
  double e = p1->getEnergy()+p2->getEnergy() ;
  double px = p1->getMomentum()[0]+p2->getMomentum()[0];
  double py = p1->getMomentum()[1]+p2->getMomentum()[1];
  double pz = p1->getMomentum()[2]+p2->getMomentum()[2];
  return( sqrt( e*e - px*px - py*py - pz*pz  ) );
}

ttbarKinFit attbarKinFit ;

ttbarKinFit::ttbarKinFit() :
  Processor("ttbarKinFit"),
  m_nRun(0),
  m_nEvt(0),
  m_nRunSum(0),
  m_nEvtSum(0),
  m_Bfield(0.0),
  c(0.0),
  mm2m(0.0),
  eV2GeV(0.0),
  eB(0.0),
  m_pTFile(nullptr)
{
  //	modify processor description
  _description = "ttbarKinFit does a fit on ttbar events with 4-momentum conservation and possiblity for mass constraints" ;
  
  //	register steering parameters: name, description, class-variable, default value
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "LeptonCollectionName" ,
			  "Name of the Lepton collection"  ,
			  m_inputleptonCollection ,
			  std::string("LeptonPair")
			  );
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "JetCollectionName" ,
			  "Name of the Jet collection"  ,
			  m_inputJetCollection ,
			  std::string("Durham4Jets")
			  );

  registerInputCollection(LCIO::VERTEX,
			  "SLDVertexCollection" ,
			  "Name of Semi-Leptonic Decay Vertices Collection"  ,
			  m_inputSLDVertexCollection ,
			  std::string("SemiLeptonicDecayVertex")
			  );
  
  registerInputCollection(LCIO::LCRELATION,
			  "JetSLDRelationCollection",
			  "Name of the Jet-SemiLeptonicDecay Relation collection",
			  m_inputJetSLDLink,
			  std::string("JetSLDLinkName")
			  );
  
  registerInputCollection(LCIO::LCRELATION,
			  "SLDNeutrinoRelationCollection",
			  "Name of the JetSemiLeptonicDecayVertex-Neutrinos Relation collection",
			  m_inputSLDNuLink,
			  std::string("SLDNuLinkName")
			  );
  
  registerInputCollection(LCIO::LCRELATION,
			  "recoNumcNuLinkName",
			  "Name of the reconstructedNeutrino-trueNeutrino input Link collection",
			  m_recoNumcNuLinkName,
			  std::string("recoNumcNuLinkName")
			  );
  
  registerInputCollection(LCIO::MCPARTICLE,
			  "MCParticleCollection" ,
			  "Name of the MCParticle collection"  ,
			  _MCParticleColllectionName ,
			  std::string("MCParticlesSkimmed")
			  );
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "RecoParticleCollection" ,
			  "Name of the ReconstructedParticles input collection"  ,
			  _recoParticleCollectionName ,
			  std::string("PandoraPFOs")
			  );

  registerInputCollection(LCIO::LCRELATION,
			  "RecoMCTruthLink",
			  "Name of the RecoMCTruthLink input collection"  ,
			  _recoMCTruthLink,
			  std::string("RecoMCTruthLink")
			  );
  
  registerProcessorParameter("whichSignature",
			     "Which signature; lvqqbb, tvqqbb or qqqqbb",
			     m_signature,
			     std::string("qqqqbb")
			     );
  
  
  registerProcessorParameter("ECM" ,
			     "Center-of-Mass Energy in GeV",
			     m_ECM,
			     float(500.f)
			     );
  
  registerProcessorParameter("ISRPzMax" ,
			     "Maximum possible energy for a single ISR photon",
			     m_isrpzmax,
			     float(125.6f)
			     );

  registerProcessorParameter("SigmaInvPtScaleFactor" ,
			     "Factor for scaling up inverse pT error",
			     m_SigmaInvPtScaleFactor,
			     float(1.0f)
			     );

  registerProcessorParameter("SigmaEnergyScaleFactor" ,
			     "Factor for scaling up energy error",
			     m_SigmaEnergyScaleFactor,
			     float(1.0f)
			     );
  
  registerProcessorParameter("SigmaAnglesScaleFactor" ,
			     "Factor for scaling up angular errors",
			     m_SigmaAnglesScaleFactor,
			     float(1.0f)
			     );

  registerProcessorParameter("NeutrinoPxPyError" ,
			     "Error on px and py for W->invisible fit object",
			     m_NeutrinoPxPyError,
			     float(10.0f)
			     );
  
  registerProcessorParameter("NeutrinoPzError" ,
			     "Error on pz for W->invisible fit object",
			     m_NeutrinoPzError,
			     float(10.0f)
			     );
  
  registerProcessorParameter("includeISR",
			     "Include ISR in fit hypothesis; false: without ISR , true: with ISR",
			     m_fitISR,
			     bool(true)
			     );
  
  registerProcessorParameter("solveNuCorrection",
			     "Solve the nu correction in fit hypothesis",
			     m_solveNu,
			     bool(false)
			     );
  
  registerProcessorParameter("fitter" ,
			     "0 = OPALFitter, 1 = NewFitter, 2 = NewtonFitter",
			     m_fitter,
			     int(0)
			     );
  
  registerProcessorParameter("fithypothesis",
			     "name of the fit hypothesis",
			     m_fithypothesis,
			     std::string("")
			     );
  
  registerProcessorParameter("outputFilename",
			     "name of output root file",
			     m_outputFile,
			     std::string("")
			     );
  
  registerProcessorParameter("treeName",
			     "name of output ROOT TTree; if empty, will be chosen as KinFit_<FitHypothesis>",
			     m_treeName,
			     std::string("")
			     );
  
  registerProcessorParameter("traceall" ,
			     "set true if every event should be traced",
			     m_traceall,
			     (bool)false
			     );

  registerProcessorParameter("ievttrace" ,
			     "number of individual event to be traced",
			     m_ievttrace,
			     (int)0
			     );

	registerProcessorParameter("JetTaggingPIDAlgorithm",
            "Name of flavor tagging algo",
            m_JetTaggingPIDAlgorithm,
            std::string("weaver")
            );

  registerProcessorParameter("JetTaggingPIDParameterB",
            "B parameter",
            m_JetTaggingPIDParameterB,
            std::string("mc_b")
            );

  registerProcessorParameter("JetTaggingPIDParameterBbar",
            "Bbar parameter",
            m_JetTaggingPIDParameterBbar,
            std::string("mc_bbar")
            );

  registerProcessorParameter("JetTaggingPIDParameters",
            "Number of jet should be in the event",
            m_JetTaggingPIDParameters,
            std::vector<std::string>{"mc_b mc_bbar mc_c mc_cbar mc_d mc_dbar mc_g mc_s mc_sbar mc_u mc_ubar"}
            );
  
  // Outputs: Fitted jet and leptons and their prefit counterparts, the neutrino correction, and pulls
  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "outputLeptonCollection" ,
			   "Name of output fitted lepton collection"  ,
			   m_outputLeptonCollection ,
			   std::string("Leptons_KinFit")
			   );
  
  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "outputJetCollection",
			   "Name of output fitted jet collection",
			   m_outputJetCollection,
			   std::string("JetsKinFit")
			   );
	
  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "outputStartLeptonCollection" ,
			   "Name of output prefit lepton collection"  ,
			   m_outputStartLeptonCollection ,
			   std::string("Leptons_PreFit")
			   );
  
  registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "outputStartJetCollection",
			   "Name of output prefit jet collection",
			   m_outputStartJetCollection,
			   std::string("JetsPreFit")
			   );
  /*
    registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
    "outputNeutrinoCollection",
    "Name of output neutrino collection",
    m_outputNeutrinoCollection,
    std::string("SLDCorrections")
    );
  */
  /*registerOutputCollection(       LCIO::LCFLOATVEC,
    "NuEnergyOutputCollection",
    "Output Nu Energy Collection" ,
    m_outputNuEnergyCollection,
    std::string("NuEnergy")
    );
  */
  
  registerOutputCollection(LCIO::LCFLOATVEC,
			   "LeptonPullsOutputCollection",
			   "Output LeptonPulls (invPt, theta, phi)  Collection" ,
			   _OutLeptonPullsCol,
			   std::string("LeptonPulls"));
  
  registerOutputCollection(LCIO::LCFLOATVEC,
			   "JetPullsOutputCollection",
			   "Output JetPulls (E, theta, phi)  Collection" ,
			   _OutJetPullsCol,
			   std::string("JetPulls"));
}

void ttbarKinFit::init()
{
  //	usually a good idea to
  streamlog_out(DEBUG) << "   init called  " << std::endl;
  this->Clear();
  m_nRun = 0;
  m_nEvt = 0;
  m_nRunSum = 0;
  m_nEvtSum = 0;
  DDMarlinCED::init(this);
  
  m_Bfield = MarlinUtil::getBzAtOrigin();
  streamlog_out(DEBUG0) << " BField =  "<< m_Bfield << " Tesla" << std::endl ;
  c = 2.99792458e8;
  mm2m = 1e-3;
  eV2GeV = 1e-9;
  eB = m_Bfield * c * mm2m * eV2GeV;
  
  
  b = (double) 0.00464564 * ( std::log( m_ECM * m_ECM * 3814714. ) - 1. );
  //	  = 2*alpha/pi*( ln(s/m_e^2)-1 )
  ISRPzMaxB = std::pow((double)m_isrpzmax,b);
  
  printParameters();

  const char* treeName;
  if (m_treeName == "")
    treeName = (std::string("KinFit_") + m_fithypothesis).c_str();
  else
    treeName = m_treeName.c_str();
  
  streamlog_out(MESSAGE) << "Writing to TTree " << treeName << std::endl;
	m_pTTree = new TTree(treeName, treeName);
  
  if (m_outputFile.size()) {
    m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
    m_pTTree->SetDirectory(m_pTFile);
  }

  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("nJets",&m_nJets,"nJets/I") ;
  m_pTTree->Branch("nLeptons",&m_nLeps,"nLeptons/I") ;
  m_pTTree->Branch("nSLDecayBHadron",&m_nSLDecayBHadron,"nSLDecayBHadron/I") ;
  m_pTTree->Branch("nSLDecayCHadron",&m_nSLDecayCHadron,"nSLDecayCHadron/I") ;
  m_pTTree->Branch("nSLDecayTauLepton",&m_nSLDecayTauLepton,"nSLDecayTauLepton/I") ;
  m_pTTree->Branch("nSLDecayTotal",&m_nSLDecayTotal,"nSLDecayTotal/I") ;
  m_pTTree->Branch("nCorrectedSLD",&m_nCorrectedSLD,"nCorrectedSLD/I") ;
  m_pTTree->Branch("ISREnergyTrue",&m_ISREnergyTrue,"ISREnergyTrue/F") ;
  m_pTTree->Branch("BSEnergyTrue",&m_BSEnergyTrue,"BSEnergyTrue/F") ;
  m_pTTree->Branch("W1MassHardProcess",&m_W1MassHardProcess,"W1MassHardProcess/F") ;
  m_pTTree->Branch("W2MassHardProcess",&m_W2MassHardProcess,"W2MassHardProcess/F") ;
  m_pTTree->Branch("tMassHardProcess",&m_tMassHardProcess,"tMassHardProcess/F") ;
  m_pTTree->Branch("tbarMassHardProcess",&m_tbarMassHardProcess,"tbarMassHardProcess/F") ;
  m_pTTree->Branch("ttbarMassHardProcess",&m_ttbarMassHardProcess,"ttbarMassHardProcess/F") ;
  m_pTTree->Branch( "FitErrorCode" , &m_FitErrorCode , "FitErrorCode/I" );
  m_pTTree->Branch( "invalidCovMatrixAtJet" , &m_invalidCovMatrixAtJet , "invalidCovMatrixAtJet/I" );
  m_pTTree->Branch( "pxcstartvalue", &m_pxcstartvalue, "pxcstartvalue/F" );
  m_pTTree->Branch( "pycstartvalue", &m_pycstartvalue, "pycstartvalue/F" );
  m_pTTree->Branch( "pzcstartvalue", &m_pzcstartvalue, "pzcstartvalue/F" );
  m_pTTree->Branch( "ecstartvalue", &m_ecstartvalue, "ecstartvalue/F" );
  m_pTTree->Branch( "W1MassBeforeFit" , &m_W1MassBeforeFit , "W1MassBeforeFit/F" );
  m_pTTree->Branch( "W2MassBeforeFit" , &m_W2MassBeforeFit , "W2MassBeforeFit/F" );
  m_pTTree->Branch( "tMassBeforeFit" , &m_tMassBeforeFit , "tMassBeforeFit/F" );
  m_pTTree->Branch( "tbarMassBeforeFit" , &m_tbarMassBeforeFit , "tbarMassBeforeFit/F" );
  m_pTTree->Branch( "ttbarMassBeforeFit" , &m_ttbarMassBeforeFit , "ttbarMassBeforeFit/F" );
  m_pTTree->Branch( "ISREnergyBeforeFit" , &m_ISREnergyBeforeFit , "ISREnergyBeforeFit/F" );
  m_pTTree->Branch( "p1stBeforeFit" , &m_p1stBeforeFit , "p1stBeforeFit/F" );
  m_pTTree->Branch( "cosThBeforeFit" , &m_cos1stBeforeFit , "cosThBeforeFit/F" );

  m_pTTree->Branch( "W1MassAfterFit" , &m_W1MassAfterFit , "W1MassAfterFit/F" );
  m_pTTree->Branch( "W2MassAfterFit" , &m_W2MassAfterFit , "W2MassAfterFit/F" );
  m_pTTree->Branch( "tMassAfterFit" , &m_tMassAfterFit , "tMassAfterFit/F" );
  m_pTTree->Branch( "tbarMassAfterFit" , &m_tbarMassAfterFit , "tbarMassAfterFit/F" );
  m_pTTree->Branch( "ttbarMassAfterFit" , &m_ttbarMassAfterFit , "ttbarMassAfterFit/F" );
  m_pTTree->Branch( "ISREnergyAfterFit" , &m_ISREnergyAfterFit , "ISREnergyAfterFit/F" );
  m_pTTree->Branch( "p1stAfterFit" , &m_p1stAfterFit , "p1stAfterFit/F" );
  m_pTTree->Branch( "cosThAfterFit" , &m_cos1stAfterFit , "cosThAfterFit/F" );
  m_pTTree->Branch( "FitProbability" , &m_FitProbability , "FitProbability/F" );
  m_pTTree->Branch( "FitChi2" , &m_FitChi2 , "FitChi2/F" );
  m_pTTree->Branch( "perm" , &m_perm );
  m_pTTree->Branch( "PrefitJetFourMomentum", &m_PrefitJetFourMomentum);
  m_pTTree->Branch( "PostfitJetFourMomentum", &m_PostfitJetFourMomentum);
  m_pTTree->Branch( "pullJetEnergy" , &m_pullJetEnergy );
  m_pTTree->Branch( "pullJetTheta" , &m_pullJetTheta );
  m_pTTree->Branch( "pullJetPhi" , &m_pullJetPhi );
  m_pTTree->Branch( "pullLeptonInvPt" , &m_pullLeptonInvPt );
  m_pTTree->Branch( "pullLeptonTheta" , &m_pullLeptonTheta );
  m_pTTree->Branch( "pullLeptonPhi" , &m_pullLeptonPhi );
  m_pTTree->Branch( "pullInvPx" , &m_pullInvPx );
  m_pTTree->Branch( "pullInvPy" , &m_pullInvPy );
  m_pTTree->Branch( "pullInvPz" , &m_pullInvPz );
  m_pTTree->Branch( "TrueNeutrinoEnergy", &m_TrueNeutrinoEnergy );
  m_pTTree->Branch( "RecoNeutrinoEnergy", &m_RecoNeutrinoEnergy );
  m_pTTree->Branch( "RecoNeutrinoEnergyKinfit", &m_RecoNeutrinoEnergyKinfit );
  m_pTTree->Branch("Sigma_Px2",&m_Sigma_Px2);
  m_pTTree->Branch("Sigma_PxPy",&m_Sigma_PxPy);
  m_pTTree->Branch("Sigma_Py2",&m_Sigma_Py2);
  m_pTTree->Branch("Sigma_PxPz",&m_Sigma_PxPz);
  m_pTTree->Branch("Sigma_PyPz",&m_Sigma_PyPz);
  m_pTTree->Branch("Sigma_Pz2",&m_Sigma_Pz2);
  m_pTTree->Branch("Sigma_PxE",&m_Sigma_PxE);
  m_pTTree->Branch("Sigma_PyE",&m_Sigma_PyE);
  m_pTTree->Branch("Sigma_PzE",&m_Sigma_PzE);
  m_pTTree->Branch("Sigma_E2",&m_Sigma_E2);
  m_pTTree->Branch("bTags", &m_bTagValues);
  m_pTTree->Branch("cosbmax", &m_cosbmax, "cosbmax/F");
  m_pTTree->Branch("bmax1", &m_bmax1, "bmax1/F");
  m_pTTree->Branch("bmax2", &m_bmax2, "bmax2/F");
  m_pTTree->Branch("bmax3", &m_bmax3, "bmax3/F");
  m_pTTree->Branch("bmax4", &m_bmax4, "bmax4/F");
  m_pTTree->Branch("bmax5", &m_bmax5, "bmax5/F");
  m_pTTree->Branch("bmax6", &m_bmax6, "bmax6/F");
  m_pTTree->Branch("scoreB", &m_scoreB);
  m_pTTree->Branch("scoreBbar", &m_scoreBbar);


  streamlog_out(DEBUG) << "   init finished  " << std::endl;  
}

void ttbarKinFit::Clear()
{
  streamlog_out(DEBUG) << "   Clear called  " << std::endl;
  
  m_nJets = 0;
  m_nLeps = 0;
  m_nSLDecayBHadron = 0;
  m_nSLDecayCHadron = 0;
  m_nSLDecayTauLepton = 0;
  m_nSLDecayTotal = 0;
  m_nCorrectedSLD = 0;
  m_ISREnergyTrue = 0.0;
  m_BSEnergyTrue = 0.0;
  m_W1MassHardProcess = 0.0;
  m_W2MassHardProcess = 0.0;
  m_tMassHardProcess = 0.0;
  m_tbarMassHardProcess = 0.0;
  m_ttbarMassHardProcess = 0.0;
  m_FitErrorCode = 1;
  m_invalidCovMatrixAtJet = -1;
  m_pxcstartvalue = 0.0;
  m_pycstartvalue = 0.0;
  m_pzcstartvalue = 0.0;
  m_ecstartvalue = 0.0;
  m_W1MassBeforeFit = 0.0;
  m_W2MassBeforeFit = 0.0;
  m_tMassBeforeFit = 0.0;
  m_tbarMassBeforeFit = 0.0;
  m_ttbarMassBeforeFit = 0.0;
  m_ISREnergyBeforeFit = 0.0;
  m_p1stBeforeFit = 0.0;
  m_cos1stBeforeFit = 0.0;
  m_W1MassAfterFit = 0.0;
  m_W2MassAfterFit = 0.0;
  m_tMassAfterFit = 0.0;
  m_tbarMassAfterFit = 0.0;
  m_ttbarMassAfterFit = 0.0;
  m_ISREnergyAfterFit = 0.0;
  m_p1stAfterFit = 0.0;
  m_cos1stAfterFit = 0.0;
  m_FitProbability = 0.0;
  m_FitChi2 = 0.0;
  m_cosbmax = 0.;
  m_bmax1 = 0.;
  m_bmax2 = 0.;
  m_bmax3 = 0.;
  m_bmax4 = 0.;
  m_bmax5 = 0.;
  m_bmax6 = 0.;
  m_perm.clear();
  m_PrefitJetFourMomentum.clear();
  m_PostfitJetFourMomentum.clear();
  m_pullJetEnergy.clear();
  m_pullJetTheta.clear();
  m_pullJetPhi.clear();
  m_pullLeptonInvPt.clear();
  m_pullLeptonTheta.clear();
  m_pullLeptonPhi.clear();
  m_pullInvPx.clear();
  m_pullInvPy.clear();
  m_pullInvPz.clear();
  m_TrueNeutrinoEnergy.clear();
  m_RecoNeutrinoEnergy.clear();
  m_RecoNeutrinoEnergyKinfit.clear();
  m_Sigma_Px2.clear();
  m_Sigma_PxPy.clear();
  m_Sigma_Py2.clear();
  m_Sigma_PxPz.clear();
  m_Sigma_PyPz.clear();
  m_Sigma_Pz2.clear();
  m_Sigma_PxE.clear();
  m_Sigma_PyE.clear();
  m_Sigma_PzE.clear();
  m_Sigma_E2.clear();
  m_bTagsSorted.clear();
  m_jetTags.clear();

  std::fill(m_scoreB.begin(), m_scoreB.end(), 0.0);
  std::fill(m_scoreBbar.begin(), m_scoreBbar.end(), 0.0);
  std::fill(m_bTagValues.begin(), m_bTagValues.end(), -1.);
}

void ttbarKinFit::processRunHeader()
{
  m_nRun++ ;
}

void ttbarKinFit::processEvent( EVENT::LCEvent *pLCEvent )
{
  this->Clear();
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
  streamlog_out(WARNING) << "	////////////////////////////////// processing event " << m_nEvt << " in run " << m_nRun << " /////////////////////////////////////////////////" << std::endl;
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
  
  LCCollection *inputJetCollection = NULL;
  LCCollection *inputLeptonCollection = NULL;
  LCCollection *inputSLDecayCollection = NULL;
  LCCollection *inputMCParticleCollection = NULL;
  IMPL::LCCollectionVec* outputLeptonCollection = NULL;
  outputLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  //outputLeptonCollection->setSubset( true );
  IMPL::LCCollectionVec* outputJetCollection = NULL;
  outputJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  //outputJetCollection->setSubset( true );
  IMPL::LCCollectionVec* outputStartLeptonCollection = NULL;
  outputStartLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  //outputStartLeptonCollection->setSubset( true );
  IMPL::LCCollectionVec* outputStartJetCollection = NULL;
  outputStartJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  //outputStartJetCollection->setSubset( true );
  //LCCollectionVec *outputNuEnergyCollection = new LCCollectionVec(LCIO::LCFLOATVEC);

  LCRelationNavigator* JetSLDNav = NULL;
  LCRelationNavigator* SLDNuNav = NULL;
  LCRelationNavigator* NuMCNav = NULL;
  try {
    streamlog_out(DEBUG0) << "  getting lepton collection: " << m_inputleptonCollection << std::endl ;
    inputLeptonCollection = pLCEvent->getCollection( m_inputleptonCollection );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input lepton collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    streamlog_out(DEBUG0) << "  getting jet collection: " << m_inputJetCollection << std::endl ;
    inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input jet collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    streamlog_out(DEBUG0) << "  getting semi-leptonic vertex collection: " << m_inputSLDVertexCollection << std::endl ;
    inputSLDecayCollection = pLCEvent->getCollection( m_inputSLDVertexCollection );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input semi-leptonic vertex collection collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    streamlog_out(DEBUG0) << "  getting mc particle collection: " << _MCParticleColllectionName << std::endl ;
    inputMCParticleCollection = pLCEvent->getCollection( _MCParticleColllectionName );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input mc particle collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    JetSLDNav = new LCRelationNavigator( pLCEvent->getCollection( m_inputJetSLDLink ) );
    } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : JetSLDNav collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    SLDNuNav = new LCRelationNavigator( pLCEvent->getCollection( m_inputSLDNuLink ) );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : SLDNuNav collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }
  try {
    NuMCNav = new LCRelationNavigator( pLCEvent->getCollection( m_recoNumcNuLinkName ) );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : NuMCNav collection not found in event " << m_nEvt << std::endl;
    m_pTTree->Fill();
    return;
  }

  m_nCorrectedSLD = inputSLDecayCollection->getNumberOfElements();
  m_nSLDecayBHadron = inputSLDecayCollection->getParameters().getIntVal( "nBHadronSLD_found" );
  m_nSLDecayCHadron = inputSLDecayCollection->getParameters().getIntVal( "nCHadronSLD_found" );
  m_nSLDecayTauLepton = inputSLDecayCollection->getParameters().getIntVal( "nTauLeptonSLD_found" );
  m_nSLDecayTotal = inputSLDecayCollection->getParameters().getIntVal( "nTotalSLD_found" );
  m_nJets = inputJetCollection->getNumberOfElements();
  m_nLeps = inputLeptonCollection->getNumberOfElements();
  streamlog_out(MESSAGE8) << "	Number of jets: " << m_nJets << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of leptons: " << m_nLeps << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of found semi-leptonic decays: " << m_nSLDecayTotal << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of corrected semi-leptonic decays: " << m_nCorrectedSLD << std::endl ;
  //Beamstrahlung
  ROOT::Math::PxPyPzEVector bsFourMomentum(0.,0.,0.,0.);
  for (int i_mc = 0; i_mc < 2; i_mc++){ //no. 0 and 1 are e+e- pre bs
    MCParticle* mcp = (MCParticle*) inputMCParticleCollection->getElementAt(i_mc);
    bsFourMomentum += ROOT::Math::PxPyPzEVector(mcp->getMomentum()[0],mcp->getMomentum()[1],mcp->getMomentum()[2], mcp->getEnergy());
  }
  for (int i_mc = 2; i_mc < 4; i_mc++){ //no. 2 and 3 are e+e- post bs
    MCParticle* mcp = (MCParticle*) inputMCParticleCollection->getElementAt(i_mc);
    bsFourMomentum -= ROOT::Math::PxPyPzEVector(mcp->getMomentum()[0],mcp->getMomentum()[1],mcp->getMomentum()[2], mcp->getEnergy());
  }
  m_BSEnergyTrue = bsFourMomentum.E();
  //ISR: saves largest value since fit can only handle one ISR photon
  MCParticle* ISR1 = (MCParticle*) inputMCParticleCollection->getElementAt(6);
  MCParticle* ISR2 = (MCParticle*) inputMCParticleCollection->getElementAt(7);
  if (ISR1->getPDG() == 22 && ISR2->getPDG() == 22) m_ISREnergyTrue = (ISR1->getEnergy() > ISR2->getEnergy()) ? ISR1->getEnergy() : ISR2->getEnergy();
  else streamlog_out(WARNING) << "Not photons:" << ISR1->getPDG() << " and " << ISR2->getPDG() << endl;
  streamlog_out(MESSAGE) << "ISR energies: " << ISR1->getEnergy() << ", " << ISR2->getEnergy() << endl;
  streamlog_out(MESSAGE) << "Picked ISR energy: " << m_ISREnergyTrue << endl;
 //W, t, tbar and ttbar masses in hard process
  MCParticle *t = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(8));
  MCParticle *tbar = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(9));
  std::vector<MCParticle*> mcW{};
  for (int i = 0; i < inputMCParticleCollection->getNumberOfElements(); ++i) {
    MCParticle *mcp = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(i));
    if (std::abs(mcp->getPDG()) == 24) mcW.push_back(mcp); //abs value to catch W+ and W-
  }
  if (mcW.size() == 2) {
    m_W1MassHardProcess = mcW.at(0)->getMass();
    m_W2MassHardProcess = mcW.at(1)->getMass();
    m_tMassHardProcess = t->getMass();
    m_tbarMassHardProcess = tbar->getMass();
    ROOT::Math::PxPyPzEVector ttbarFourMomentum(0.,0.,0.,0.);
    ttbarFourMomentum +=  ROOT::Math::PxPyPzEVector( t->getMomentum()[0], t->getMomentum()[1], t->getMomentum()[2] , t->getEnergy() );
    ttbarFourMomentum +=  ROOT::Math::PxPyPzEVector( tbar->getMomentum()[0], tbar->getMomentum()[1], tbar->getMomentum()[2] , tbar->getEnergy() );
    m_ttbarMassHardProcess = ttbarFourMomentum.M();
  }

  streamlog_out(MESSAGE) << "Hard process Top  mass: " << m_tMassHardProcess << endl;
  streamlog_out(MESSAGE) << "Hard process Top bar mass: " << m_tbarMassHardProcess << endl;
  streamlog_out(MESSAGE) << "Hard process W 1 mass: " << m_W1MassHardProcess << endl;
  streamlog_out(MESSAGE) << "Hard process W 2 mass: " << m_W2MassHardProcess << endl;
  streamlog_out(MESSAGE) << "Hard process ttbar mass: " << m_ttbarMassHardProcess << endl;
  
  else streamlog_out(WARNING) << "////////////////////////////////////////////////// MC W pair not found //////////////////////////////////////////////////" << endl;

  if (m_signature == "lvqqbb") {
    m_nAskedJets = 4;
    m_nAskedLeps = 1;
  } else if (m_signature == "tvqqbb") {
    m_nAskedJets = 5;
    m_nAskedLeps = 0;
  } else if (m_signature == "qqqqbb") {
    m_nAskedJets = 6;
    m_nAskedLeps = 0;
  }
  if ( m_nJets != m_nAskedJets || m_nLeps != m_nAskedLeps ) {
    m_pTTree->Fill();
    return;
  }
  //&& m_nCorrectedSLD == m_nSLDecayTotal )	
  bool traceEvent = false;
  if ( pLCEvent->getEventNumber() == m_ievttrace || m_traceall ) traceEvent = true;
  
  std::vector< ReconstructedParticle* > Leptons{};
  for (int i_lep = 0; i_lep < m_nLeps; i_lep++) {
    ReconstructedParticle* lepton = dynamic_cast<ReconstructedParticle*>( inputLeptonCollection->getElementAt( i_lep ) );
    Leptons.push_back(lepton);
  }
  
  std::vector<ReconstructedParticle*> Jets{};
  for (int i_jet = 0; i_jet < m_nJets; i_jet++) {
    ReconstructedParticle* jet = dynamic_cast<ReconstructedParticle*>( inputJetCollection->getElementAt( i_jet ) );
    Jets.push_back(jet);
    
    m_Sigma_Px2.push_back(jet->getCovMatrix()[0]);
    m_Sigma_PxPy.push_back(jet->getCovMatrix()[1]);
    m_Sigma_Py2.push_back(jet->getCovMatrix()[2]);
    m_Sigma_PxPz.push_back(jet->getCovMatrix()[3]);
    m_Sigma_PyPz.push_back(jet->getCovMatrix()[4]);
    m_Sigma_Pz2.push_back(jet->getCovMatrix()[5]);
    m_Sigma_PxE.push_back(jet->getCovMatrix()[6]);
    m_Sigma_PyE.push_back(jet->getCovMatrix()[7]);
    m_Sigma_PzE.push_back(jet->getCovMatrix()[8]);
    m_Sigma_E2.push_back(jet->getCovMatrix()[9]);

    if (m_Sigma_Px2.back() == 0 ||
        m_Sigma_Py2.back() == 0 ||
        m_Sigma_Pz2.back() == 0 ||
        m_Sigma_E2.back() == 0) {
      m_invalidCovMatrixAtJet = i_jet;
    }
  }

  if (m_invalidCovMatrixAtJet >= 0) {
    streamlog_out(MESSAGE) << "	Skipping event due to at least one zero diagonal element for jet " << m_invalidCovMatrixAtJet << std::endl ;
    m_pTTree->Fill();
    return;
  }

  vector<ReconstructedParticleImpl*> startjets;
  vector<ReconstructedParticleImpl*> startleptons;
  vector<ReconstructedParticleImpl*> fittedjets;
  vector<ReconstructedParticleImpl*> fittedleptons;

  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||  KINFIT WITHOUT NEUTRINO COORECTION  ||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;
  
  FitResult woNuFitResult;
  if (m_signature == "lvqqbb") {
    woNuFitResult = performlvqqbbFIT( Jets, Leptons, traceEvent , inputMCParticleCollection);
  }
  if (m_signature == "tvqqbb") {
    woNuFitResult = performtvqqbbFIT( Jets, traceEvent , inputMCParticleCollection);
  }
  if (m_signature == "qqqqbb") {
    woNuFitResult = performqqqqbbFIT( Jets, traceEvent, pLCEvent->getCollection("RefinedJets6") );
  }
  BaseFitter* woNuFitter = woNuFitResult.fitter.get();
  
  streamlog_out(MESSAGE) << "Performed fit without neutrino correction" << endl;
  if (!woNuFitResult.fitter) {
    streamlog_out(MESSAGE) << "Did not find a functioning fit" << endl;
  } else { //succesful fit
    //Fill root branches
    m_FitErrorCode = woNuFitResult.fitter->getError();
    m_FitProbability = woNuFitResult.fitter->getProbability();
    m_FitChi2 = woNuFitResult.fitter->getChi2();
    streamlog_out(MESSAGE) << "error code = " << woNuFitResult.fitter->getError() << endl;
    streamlog_out(MESSAGE) << "fit prob = " << woNuFitResult.fitter->getProbability() << endl;
    streamlog_out(MESSAGE) << "fit chi2 = " << woNuFitResult.fitter->getChi2()<< endl;
    streamlog_out(MESSAGE) << "Getting constraints now... ";
    auto constraints = woNuFitResult.constraints;
    streamlog_out(MESSAGE) << "Fitter contains " << constraints->size() << " constraints : ";
    for (auto it = constraints->begin(); it != constraints->end(); ++it) {
      streamlog_out(MESSAGE) << (*it)->getName() << " ";
      if (strcmp((*it)->getName(), "w1 mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_W1MassAfterFit = mc->getMass();
      }
      if (strcmp((*it)->getName(), "w2 mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_W2MassAfterFit = mc->getMass();
      }
      if (strcmp((*it)->getName(), "t mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_tMassAfterFit = mc->getMass();
      }
      if (strcmp((*it)->getName(), "tbar mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_tbarMassAfterFit = mc->getMass();
      }
      if (strcmp((*it)->getName(), "ttbar mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_ttbarMassAfterFit = mc->getMass();
      }
    }
    streamlog_out(MESSAGE) << endl;
    
    streamlog_out(MESSAGE) << "Getting fitobjects now... ";
    auto fitobjects = woNuFitResult.fitobjects;
    streamlog_out(MESSAGE) << "Fitter contains " << fitobjects->size() << " fitobjects : ";
    for (auto it = fitobjects->begin(); it != fitobjects->end(); ++it) {
      streamlog_out(MESSAGE) << (*it)->getName() << " ";
      m_perm.push_back((int)((string)(*it)->getName()).back()-48);
    }
    streamlog_out(MESSAGE) << endl;

    std::pair<vector<double>,vector<double>> startvalues = calculateInitialValues(Jets, Leptons, m_perm);
    vector<double> startmasses = startvalues.first;
    m_W1MassBeforeFit  = startmasses[0];
    m_W2MassBeforeFit = startmasses[1];
    m_tMassBeforeFit = startmasses[2];
    m_tbarMassBeforeFit = startmasses[3];
    m_ttbarMassBeforeFit = startmasses[4];

    m_pxcstartvalue = startvalues.second[0];
    m_pycstartvalue = startvalues.second[1];
    m_pzcstartvalue = startvalues.second[2];
    m_ecstartvalue = startvalues.second[3];
    
    streamlog_out(MESSAGE1) << "W1 mass prefit = " << m_W1MassBeforeFit << endl;
    streamlog_out(MESSAGE1) << "W2 mass prefit = " << m_W2MassBeforeFit << endl;
    streamlog_out(MESSAGE1) << "t mass prefit = " << m_tMassBeforeFit << endl;
    streamlog_out(MESSAGE1) << "tbar mass prefit = " << m_tbarMassBeforeFit << endl;
    streamlog_out(MESSAGE1) << "ttbar mass prefit = " << m_ttbarMassBeforeFit << endl;
    streamlog_out(MESSAGE) << "W1 mass postfit = " << m_W1MassAfterFit << endl;
    streamlog_out(MESSAGE) << "W2 mass postfit = " << m_W2MassAfterFit << endl;
    streamlog_out(MESSAGE) << "t mass postfit = " << m_tMassAfterFit << endl;
    streamlog_out(MESSAGE) << "tbar mass postfit = " << m_tbarMassAfterFit << endl;
    streamlog_out(MESSAGE) << "ttbar mass postfit = " << m_ttbarMassAfterFit << endl;

    string photonname = "photon";
    auto fitphoton = find_if(fitobjects->begin(), fitobjects->end(), [&photonname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == photonname;});
    if (fitphoton == fitobjects->end()) {
      streamlog_out(MESSAGE) << "Did not find photon" <<endl;
    }
    else {
      auto gamma = dynamic_pointer_cast<ISRPhotonFitObject>(*fitphoton);
      m_ISREnergyAfterFit = gamma->getE();
      streamlog_out(MESSAGE) << "ISR energy true    = " << m_ISREnergyTrue << endl;
      streamlog_out(MESSAGE) << "ISR energy postfit = " << m_ISREnergyAfterFit << endl;
    }

    for (int i = 0; i < m_nJets; ++i) {   
      //set pre fit objects
      ReconstructedParticleImpl* startjet = new ReconstructedParticleImpl;
      ROOT::Math::PxPyPzEVector startFourMomentum(Jets[i]->getMomentum()[0], Jets[i]->getMomentum()[1], Jets[i]->getMomentum()[2], Jets[i]->getEnergy());
      m_PrefitJetFourMomentum.push_back(startFourMomentum);
      startjet->setMomentum(Jets[i]->getMomentum());
      startjet->setEnergy(Jets[i]->getEnergy());
      startjet->setType(Jets[i]->getType());
      startjet->setCharge(Jets[i]->getCharge());
      startjet->setMass(Jets[i]->getMass());
      startjet->setCovMatrix(Jets[i]->getCovMatrix());
      //outputStartJetCollection->addElement(startjet);
      startjets.push_back(startjet);
      //set post fit objects      
      string fitname = "jet"+to_string(i);
      auto fitjet = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitjet == fitobjects->end()) {
	streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
	continue;
      }
      std::shared_ptr<JetFitObject> castfitjet =  std::dynamic_pointer_cast<JetFitObject>(*fitjet);
      ReconstructedParticleImpl* fittedjet = new ReconstructedParticleImpl;
      ROOT::Math::PxPyPzEVector FourMomentum(castfitjet->getPx(), castfitjet->getPy(), castfitjet->getPz(), castfitjet->getE());
      m_PostfitJetFourMomentum.push_back(FourMomentum);
      float momentum[3] = {
        (float)castfitjet->getPx(),
        (float)castfitjet->getPy(),
        (float)castfitjet->getPz()};
      fittedjet->setMomentum(momentum);
      fittedjet->setEnergy(FourMomentum.E());
      fittedjet->setMass(FourMomentum.M());
      //outputJetCollection->addElement(fittedjet);
      fittedjets.push_back(fittedjet);
      //calculate pulls
      vector<double> pulls = calculatePulls(castfitjet, Jets[i], 1);
      streamlog_out(MESSAGE) << "pulls jet" << i << ": " << pulls[0] << ", " << pulls[1] << ", " << pulls[2] << endl;
      m_pullJetEnergy.push_back(pulls[0]);
      m_pullJetTheta.push_back(pulls[1]);
      m_pullJetPhi.push_back(pulls[2]);
    }
    for (int i = 0; i < m_nLeps; ++i) {
      //set pre fit objects
      ReconstructedParticleImpl* startlepton = new ReconstructedParticleImpl;
      startlepton->setMomentum(Leptons[i]->getMomentum());
      startlepton->setEnergy(Leptons[i]->getEnergy());
      startlepton->setType(Leptons[i]->getType());
      startlepton->setCharge(Leptons[i]->getCharge());
      startlepton->setMass(Leptons[i]->getMass());
      startlepton->setCovMatrix(Leptons[i]->getCovMatrix());
      //outputStartLeptonCollection->addElement(startlepton);
      startleptons.push_back(startlepton);
      //set post fit objects
      string fitname = "lepton"+to_string(i);
      auto fitlepton = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitlepton == fitobjects->end()) {
	streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
	continue;
      }
      std::shared_ptr<LeptonFitObject> castfitlepton =  std::dynamic_pointer_cast<LeptonFitObject>(*fitlepton);
      ReconstructedParticleImpl* fittedlepton = new ReconstructedParticleImpl;
      ROOT::Math::PxPyPzEVector FourMomentum(castfitlepton->getPx(), castfitlepton->getPy(), castfitlepton->getPz(), castfitlepton->getE());
      float momentum[3] = {
        (float)castfitlepton->getPx(),
        (float)castfitlepton->getPy(),
        (float)castfitlepton->getPz()};
      fittedlepton->setMomentum(momentum);
      fittedlepton->setEnergy(FourMomentum.E());
      fittedlepton->setMass(FourMomentum.M());
      //outputLeptonCollection->addElement(fittedlepton);
      fittedleptons.push_back(fittedlepton);
      //calculate pulls
      vector<double> pulls = calculatePulls(castfitlepton, Leptons[i], 2);
      m_pullLeptonInvPt.push_back(pulls[0]);
      m_pullLeptonTheta.push_back(pulls[1]);
      m_pullLeptonPhi.push_back(pulls[2]);
    }
    if (m_signature == "tvqqbb" || m_signature == "lvqqbb"){
      //calculate 4-momentum of W->invisible, the primary neutrino
      // correct for crossing angle 
      float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad 
      double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);
      ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
      ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
      // accounting for visible jets
      for (int i_jet = 0; i_jet < m_nJets; i_jet++) {
	    seenFourMomentum += ROOT::Math::PxPyPzEVector(Jets[ i_jet ]->getMomentum()[0],Jets[ i_jet ]->getMomentum()[1],Jets[ i_jet ]->getMomentum()[2], Jets[ i_jet ]->getEnergy());
      }
      //accounting for leptons
      for (int i_lep = 0; i_lep < m_nLeps; i_lep++) {
      seenFourMomentum += ROOT::Math::PxPyPzEVector(Leptons[ i_lep ]->getMomentum()[0],Leptons[ i_lep ]->getMomentum()[1],Leptons[ i_lep ]->getMomentum()[2], Leptons[ i_lep ]->getEnergy());
      }
      ROOT::Math::PxPyPzEVector missingFourMomentum = ecms-seenFourMomentum;
      double startNu[3] = {missingFourMomentum.Px(), missingFourMomentum.Py(), missingFourMomentum.Pz()};
      float errorsNu[3] = {m_NeutrinoPxPyError, m_NeutrinoPxPyError, m_NeutrinoPzError};
      //get neutrino fit object
      string fitname = "Neutrino";
      auto fitNu = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitNu == fitobjects->end()) {
	    streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
      } else {
	std::shared_ptr<NeutrinoFitObjectNew> castfitNu =  std::dynamic_pointer_cast<NeutrinoFitObjectNew>(*fitNu);
	//calculate pulls
	vector<double> pulls;
	for (int ipar = 0; ipar < 3; ipar++) {
	  double fitted = castfitNu->getParam(ipar);
	  double start = startNu[ipar];
	  double errfit = castfitNu->getError(ipar);
	  double errmea = errorsNu[ipar];
	  double sigma = errmea*errmea-errfit*errfit;
	  //streamlog_out(MESSAGE) << "fitted = " << fitted << ", start = " << start << std::endl ;
	  //streamlog_out(MESSAGE) << "errfit = " << errfit << ", errmea = " << errmea << ", sigma = " << sigma << std::endl ;
	  if (sigma > 0) {
	    sigma = sqrt(sigma);
	    streamlog_out(MESSAGE) << "pull = " << (fitted - start)/sigma << std::endl ;
	    pulls.push_back((fitted - start)/sigma);
	  } else {
	    streamlog_out(DEBUG3) << "NOT GOOD...................." << std::endl ;
	    pulls.push_back(-999.);
	  }	  
	}
	m_pullInvPx.push_back(pulls[0]);
	m_pullInvPy.push_back(pulls[1]);
	m_pullInvPz.push_back(pulls[2]);
      }
    } //if lvqqbb or tvqqbb --> calculate Nu pulls
  }

  //add start and fitted leptons and jets to output collections
  for (auto startjet: startjets) outputStartJetCollection->addElement(startjet);
  for (auto startlepton: startleptons) outputStartLeptonCollection->addElement(startlepton);
  for (auto fittedjet: fittedjets) outputJetCollection->addElement(fittedjet);
  for (auto fittedlepton: fittedleptons) outputLeptonCollection->addElement(fittedlepton);

  //add permutations as parameter to both jet collections
  //std::vector<int> jetperm= {m_perm.begin(),m_perm.begin()+m_nJets};
  //std::vector<int> jetperm;
  //for (unsigned int i = 0; i < m_nJets; i++) streamlog_out(MESSAGE1) << m_perm[i] << endl;
  //for (unsigned int i = 0; i < m_nJets; i++) jetperm.push_back(m_perm[i]);
  //for (unsigned int i = 0; i < m_nJets; i++) streamlog_out(MESSAGE1) << jetperm[i] << endl;
  //m_perm.erase(m_perm.begin()+m_nJets,m_perm.end());
  outputStartJetCollection->parameters().setValues("permutation", m_perm);
  outputJetCollection->parameters().setValues("permutation", m_perm);
  outputStartJetCollection->parameters().setValue("fitprob", m_FitProbability);
  outputJetCollection->parameters().setValue("fitprob", m_FitProbability);
  outputStartJetCollection->parameters().setValue("fitchi2", m_FitChi2);
  outputJetCollection->parameters().setValue("fitchi2", m_FitChi2);
  
  pLCEvent->addCollection( outputJetCollection , m_outputJetCollection.c_str() );
  streamlog_out(DEBUG0) << " Output Jet collection added to event" << std::endl;
  pLCEvent->addCollection( outputLeptonCollection , m_outputLeptonCollection.c_str() );
  streamlog_out(DEBUG0) << " Output Lepton collection added to event" << std::endl;
  pLCEvent->addCollection( outputStartJetCollection , m_outputStartJetCollection.c_str() );
  streamlog_out(DEBUG0) << " Output Start Jet collection added to event" << std::endl;
  pLCEvent->addCollection( outputStartLeptonCollection , m_outputStartLeptonCollection.c_str() );
  streamlog_out(DEBUG0) << " Output Start Lepton collection added to event" << std::endl;
  //pLCEvent->addCollection( outputNuEnergyCollection, m_outputNuEnergyCollection.c_str() );
  //streamlog_out(DEBUG0) << " Output true and reco Nu collection added to event" << std::endl;
  m_pTTree->Fill();
}

ttbarKinFit::FitResult ttbarKinFit::performqqqqbbFIT( pfoVector jets, bool traceEvent, LCCollection *originalJetCollection) {
  shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////												  //////
  //////					Set JetFitObjects					  //////
  //////												  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
    streamlog_out(MESSAGE6) << "get jet"<< i_jet+1 <<" parameters"  << std::endl ; //changed from debug level
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getJetParameters( jets[ i_jet ] , parameters , errors );
    auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
    jfo->push_back(j);
    const string name = "jet"+to_string(i_jet);
    j->setName(name.c_str());
    //streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << *j  << std::endl ;
    streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << "[" << jets[ i_jet ]->getMomentum()[0] << ", " << jets[ i_jet ]->getMomentum()[1] << ", " << jets[ i_jet ]->getMomentum()[2] << ", " << jets[ i_jet ]->getEnergy() << "]" << std::endl ;
  }

  PIDHandler jetPIDh(originalJetCollection);
	int _FTAlgoID = jetPIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm);

	int BTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
	int BbarTagID = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterBbar);
  
  m_bTagValues.assign(jets.size(), -1.0);
  m_scoreB.assign(jets.size(), 0.0);
  m_scoreBbar.assign(jets.size(), 0.0);
  m_jetTags.assign(jets.size(), std::vector<float>(m_JetTaggingPIDParameters.size(), 0.0));

	//int OTagID = jetPIDh.getParameterIndex(_FTAlgoID, "OTag");
	streamlog_out(MESSAGE) << "--- FLAVOR TAG ---" << std::endl;

	for (unsigned int i_jet = 0; i_jet < jets.size(); i_jet++) {
      ReconstructedParticle* origJet = dynamic_cast<ReconstructedParticle*>(originalJetCollection->getElementAt(i_jet));
      const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(jetPIDh.getParticleID(origJet, _FTAlgoID));
      const FloatVec& FTPara = FTImpl.getParameters();

			m_bTagValues[i_jet] = FTPara[BTagID] + FTPara[BbarTagID];
			streamlog_out(MESSAGE) << "Jet " << i_jet << std::endl << " > Algo1 - B [B,Bbar] = " << m_bTagValues[i_jet] << " [" << FTPara[BTagID] << ", " << FTPara[BbarTagID] << "]" << std::endl;

      m_scoreB[i_jet] = FTPara[BTagID];
      m_scoreBbar[i_jet] = FTPara[BbarTagID];

			// write all requested parameters
			for (size_t j = 0; j < m_JetTaggingPIDParameters.size(); j++) {
					int param_id = jetPIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameters[j]);
					if (param_id + 1 > (int)FTPara.size()) {
						std::cerr << "Parameter error: Param " << j << ", value=" << m_JetTaggingPIDParameters[j] << std::endl;
						throw EVENT::Exception("No flavor tagging value for parameter");
					}
					
					m_jetTags[i_jet][j] = FTPara[param_id];
				}
      }

  // calculate bmax1,2,3,4
	for (size_t i = 0; i < m_bTagValues.size(); i++)
			m_bTagsSorted.push_back(std::make_pair(i, m_bTagValues[i]));

	std::sort (m_bTagsSorted.begin(), m_bTagsSorted.end(), jetTaggingComparator);

	TVector3 pjbmaxA1 (jets[m_bTagsSorted[0].first]->getMomentum());
	TVector3 pjbmaxA2 (jets[m_bTagsSorted[1].first]->getMomentum());
	m_cosbmax = pjbmaxA1.Dot(pjbmaxA2)/pjbmaxA1.Mag()/pjbmaxA2.Mag();

	m_bmax1 = m_bTagsSorted[0].second;
	m_bmax2 = m_bTagsSorted[1].second;
	m_bmax3 = m_bTagsSorted[2].second;
	m_bmax4 = m_bTagsSorted[3].second;
	m_bmax5 = m_bTagsSorted[4].second;
	m_bmax6 = m_bTagsSorted[5].second;

  double bestProb = -1;
  double bestChi2 = 9999999999999.;
  FitResult bestFitResult;

  assert(jets.size()==6);
  vector<vector<unsigned int>> perms = {
      {0, 1, 2, 3, 4, 5}, {1, 2, 0, 3, 4, 5}, {2, 4, 0, 1, 3, 5},
      {0, 1, 2, 4, 3, 5}, {1, 2, 0, 4, 3, 5}, {2, 4, 0, 3, 1, 5},
      {0, 1, 2, 5, 3, 4}, {1, 2, 0, 5, 3, 4}, {2, 4, 0, 5, 1, 3},
      {0, 2, 1, 3, 4, 5}, {1, 3, 0, 2, 4, 5}, {2, 5, 0, 1, 3, 4},
      {0, 2, 1, 4, 3, 5}, {1, 3, 0, 4, 2, 5}, {2, 5, 0, 3, 1, 4},
      {0, 2, 1, 5, 3, 4}, {1, 3, 0, 5, 2, 4}, {2, 5, 0, 4, 1, 3},
      {0, 3, 1, 2, 4, 5}, {1, 4, 0, 2, 3, 5}, {3, 4, 0, 1, 2, 5},
      {0, 3, 1, 4, 2, 5}, {1, 4, 0, 3, 2, 5}, {3, 4, 0, 2, 1, 5},
      {0, 3, 1, 5, 2, 4}, {1, 4, 0, 5, 2, 3}, {3, 4, 0, 5, 1, 2},
      {0, 4, 1, 2, 3, 5}, {1, 5, 0, 2, 3, 4}, {3, 5, 0, 1, 2, 4},
      {0, 4, 1, 3, 2, 5}, {1, 5, 0, 3, 2, 4}, {3, 5, 0, 2, 1, 4},
      {0, 4, 1, 5, 2, 3}, {1, 5, 0, 4, 2, 3}, {3, 5, 0, 4, 1, 2},
      {0, 5, 1, 2, 3, 4}, {2, 3, 0, 1, 4, 5}, {4, 5, 0, 1, 2, 3},
      {0, 5, 1, 3, 2, 4}, {2, 3, 0, 4, 1, 5}, {4, 5, 0, 2, 1, 3},
      {0, 5, 1, 4, 2, 3}, {2, 3, 0, 5, 1, 4}, {4, 5, 0, 3, 1, 2},
    };

  for (unsigned int iperm = 0; iperm < perms.size(); iperm++) {
    streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
    streamlog_out(MESSAGE) << " iperm = " << iperm << std::endl ;

    shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();

    // important: (re-)set fitjets array!                                                                                                       // keep track of newly created heap particles
    shared_ptr<vector<shared_ptr<BaseFitObject>>> fos = make_shared<vector<shared_ptr<BaseFitObject>>>();
      streamlog_out(MESSAGE) << " Picking jets ";
    for(auto i : perms[iperm]) {
      streamlog_out(MESSAGE) << i << " ";
      auto jsp = make_shared<JetFitObject>(*jfo->at(i));
      jfo_perm->push_back(jsp);
    }
    for(auto j : *jfo_perm) fos->push_back(j);
    for (auto j : *jfo_perm) 
      streamlog_out(MESSAGE)  << "start four-vector of jet " << j->getName() << ": " << *j  << std::endl ;  //changed from debug level 
    //for (int i = 0; i < NJETS; ++i) streamlog_out(DEBUG)  << "original four-vector of jet " << i << ": " << fitjets[i]  << std::endl ;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////												    //////
    //////					Set Constraints Before Fit				    //////
    //////												    //////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
    shared_ptr<MomentumConstraint> pxc = make_shared<MomentumConstraint>( 0 , 1 , 0 , 0 , target_p_due_crossing_angle);//Factor for: (energy sum, px sum, py sum,pz sum,target value of sum)    
    pxc->setName("sum(p_x)");
    for (auto j : *jfo_perm) pxc->addToFOList(*j);
    
    shared_ptr<MomentumConstraint> pyc = make_shared<MomentumConstraint>(0, 0, 1, 0, 0);
    pyc->setName("sum(p_y)");
    for (auto j : *jfo_perm) pyc->addToFOList(*j);
    
    shared_ptr<MomentumConstraint> pzc = make_shared<MomentumConstraint>(0, 0, 0, 1, 0);
    pzc->setName("sum(p_z)");
    for (auto j : *jfo_perm) pzc->addToFOList(*j);

    double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.); //TODO: check equation
    shared_ptr<MomentumConstraint> ec = make_shared<MomentumConstraint>(1, 0, 0, 0, E_lab);
    ec->setName("sum(E)");
    for (auto j : *jfo_perm) ec->addToFOList(*j);
    
    streamlog_out(MESSAGE8)  << "	Value of E_lab before adding ISR: " << E_lab << std::endl ;  //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before adding ISR: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pxc before adding ISR: " << pxc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pyc before adding ISR: " << pyc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pzc before adding ISR: " << pzc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of ec before adding ISR: " << ec->getValue() << std::endl ; //changed from debug level 
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////													//////
    //////					Set ISR PhotonFitObjects					//////
    //////													//////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    shared_ptr<ISRPhotonFitObject> photon = make_shared<ISRPhotonFitObject>(0., 0., -pzc->getValue(), b, ISRPzMaxB);
    photon->setName("photon");
    if( m_fitISR ) {
      streamlog_out(MESSAGE)  << "start four-vector of ISR photon: " << *(photon) << std::endl ; //changed from debug level 
      fos->push_back(photon);
      pxc->addToFOList(*(photon));
      pyc->addToFOList(*(photon));
      pzc->addToFOList(*(photon));
      ec->addToFOList(*(photon));
    }
    streamlog_out(MESSAGE8)  << "	Value of E_lab before fit: " << E_lab << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before fit: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pxc after adding ISR before fit: " << pxc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pyc after adding ISR before fit: " << pyc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pzc after adding ISR before fit: " << pzc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of ec after adding ISR before fit: " << ec->getValue() << std::endl ; //changed from debug level 
      
    //To be added to fit, depending on fit hypothesis
    shared_ptr<MassConstraint> w1m = make_shared<MassConstraint>(80.398);
    w1m->addToFOList (*jfo_perm->at(0), 1);
    w1m->addToFOList (*jfo_perm->at(1), 1);
    w1m->setName("w1 mass");
    //shared_ptr<SoftGaussMassConstraint> h2m = make_shared<SoftGaussMassConstraint>(125.,3.); 
    shared_ptr<MassConstraint> w2m = make_shared<MassConstraint>(80.398);
    w2m->addToFOList (*jfo_perm->at(2), 1);
    w2m->addToFOList (*jfo_perm->at(3), 1);
    w2m->setName("w2 mass");
    shared_ptr<MassConstraint> tm = make_shared<MassConstraint>(172.52);
    tm->addToFOList (*jfo_perm->at(0), 1);
    tm->addToFOList (*jfo_perm->at(1), 1);
    tm->addToFOList (*jfo_perm->at(4), 1);
    tm->setName("t mass");
    shared_ptr<MassConstraint> tbarm = make_shared<MassConstraint>(172.52);
    tbarm->addToFOList (*jfo_perm->at(2), 1);
    tbarm->addToFOList (*jfo_perm->at(3), 1);
    tbarm->addToFOList (*jfo_perm->at(5), 1);
    tbarm->setName("tbar mass");
    //Not part of fit hypothesis, added after fit:
    shared_ptr<MassConstraint> w1 = make_shared<MassConstraint>(80.398);
    w1->addToFOList (*jfo_perm->at(0), 1);
    w1->addToFOList (*jfo_perm->at(1), 1);
    w1->setName("w1 fitted mass");
    //shared_ptr<SoftGaussMassConstraint> h2m = make_shared<SoftGaussMassConstraint>(125.,3.); 
    shared_ptr<MassConstraint> w2 = make_shared<MassConstraint>(80.398);
    w2->addToFOList (*jfo_perm->at(2), 1);
    w2->addToFOList (*jfo_perm->at(3), 1);
    w2->setName("w2 fitted mass");
    shared_ptr<MassConstraint> t = make_shared<MassConstraint>(172.52);
    t->addToFOList (*jfo_perm->at(0), 1);
    t->addToFOList (*jfo_perm->at(1), 1);
    t->addToFOList (*jfo_perm->at(4), 1);
    t->setName("t fitted mass");
    shared_ptr<MassConstraint> tbar = make_shared<MassConstraint>(172.52);
    tbar->addToFOList (*jfo_perm->at(2), 1);
    tbar->addToFOList (*jfo_perm->at(3), 1);
    tbar->addToFOList (*jfo_perm->at(5), 1);
    tbar->setName("tbar fitted mass");
    shared_ptr<MassConstraint> ttbar = make_shared<MassConstraint>(345.);
    ttbar->addToFOList(*jfo_perm->at(0), 1);
    ttbar->addToFOList(*jfo_perm->at(1), 1);
    ttbar->addToFOList (*jfo_perm->at(2), 1);
    ttbar->addToFOList (*jfo_perm->at(3), 1);
    ttbar->addToFOList (*jfo_perm->at(4), 1);
    ttbar->addToFOList (*jfo_perm->at(5), 1);
    ttbar->setName("ttbar fitted mass");
      
    streamlog_out(MESSAGE) << "start mass of W1  : " << w1->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of W2: " << w2->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of t: " << t->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of tbar: " << tbar->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of ttbar: " << ttbar->getMass(1) << std::endl ; //changed from debug level 
    
    shared_ptr<BaseFitter> fitter = NULL;
    if ( m_fitter == 1 ) {
      fitter = shared_ptr<BaseFitter>(new NewFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewFitterGSL>(fitter))->setDebug( 4 );    
      streamlog_out(MESSAGE6) << "fit using GSL Fitter"  << std::endl ; //changed from debug level 
    } else if ( m_fitter == 2 ) {
      fitter = shared_ptr<BaseFitter>(new NewtonFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewtonFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(MESSAGE6) << "fit using Newton GSL Fitter"  << std::endl ; //changed from debug level 
    } else {
      ////		OPALFitter has no method setDebug !
      fitter = shared_ptr<BaseFitter>(new OPALFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<OPALFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(MESSAGE6) << "fit using OPAL GSL Fitter"  << std::endl ; //changed from debug level 
    }

    for(auto j : *jfo_perm) fitter->addFitObject(*j);

    if( m_fitISR ) {
      fitter->addFitObject( *(photon) );
      streamlog_out(MESSAGE8) << "ISR added to fit"  << std::endl ; //changed from debug level 
    }

    fitter->addConstraint( pxc.get() );
    fitter->addConstraint( pyc.get() );
    fitter->addConstraint( pzc.get() );
    fitter->addConstraint( ec.get() );

    fitter->addConstraint( w1m.get() );
    fitter->addConstraint( w2m.get() );
    fitter->addConstraint( tm.get() );
    fitter->addConstraint( tbarm.get() );

    streamlog_out(MESSAGE8) << "constraints added to fit:" << std::endl ; //changed from debug level 
    auto fitconstraints = fitter->getConstraints();
    auto fitsoftconstraints = fitter->getSoftConstraints();
    for (auto it = fitconstraints->begin(); it != fitconstraints->end(); ++it) {
      streamlog_out(MESSAGE8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl; //changed from debug level 
    }
    for (auto it = fitsoftconstraints->begin(); it != fitsoftconstraints->end(); ++it) {
      streamlog_out(MESSAGE8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl; //changed from debug level 
    }

    //perform fit: 
    //streamlog_out(MESSAGE) << "chi2 before fit" << calcChi2(fos) << endl; 
    float fitProbability = fitter->fit();
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from calcchi2)" << (dynamic_pointer_cast<NewFitterGSL>(fitter))->calcChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitobjects)" << calcChi2(fos) << endl; 
    fitter->addConstraint( w1.get() );
    fitter->addConstraint( w2.get() );
    fitter->addConstraint( t.get() );
    fitter->addConstraint( tbar.get() );
    fitter->addConstraint( ttbar.get() );
    //streamlog_out(MESSAGE) << "chi2 after adding helper constraints" << fitter->getChi2() << endl; 
    shared_ptr<vector<shared_ptr<BaseHardConstraint>>> constraints = make_shared<vector<shared_ptr<BaseHardConstraint>>>();
    constraints->push_back(t);
    constraints->push_back(pxc);
    constraints->push_back(pyc);
    constraints->push_back(pzc);
    constraints->push_back(ec);
    constraints->push_back(w1);
    constraints->push_back(w2);
    constraints->push_back(tbar);
    constraints->push_back(ttbar);
    //constraints->push_back(h);

    streamlog_out(MESSAGE8) << "helper constraints added"  << std::endl ; //changed from debug level 

    //---------------------------
    if(fitter->getError()==0) {
      if (fitter->getChi2() < bestChi2) {
      //if(fitProbability > bestProb) {
	streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " is better than " << bestProb << " use that one" << endl;
	streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
	streamlog_out(MESSAGE) << "error code: " << fitter->getError() << endl;
	bestProb = fitProbability;
	bestChi2 = fitter->getChi2();

	if( m_fitISR ) {
	  streamlog_out(MESSAGE)  << "After fit four-vector of ISR photon: " << *(photon) << std::endl ; //changed from debug level
	  streamlog_out(MESSAGE)  << "After fit ISR energy" << photon->getE() << endl;
	}
	
	FitResult fitresult(fitter, constraints, fos);
	/*for(auto it = fitresult.constraints->begin(); it != fitresult.constraints->end(); it++) {
	  streamlog_out(MESSAGE) << "   testing " << (*it)->getName() << endl;
	  if (strcmp((*it)->getName(), "h1 mass")==0) {
	    auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	    streamlog_out(MESSAGE)<< "   higgs mass constraint: " << mc->getMass() << endl;
	  }
	  }*/
	bestFitResult = fitresult;
      } else {
	streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " not better than " << bestProb << endl;
	streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
	streamlog_out(MESSAGE) << "error code: " << fitter->getError() << endl;
      }
    } else {
      streamlog_out(MESSAGE) << "fit failed with error code " << fitter->getError() << endl;
      streamlog_out(MESSAGE) << "fit probability: " << fitProbability << endl;
      streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
    }
  }
  streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
  streamlog_out(MESSAGE) << "Converged on best fit with probability " << bestProb << endl;
  streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
  return bestFitResult;
}



ttbarKinFit::FitResult ttbarKinFit::performlvqqbbFIT( pfoVector jets, pfoVector leptons, bool traceEvent, LCCollection *inputMCParticleCollection) {
  shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
  shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo= make_shared<vector<shared_ptr<LeptonFitObject>>>();
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////												  //////
  //////					Set JetFitObjects					  //////
  //////												  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
    streamlog_out(MESSAGE6) << "get jet"<< i_jet+1 <<" parameters"  << std::endl ; //changed from debug level
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getJetParameters( jets[ i_jet ] , parameters , errors );
    auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
    jfo->push_back(j);
    const string name = "jet"+to_string(i_jet);
    j->setName(name.c_str());
    //streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << *j  << std::endl ;
    streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << "[" << jets[ i_jet ]->getMomentum()[0] << ", " << jets[ i_jet ]->getMomentum()[1] << ", " << jets[ i_jet ]->getMomentum()[2] << ", " << jets[ i_jet ]->getEnergy() << "]" << std::endl ;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////							         				  //////
  //////					Set LeptonFitObjects					  //////
  //////												  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for (unsigned int i_lep =0; i_lep < leptons.size(); i_lep++) {
    streamlog_out(MESSAGE6) << "get lepton"<< i_lep+1 <<" parameters"  << std::endl ; //changed from debug level 
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getLeptonParameters( leptons[ i_lep ] , parameters , errors );
    auto l = make_shared<LeptonFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , leptons[ i_lep ]->getMass() );
    lfo->push_back(l);
    const string name = "lepton"+to_string(i_lep);
    l->setName(name.c_str());
    //streamlog_out(MESSAGE)  << " start four-vector of lepton"<< i_lep+1 <<": " << *l  << std::endl ;
    streamlog_out(MESSAGE)  << " start four-vector of lepton"<< i_lep+1 <<": " << "[" << leptons[ i_lep ]->getMomentum()[0] << ", " << leptons[ i_lep ]->getMomentum()[1] << ", " << leptons[ i_lep ]->getMomentum()[2] << ", " << leptons[ i_lep ]->getEnergy() << "]"  << std::endl ;
  }
  
  const int NLEPTONS = 1;
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////                                                                                                  //////
  //////                                        Set NeutrinoFitObject                                     //////
  //////                                                                                                  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //calculate 4-momentum of W->invisible
  // correct for crossing angle
  float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
  double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);
  ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
  ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
  for (int i_jet = 0; i_jet < m_nJets; i_jet++) {
    seenFourMomentum += ROOT::Math::PxPyPzEVector(jets[ i_jet ]->getMomentum()[0],jets[ i_jet ]->getMomentum()[1],jets[ i_jet ]->getMomentum()[2], jets[ i_jet ]->getEnergy());
  }
  //accounting for leptons
  for (int i_lep = 0; i_lep < m_nLeps; i_lep++) {
    seenFourMomentum += ROOT::Math::PxPyPzEVector(leptons[ i_lep ]->getMomentum()[0],leptons[ i_lep ]->getMomentum()[1],leptons[ i_lep ]->getMomentum()[2], leptons[ i_lep ]->getEnergy());
  }
  ROOT::Math::PxPyPzEVector missingFourMomentum = ecms-seenFourMomentum;
 
  shared_ptr<NeutrinoFitObjectNew> nfo = make_shared<NeutrinoFitObjectNew> (missingFourMomentum.Px(), missingFourMomentum.Py(), missingFourMomentum.Pz(), m_NeutrinoPxPyError, m_NeutrinoPxPyError, m_NeutrinoPzError);
  nfo->setName("Neutrino");
  streamlog_out(MESSAGE)  << "==== Neutrino errors ==== " << m_NeutrinoPxPyError << ", " << m_NeutrinoPzError << endl;
  
  const int NNEUTRINOS = 1;

  double bestProb = -1;
  double bestChi2 = 9999999999999.;
  FitResult bestFitResult;

  assert(jets.size()==4);
  // Indices 1 & 2 -> hadronic W, Index 3 -> top b jet & Index 4 -> topbar b jets
  vector<vector<unsigned int>> perms = {
      {0, 1, 2, 3}, {0, 1, 3, 2},
      {0, 2, 1, 3}, {0, 2, 3, 1},
      {0, 3, 1, 2}, {0, 3, 2, 1},
      {1, 2, 0, 3}, {1, 2, 3, 0},
      {1, 3, 0, 2}, {1, 3, 2, 0},
      {2, 3, 0, 1}, {2, 3, 1, 0}
    };

  for (unsigned int iperm = 0; iperm < perms.size(); iperm++) {
    streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
    streamlog_out(MESSAGE) << " iperm = " << iperm << std::endl ;

    shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();
    shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo_perm = make_shared<vector<shared_ptr<LeptonFitObject>>>();
    shared_ptr<vector<shared_ptr<NeutrinoFitObjectNew>>> nfo_perm = make_shared<vector<shared_ptr<NeutrinoFitObjectNew>>>();

    // important: (re-)set fitjets array!                                                                                                       // keep track of newly created heap particles
    shared_ptr<vector<shared_ptr<BaseFitObject>>> fos = make_shared<vector<shared_ptr<BaseFitObject>>>();
      streamlog_out(MESSAGE) << " Picking jets ";
    for(auto i : perms[iperm]) {
      streamlog_out(MESSAGE) << i << " ";
      auto jsp = make_shared<JetFitObject>(*jfo->at(i));
      jfo_perm->push_back(jsp);
    }
    streamlog_out(MESSAGE) << std::endl ;
    for(int i = 0; i < NLEPTONS; ++i) {
      auto lsp = make_shared<LeptonFitObject>(*lfo->at(i));
      lfo_perm->push_back(lsp);
    }
    for(int i = 0; i < NNEUTRINOS; ++i) {
      auto nsp = make_shared<NeutrinoFitObjectNew>(*nfo);
      nfo_perm->push_back(nsp);
    }

    for(auto j : *jfo_perm) fos->push_back(j);
    for(auto l : *lfo_perm) fos->push_back(l);
    for(auto n : *nfo_perm) fos->push_back(n);

    for (auto j : *jfo_perm) 
      streamlog_out(MESSAGE)  << "start four-vector of jet " << j->getName() << ": " << *j  << std::endl ;  //changed from debug level 
    //for (int i = 0; i < NJETS; ++i) streamlog_out(DEBUG)  << "original four-vector of jet " << i << ": " << fitjets[i]  << std::endl ;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////												    //////
    //////					Set Constraints Before Fit				    //////
    //////												    //////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
    shared_ptr<MomentumConstraint> pxc = make_shared<MomentumConstraint>( 0 , 1 , 0 , 0 , target_p_due_crossing_angle);//Factor for: (energy sum, px sum, py sum,pz sum,target value of sum)    
    pxc->setName("sum(p_x)");
    for (auto j : *jfo_perm) pxc->addToFOList(*j);
    for (auto l : *lfo_perm) pxc->addToFOList(*l);
    for (auto n : *nfo_perm) pxc->addToFOList(*n);
    
    shared_ptr<MomentumConstraint> pyc = make_shared<MomentumConstraint>(0, 0, 1, 0, 0);
    pyc->setName("sum(p_y)");
    for (auto j : *jfo_perm) pyc->addToFOList(*j);
    for (auto l : *lfo_perm) pyc->addToFOList(*l);
    for (auto n : *nfo_perm) pyc->addToFOList(*n);
    
    shared_ptr<MomentumConstraint> pzc = make_shared<MomentumConstraint>(0, 0, 0, 1, 0);
    pzc->setName("sum(p_z)");
    for (auto j : *jfo_perm) pzc->addToFOList(*j);
    for (auto l : *lfo_perm) pzc->addToFOList(*l);
    for (auto n : *nfo_perm) pzc->addToFOList(*n);
    
    double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.); //TODO: check equation
    shared_ptr<MomentumConstraint> ec = make_shared<MomentumConstraint>(1, 0, 0, 0, E_lab);
    ec->setName("sum(E)");
    for (auto j : *jfo_perm) ec->addToFOList(*j);
    for (auto l : *lfo_perm) ec->addToFOList(*l);
    for (auto n : *nfo_perm) ec->addToFOList(*n);

    
    streamlog_out(MESSAGE8)  << "	Value of E_lab before adding ISR: " << E_lab << std::endl ;  //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before adding ISR: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pxc before adding ISR: " << pxc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pyc before adding ISR: " << pyc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pzc before adding ISR: " << pzc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of ec before adding ISR: " << ec->getValue() << std::endl ; //changed from debug level 
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////													//////
    //////					Set ISR PhotonFitObjects					//////
    //////													//////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    shared_ptr<ISRPhotonFitObject> photon = make_shared<ISRPhotonFitObject>(0., 0., -pzc->getValue(), b, ISRPzMaxB);
    photon->setName("photon");
    if( m_fitISR ) {
      streamlog_out(MESSAGE)  << "start four-vector of ISR photon: " << *(photon) << std::endl ; //changed from debug level 
      fos->push_back(photon);
      pxc->addToFOList(*(photon));
      pyc->addToFOList(*(photon));
      pzc->addToFOList(*(photon));
      ec->addToFOList(*(photon));
    }
    streamlog_out(MESSAGE8)  << "	Value of E_lab before fit: " << E_lab << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before fit: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pxc after adding ISR before fit: " << pxc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pyc after adding ISR before fit: " << pyc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of pzc after adding ISR before fit: " << pzc->getValue() << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE8)  << "	Value of ec after adding ISR before fit: " << ec->getValue() << std::endl ; //changed from debug level 

    //To be added to fit, depending on fit hypothesis
    shared_ptr<MassConstraint> w1m = make_shared<MassConstraint>(80.398);
    w1m->addToFOList (*lfo_perm->at(0), 1);
    w1m->addToFOList (*nfo_perm->at(0), 1);
    w1m->setName("w1 mass");
    //shared_ptr<SoftGaussMassConstraint> h2m = make_shared<SoftGaussMassConstraint>(125.,3.); 
    shared_ptr<MassConstraint> w2m = make_shared<MassConstraint>(80.398);
    w2m->addToFOList (*jfo_perm->at(0), 1);
    w2m->addToFOList (*jfo_perm->at(1), 1);
    w2m->setName("w2 mass");
    shared_ptr<MassConstraint> tm = make_shared<MassConstraint>(172.52);
    tm->addToFOList (*lfo_perm->at(0), 1);
    tm->addToFOList (*nfo_perm->at(0), 1);
    tm->addToFOList (*jfo_perm->at(2), 1);
    tm->setName("t mass");
    shared_ptr<MassConstraint> tbarm = make_shared<MassConstraint>(172.52);
    tbarm->addToFOList (*jfo_perm->at(0), 1);
    tbarm->addToFOList (*jfo_perm->at(1), 1);
    tbarm->addToFOList (*jfo_perm->at(3), 1);
    tbarm->setName("tbar mass");
    //Not part of fit hypothesis, added after fit:
    shared_ptr<MassConstraint> w1 = make_shared<MassConstraint>(80.398);
    w1->addToFOList (*lfo_perm->at(0), 1);
    w1->addToFOList (*nfo_perm->at(0), 1);
    w1->setName("w1 fitted mass");
    //shared_ptr<SoftGaussMassConstraint> h2m = make_shared<SoftGaussMassConstraint>(125.,3.); 
    shared_ptr<MassConstraint> w2 = make_shared<MassConstraint>(80.398);
    w2->addToFOList (*jfo_perm->at(0), 1);
    w2->addToFOList (*jfo_perm->at(1), 1);
    w2->setName("w2 fitted mass");
    shared_ptr<MassConstraint> t = make_shared<MassConstraint>(172.52);
    t->addToFOList (*lfo_perm->at(0), 1);
    t->addToFOList (*nfo_perm->at(0), 1);
    t->addToFOList (*jfo_perm->at(2), 1);
    t->setName("t fitted mass");
    shared_ptr<MassConstraint> tbar = make_shared<MassConstraint>(172.52);
    tbar->addToFOList (*jfo_perm->at(0), 1);
    tbar->addToFOList (*jfo_perm->at(1), 1);
    tbar->addToFOList (*jfo_perm->at(3), 1);
    tbar->setName("tbar fitted mass");
    shared_ptr<MassConstraint> ttbar = make_shared<MassConstraint>(345.);
    ttbar->addToFOList(*lfo_perm->at(0), 1);
    ttbar->addToFOList(*nfo_perm->at(0), 1);
    ttbar->addToFOList (*jfo_perm->at(0), 1);
    ttbar->addToFOList (*jfo_perm->at(1), 1);
    ttbar->addToFOList (*jfo_perm->at(2), 1);
    ttbar->addToFOList (*jfo_perm->at(3), 1);
    ttbar->setName("ttbar fitted mass");
      
    streamlog_out(MESSAGE) << "start mass of W1  : " << w1->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of W2: " << w2->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of t: " << t->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of tbar: " << tbar->getMass(1) << std::endl ; //changed from debug level 
    streamlog_out(MESSAGE) << "start mass of ttbar: " << ttbar->getMass(1) << std::endl ; //changed from debug level 
    
    shared_ptr<BaseFitter> fitter = NULL;
    if ( m_fitter == 1 ) {
      fitter = shared_ptr<BaseFitter>(new NewFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewFitterGSL>(fitter))->setDebug( 4 );    
      streamlog_out(MESSAGE6) << "fit using GSL Fitter"  << std::endl ; //changed from debug level 
    } else if ( m_fitter == 2 ) {
      fitter = shared_ptr<BaseFitter>(new NewtonFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewtonFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(MESSAGE6) << "fit using Newton GSL Fitter"  << std::endl ; //changed from debug level 
    } else {
      ////		OPALFitter has no method setDebug !
      fitter = shared_ptr<BaseFitter>(new OPALFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<OPALFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(MESSAGE6) << "fit using OPAL GSL Fitter"  << std::endl ; //changed from debug level 
    }

    for(auto j : *jfo_perm) fitter->addFitObject(*j);
    for(auto l : *lfo_perm) fitter->addFitObject(*l);
    for(auto n : *nfo_perm) fitter->addFitObject(*n);

    if( m_fitISR ) {
      fitter->addFitObject( *(photon) );
      streamlog_out(MESSAGE8) << "ISR added to fit"  << std::endl ; //changed from debug level 
    }

    fitter->addConstraint( pxc.get() );
    fitter->addConstraint( pyc.get() );
    fitter->addConstraint( pzc.get() );
    fitter->addConstraint( ec.get() );
 
    fitter->addConstraint( w1m.get() );
    fitter->addConstraint( w2m.get() );
    fitter->addConstraint( tm.get() );
    fitter->addConstraint( tbarm.get() );

    streamlog_out(MESSAGE8) << "constraints added to fit:" << std::endl ; //changed from debug level 
    auto fitconstraints = fitter->getConstraints();
    auto fitsoftconstraints = fitter->getSoftConstraints();
    for (auto it = fitconstraints->begin(); it != fitconstraints->end(); ++it) {
      streamlog_out(MESSAGE8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl; //changed from debug level 
    }
    for (auto it = fitsoftconstraints->begin(); it != fitsoftconstraints->end(); ++it) {
      streamlog_out(MESSAGE8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl; //changed from debug level 
    }

    //perform fit: 
    //streamlog_out(MESSAGE) << "chi2 before fit" << calcChi2(fos) << endl; 
    float fitProbability = fitter->fit();
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from calcchi2)" << (dynamic_pointer_cast<NewFitterGSL>(fitter))->calcChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    //streamlog_out(MESSAGE) << "chi2 after fit (from fitobjects)" << calcChi2(fos) << endl; 
    fitter->addConstraint( w1.get() );
    fitter->addConstraint( w2.get() );
    fitter->addConstraint( t.get() );
    fitter->addConstraint( tbar.get() );
    fitter->addConstraint( ttbar.get() );
    //streamlog_out(MESSAGE) << "chi2 after adding helper constraints" << fitter->getChi2() << endl; 
    shared_ptr<vector<shared_ptr<BaseHardConstraint>>> constraints = make_shared<vector<shared_ptr<BaseHardConstraint>>>();
    constraints->push_back(t);
    constraints->push_back(pxc);
    constraints->push_back(pyc);
    constraints->push_back(pzc);
    constraints->push_back(ec);
    constraints->push_back(w1);
    constraints->push_back(w2);
    constraints->push_back(tbar);
    constraints->push_back(ttbar);
    //constraints->push_back(h);

    streamlog_out(MESSAGE8) << "helper constraints added"  << std::endl ; //changed from debug level 

    //---------------------------
    if(fitter->getError()==0) {
      if (fitter->getChi2() < bestChi2) {
      //if(fitProbability > bestProb) {
	streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " is better than " << bestProb << " use that one" << endl;
	streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
	streamlog_out(MESSAGE) << "error code: " << fitter->getError() << endl;
	bestProb = fitProbability;
	bestChi2 = fitter->getChi2();

	if( m_fitISR ) {
	  streamlog_out(MESSAGE)  << "After fit four-vector of ISR photon: " << *(photon) << std::endl ; //changed from debug level
	  streamlog_out(MESSAGE)  << "After fit ISR energy" << photon->getE() << endl;
	}
	for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
	  streamlog_out(MESSAGE)  << " fitted four-vector of jet"<< i_jet+1 <<": " << "[" << jets[ i_jet ]->getMomentum()[0] << ", " << jets[ i_jet ]->getMomentum()[1] << ", " << jets[ i_jet ]->getMomentum()[2] << ", " << jets[ i_jet ]->getEnergy() << "]" << std::endl ;
	}
	for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
	  streamlog_out(MESSAGE)  << " fitted four-vector of jfo_perm"<< i_jet+1 <<": " << "[" << jfo_perm->at(i_jet)->getPx() << ", " << jfo_perm->at(i_jet)->getPy() << ", " << jfo_perm->at(i_jet)->getPz() << ", " << jfo_perm->at(i_jet)->getE() << "]" << std::endl ;
	}
	for (unsigned int i_lepton =0; i_lepton < leptons.size(); i_lepton++) {
	  streamlog_out(MESSAGE)  << " fitted four-vector of lepton"<< i_lepton+1 <<": " << "[" << leptons[ i_lepton ]->getMomentum()[0] << ", " << leptons[ i_lepton ]->getMomentum()[1] << ", " << leptons[ i_lepton ]->getMomentum()[2] << ", " << leptons[ i_lepton ]->getEnergy() << "]" << std::endl ;
	}
	for (unsigned int i_lepton =0; i_lepton < leptons.size(); i_lepton++) {
	  streamlog_out(MESSAGE)  << " fitted four-vector of lfo_perm"<< i_lepton+1 <<": " << "[" << lfo_perm->at(i_lepton)->getPx() << ", " << lfo_perm->at(i_lepton)->getPy() << ", " << lfo_perm->at(i_lepton)->getPz() << ", " << lfo_perm->at(i_lepton)->getE() << "]" << std::endl ;
	}
  for (unsigned int i_neutrino =0; i_neutrino < nfo_perm->size(); i_neutrino++) {
  streamlog_out(MESSAGE)  << " raw pre-fit four-vector of neutrino: [" << missingFourMomentum.Px() << ", " << missingFourMomentum.Py() << ", " << missingFourMomentum.Pz() << ", " << missingFourMomentum.E() << "]" << std::endl ;
  }
  for (unsigned int i_neutrino =0; i_neutrino < nfo_perm->size(); i_neutrino++) {
	  streamlog_out(MESSAGE)  << " fitted four-vector of nfo_perm"<< i_neutrino+1 <<": " << "[" << nfo_perm->at(i_neutrino)->getPx() << ", " << nfo_perm->at(i_neutrino)->getPy() << ", " << nfo_perm->at(i_neutrino)->getPz() << ", " << nfo_perm->at(i_neutrino)->getE() << "]" << std::endl ;
	}
  streamlog_out(MESSAGE) << "jets W1 mass: " << sqrt(pow(leptons[0]->getEnergy()+missingFourMomentum.E(),2)-pow(leptons[0]->getMomentum()[0]+missingFourMomentum.Px(),2)-pow(leptons[0]->getMomentum()[1]+missingFourMomentum.Py(),2)-pow(leptons[0]->getMomentum()[2]+missingFourMomentum.Pz(),2)) << endl;
	streamlog_out(MESSAGE) << "jets W2 mass: " << sqrt(pow(jets[0]->getEnergy()+jets[1]->getEnergy(),2)-pow(jets[0]->getMomentum()[0]+jets[1]->getMomentum()[0],2)-pow(jets[0]->getMomentum()[1]+jets[1]->getMomentum()[1],2)-pow(jets[0]->getMomentum()[2]+jets[1]->getMomentum()[2],2)) << endl;
	streamlog_out(MESSAGE) << "jets t mass: " << sqrt(pow(leptons[0]->getEnergy()+missingFourMomentum.E()+jets[2]->getEnergy(),2)-pow(leptons[0]->getMomentum()[0]+missingFourMomentum.Px()+jets[2]->getMomentum()[0],2)-pow(leptons[0]->getMomentum()[1]+missingFourMomentum.Py()+jets[2]->getMomentum()[1],2)-pow(leptons[0]->getMomentum()[2]+missingFourMomentum.Pz()+jets[2]->getMomentum()[2],2)) << endl;
	streamlog_out(MESSAGE) << "jets tbar mass: " << sqrt(pow(jets[0]->getEnergy()+jets[1]->getEnergy()+jets[3]->getEnergy(),2)-pow(jets[0]->getMomentum()[0]+jets[1]->getMomentum()[0]+jets[3]->getMomentum()[0],2)-pow(jets[0]->getMomentum()[1]+jets[1]->getMomentum()[1]+jets[3]->getMomentum()[1],2)-pow(jets[0]->getMomentum()[2]+jets[1]->getMomentum()[2]+jets[3]->getMomentum()[2],2)) << endl;
  streamlog_out(MESSAGE) << "jfo_perm W1 mass: " << sqrt(pow(lfo_perm->at(0)->getE()+nfo_perm->at(0)->getE(),2)-pow(lfo_perm->at(0)->getPx()+nfo_perm->at(0)->getPx(),2)-pow(lfo_perm->at(0)->getPy()+nfo_perm->at(0)->getPy(),2)-pow(lfo_perm->at(0)->getPz()+nfo_perm->at(0)->getPz(),2)) << endl;
  streamlog_out(MESSAGE) << "jfo_perm W2 mass: " << sqrt(pow(jfo_perm->at(0)->getE()+jfo_perm->at(1)->getE(),2)-pow(jfo_perm->at(0)->getPx()+jfo_perm->at(1)->getPx(),2)-pow(jfo_perm->at(0)->getPy()+jfo_perm->at(1)->getPy(),2)-pow(jfo_perm->at(0)->getPz()+jfo_perm->at(1)->getPz(),2)) << endl;
  streamlog_out(MESSAGE) << "jfo_perm t mass: " << sqrt(pow(lfo_perm->at(0)->getE()+nfo_perm->at(0)->getE()+jfo_perm->at(2)->getE(),2)-pow(lfo_perm->at(0)->getPx()+nfo_perm->at(0)->getPx()+jfo_perm->at(2)->getPx(),2)-pow(lfo_perm->at(0)->getPy()+nfo_perm->at(0)->getPy()+jfo_perm->at(2)->getPy(),2)-pow(lfo_perm->at(0)->getPz()+nfo_perm->at(0)->getPz()+jfo_perm->at(2)->getPz(),2)) << endl;
  streamlog_out(MESSAGE) << "jfo_perm tbar mass: " << sqrt(pow(jfo_perm->at(0)->getE()+jfo_perm->at(1)->getE()+jfo_perm->at(3)->getE(),2)-pow(jfo_perm->at(0)->getPx()+jfo_perm->at(1)->getPx()+jfo_perm->at(3)->getPx(),2)-pow(jfo_perm->at(0)->getPy()+jfo_perm->at(1)->getPy()+jfo_perm->at(3)->getPy(),2)-pow(jfo_perm->at(0)->getPz()+jfo_perm->at(1)->getPz()+jfo_perm->at(3)->getPz(),2)) << endl;
 


 // streamlog_out(MESSAGE) << "jfo_perm H1 mass: " << sqrt(pow(jfo_perm->at(0)->getE()+jfo_perm->at(1)->getE(),2)-pow(jfo_perm->at(0)->getPx()+jfo_perm->at(1)->getPx(),2)-pow(jfo_perm->at(0)->getPy()+jfo_perm->at(1)->getPy(),2)-pow(jfo_perm->at(0)->getPz()+jfo_perm->at(1)->getPz(),2)) << endl;
 // streamlog_out(MESSAGE) << "jfo_perm H2 mass: " << sqrt(pow(jfo_perm->at(2)->getE()+jfo_perm->at(3)->getE(),2)-pow(jfo_perm->at(2)->getPx()+jfo_perm->at(3)->getPx(),2)-pow(jfo_perm->at(2)->getPy()+jfo_perm->at(3)->getPy(),2)-pow(jfo_perm->at(2)->getPz()+jfo_perm->at(3)->getPz(),2)) << endl;
 //	streamlog_out(MESSAGE) << "fos H1 mass: " << sqrt(pow(dynamic_cast<JetFitObject&>(*fos->at(0)).getE()+dynamic_cast<JetFitObject&>(*fos->at(1)).getE(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(0)).getPx()+dynamic_cast<JetFitObject&>(*fos->at(1)).getPx(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(0)).getPy()+dynamic_cast<JetFitObject&>(*fos->at(1)).getPy(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(0)).getPz()+dynamic_cast<JetFitObject&>(*fos->at(1)).getPz(),2)) << endl;
 //	streamlog_out(MESSAGE) << "fos H2 mass: " << sqrt(pow(dynamic_cast<JetFitObject&>(*fos->at(2)).getE()+dynamic_cast<JetFitObject&>(*fos->at(3)).getE(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(2)).getPx()+dynamic_cast<JetFitObject&>(*fos->at(3)).getPx(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(2)).getPy()+dynamic_cast<JetFitObject&>(*fos->at(3)).getPy(),2)-pow(dynamic_cast<JetFitObject&>(*fos->at(2)).getPz()+dynamic_cast<JetFitObject&>(*fos->at(3)).getPz(),2)) << endl;
	
 FitResult fitresult(fitter, constraints, fos);
	/*for(auto it = fitresult.constraints->begin(); it != fitresult.constraints->end(); it++) {
	  streamlog_out(MESSAGE) << "   testing " << (*it)->getName() << endl;
	  if (strcmp((*it)->getName(), "h1 mass")==0) {
	    auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	    streamlog_out(MESSAGE)<< "   higgs mass constraint: " << mc->getMass() << endl;
	  }
	  }*/
	bestFitResult = fitresult;
      } else {
	streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " not better than " << bestProb << endl;
	streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
	streamlog_out(MESSAGE) << "error code: " << fitter->getError() << endl;
      }
    } else {
      streamlog_out(MESSAGE) << "fit failed with error code " << fitter->getError() << endl;
      streamlog_out(MESSAGE) << "fit probability: " << fitProbability << endl;
      streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
    }
  }
  streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
  streamlog_out(MESSAGE) << "Converged on best fit with probability " << bestProb << endl;
  streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
  return bestFitResult;
}


ttbarKinFit::FitResult ttbarKinFit::performtvqqbbFIT( pfoVector jets, bool traceEvent, LCCollection* originalJetCollection) {
  // Empty shell so the linker doesn't complain. We will write this later.
  return FitResult();
}

void ttbarKinFit::check( LCEvent * evt ) { }
void ttbarKinFit::end() { }


void ttbarKinFit::getJetParameters(ReconstructedParticle* jet, float (&parameters)[3], float (&errors)[3])
{
  float Px , Py , Pz , P2 , Pt , Pt2;
  float dTheta_dPx , dTheta_dPy , dTheta_dPz , dPhi_dPx , dPhi_dPy;
  float sigmaPx2 , sigmaPy2 , sigmaPz2 , sigmaPxPy , sigmaPxPz , sigmaPyPz;
  ROOT::Math::PxPyPzEVector jetFourMomentum(jet->getMomentum()[0], jet->getMomentum()[1], jet->getMomentum()[2], jet->getEnergy());
  
  Px		= jetFourMomentum.Px();
  Py		= jetFourMomentum.Py();
  Pz		= jetFourMomentum.Pz();
  P2		= pow(Px,2) + pow(Py,2) + pow(Pz,2);
  Pt2		= pow(Px,2) + pow(Py,2);
  Pt		= sqrt( Pt2 );
  sigmaPx2	= jet->getCovMatrix()[0];
  sigmaPxPy	= jet->getCovMatrix()[1];
  sigmaPy2	= jet->getCovMatrix()[2];
  sigmaPxPz	= jet->getCovMatrix()[3];
  sigmaPyPz	= jet->getCovMatrix()[4];
  sigmaPz2	= jet->getCovMatrix()[5];
  
  dTheta_dPx	= Px * Pz / ( P2 * Pt );
  dTheta_dPy	= Py * Pz / ( P2 * Pt );
  dTheta_dPz	= -Pt / P2;
  dPhi_dPx	= -Py / Pt2;
  dPhi_dPy	= Px / Pt2;

  float sigmaE = std::sqrt( jet->getCovMatrix()[9] );
  float sigmaTheta = std::sqrt( std::fabs( std::pow( dTheta_dPx , 2 ) * sigmaPx2 + std::pow( dTheta_dPy , 2 ) * sigmaPy2 + std::pow( dTheta_dPz , 2 ) * sigmaPz2 +
				    2.0 * dTheta_dPx * dTheta_dPy * sigmaPxPy + 2.0 * dTheta_dPx * dTheta_dPz * sigmaPxPz + 2.0 * dTheta_dPy * dTheta_dPz * sigmaPyPz ) );
  float sigmaPhi = std::sqrt( std::fabs( std::pow( dPhi_dPx , 2 ) * sigmaPx2 + std::pow( dPhi_dPy , 2 ) * sigmaPy2 + 2.0 * dPhi_dPx * dPhi_dPy * sigmaPxPy ) );

  
  parameters[0] = jetFourMomentum.E();
  parameters[1] = jetFourMomentum.Theta();
  parameters[2] = jetFourMomentum.Phi();
  errors[0] = m_SigmaEnergyScaleFactor*sigmaE;
  errors[1] = m_SigmaAnglesScaleFactor*sigmaTheta;
  errors[2] = m_SigmaAnglesScaleFactor*sigmaPhi;


  streamlog_out(DEBUG6) << "			E       	= " << parameters[ 0 ] << std::endl ;
  streamlog_out(DEBUG6) << "			Theta		= " << parameters[ 1 ] << std::endl ;
  streamlog_out(DEBUG6) << "			Phi		= " << parameters[ 2 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaE  	= " << errors[ 0 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaTheta	= " << errors[ 1 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaPhi	= " << errors[ 2 ] << std::endl ;
}

void ttbarKinFit::getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] )
{
  TrackVec trackVec = lepton->getTracks();
  if ( trackVec.size() != 1 )
    {
      streamlog_out(DEBUG4)  << "Number of tracks for lepton is not exactly ONE!!! (nTracks = " << trackVec.size() << " ) " << std::endl ;
      streamlog_out(DEBUG4) << *lepton << std::endl;
      TLorentzVector leptonFourMomentum( lepton->getMomentum() , lepton->getEnergy() );
      float Px		= leptonFourMomentum.Px();
      float Py		= leptonFourMomentum.Py();
      float Pz		= leptonFourMomentum.Pz();
      float P2		= ( leptonFourMomentum.Vect() ).Mag2();
      float Pt2		= std::pow( Px , 2 ) + std::pow( Py , 2 );
      float Pt		= std::sqrt( Pt2 );
      
      float sigmaPx2		= lepton->getCovMatrix()[ 0 ];
      float sigmaPxPy		= lepton->getCovMatrix()[ 1 ];
      float sigmaPy2		= lepton->getCovMatrix()[ 2 ];
      float sigmaPxPz		= lepton->getCovMatrix()[ 3 ];
      float sigmaPyPz		= lepton->getCovMatrix()[ 4 ];
      float sigmaPz2		= lepton->getCovMatrix()[ 5 ];
      
      float dInvPt_dPx	= - Px / ( Pt * Pt2 );
      float dInvPt_dPy	= - Py / ( Pt * Pt2 );
      float dTheta_dPx	= Px * Pz / ( P2 * Pt );
      float dTheta_dPy	= Py * Pz / ( P2 * Pt );
      float dTheta_dPz	= -Pt / P2;
      float dPhi_dPx		= -Py / Pt2;
      float dPhi_dPy		= Px / Pt2;
      
      parameters[ 0 ] = 1.0 / std::sqrt( std::pow( leptonFourMomentum.Px() , 2 ) + std::pow( leptonFourMomentum.Py() , 2 ) );
      parameters[ 1 ] = leptonFourMomentum.Theta();
      parameters[ 2 ] = leptonFourMomentum.Phi();
      errors[ 0 ]	= std::sqrt( std::pow( dInvPt_dPx , 2 ) * sigmaPx2 + std::pow( dInvPt_dPy , 2 ) * sigmaPy2 + 2.0 * dInvPt_dPx * dInvPt_dPy * sigmaPxPy );
      errors[ 1 ]	= std::sqrt( std::fabs( std::pow( dTheta_dPx , 2 ) * sigmaPx2 + std::pow( dTheta_dPy , 2 ) * sigmaPy2 + std::pow( dTheta_dPz , 2 ) * sigmaPz2 +
						2.0 * dTheta_dPx * dTheta_dPy * sigmaPxPy + 2.0 * dTheta_dPx * dTheta_dPz * sigmaPxPz + 2.0 * dTheta_dPy * dTheta_dPz * sigmaPyPz ) );
      errors[ 2 ]	= std::sqrt( std::fabs( std::pow( dPhi_dPx , 2 ) * sigmaPx2 + std::pow( dPhi_dPy , 2 ) * sigmaPy2 + 2.0 * dPhi_dPx * dPhi_dPy * sigmaPxPy ) );
    }
  else
    {
      streamlog_out(DEBUG4)  << "	Lepton has exactly ONE track:" << std::endl ;
      streamlog_out(DEBUG4) << *lepton << std::endl;
      streamlog_out(DEBUG4) << *trackVec[ 0 ] << std::endl;
      float Omega		= trackVec[ 0 ]->getOmega();
      float tanLambda		= trackVec[ 0 ]->getTanLambda();
      float Theta		= 2.0 * atan( 1.0 ) - atan( tanLambda );//atan( 1.0 / tanLambda );
      float Phi		= trackVec[ 0 ]->getPhi();
      
      float sigmaOmega	= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 5 ] );
      float sigmaTanLambda	= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 14 ] );
      float sigmaPhi		= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 2 ] );
      
      float dTheta_dTanLambda	= -1.0 / ( 1.0 + std::pow( tanLambda , 2 ) );
      
      parameters[ 0 ]	= Omega / eB;
      parameters[ 1 ]	= Theta;
      parameters[ 2 ]	= Phi;
      errors[ 0 ]	= sigmaOmega / eB;
      errors[ 1 ]	= std::fabs( dTheta_dTanLambda ) * sigmaTanLambda;
      errors[ 2 ]	= sigmaPhi;
    }
  streamlog_out(DEBUG6) << "			Inverse pT	= " << parameters[ 0 ] << std::endl ;
  streamlog_out(DEBUG6) << "			Theta		= " << parameters[ 1 ] << std::endl ;
  streamlog_out(DEBUG6) << "			Phi		= " << parameters[ 2 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaInverse pT	= " << errors[ 0 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaTheta	= " << errors[ 1 ] << std::endl ;
  streamlog_out(DEBUG6) << "			SigmaPhi	= " << errors[ 2 ] << std::endl ;
}

std::pair<std::vector<double>,std::vector<double>> ttbarKinFit::calculateInitialValues(pfoVector jets, pfoVector leptons, vector<int> perm)
{
  std::vector<double> masses;
  std::vector<double> fourmomentum;
  shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
  shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo= make_shared<vector<shared_ptr<LeptonFitObject>>>();
  //Set JetFitObjects
  for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getJetParameters( jets[ i_jet ] , parameters , errors );
    auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
    jfo->push_back(j);
    const string name = "jet"+to_string(i_jet);
    j->setName(name.c_str());
  }
  //Set LeptonFitObjects
  for (unsigned int i_lep =0; i_lep < leptons.size(); i_lep++) {
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getLeptonParameters( leptons[ i_lep ] , parameters , errors );
    auto l = make_shared<LeptonFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , leptons[ i_lep ]->getMass() );
    lfo->push_back(l);
    const string name = "lepton"+to_string(i_lep);
    l->setName(name.c_str());
  }
  shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();
  for (auto i = perm.begin(); i != perm.begin()+m_nJets; ++i) {
    //for(auto i : perm) {
    streamlog_out(MESSAGE) << " Picking jet for start masses " << *i << std::endl ;
    auto jsp = make_shared<JetFitObject>(*jfo->at(*i));
    jfo_perm->push_back(jsp);
  }
  //calculate 4-momentum of W->invisible
  // correct for crossing angle
  float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
  double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);
  ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
  ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
  for (int i_jet = 0; i_jet < m_nJets; i_jet++) {
    seenFourMomentum += ROOT::Math::PxPyPzEVector(jets[ i_jet ]->getMomentum()[0],jets[ i_jet ]->getMomentum()[1],jets[ i_jet ]->getMomentum()[2], jets[ i_jet ]->getEnergy());
  }
  //accounting for leptons
  for (int i_lep = 0; i_lep < m_nLeps; i_lep++) {
    seenFourMomentum += ROOT::Math::PxPyPzEVector(leptons[ i_lep ]->getMomentum()[0],leptons[ i_lep ]->getMomentum()[1],leptons[ i_lep ]->getMomentum()[2], leptons[ i_lep ]->getEnergy());
  }
  ROOT::Math::PxPyPzEVector missingFourMomentum = ecms-seenFourMomentum;
  shared_ptr<NeutrinoFitObjectNew> nfo = make_shared<NeutrinoFitObjectNew> (missingFourMomentum.Px(), missingFourMomentum.Py(), missingFourMomentum.Pz(), 1.0, 1.0, 1.0, missingFourMomentum.M());
  nfo->setName("Neutrino");

  shared_ptr<MomentumConstraint> pxc = make_shared<MomentumConstraint>( 0 , 1 , 0 , 0 , target_p_due_crossing_angle);//Factor for: (energy sum, px sum, py sum,pz sum,target value of sum)                       
  pxc->setName("sum(p_x)");
  for (auto j : *jfo_perm) pxc->addToFOList(*j);
  for (auto l : *lfo) pxc->addToFOList(*l);
  if (m_signature == "lvqqbb" || m_signature == "tvqqbb") pxc->addToFOList(*nfo);
  
  shared_ptr<MomentumConstraint> pyc = make_shared<MomentumConstraint>(0, 0, 1, 0, 0);
  pyc->setName("sum(p_y)");
  for (auto j : *jfo_perm) pyc->addToFOList(*j);
  for (auto l : *lfo) pyc->addToFOList(*l); 
  if (m_signature == "lvqqbb" || m_signature == "tvqqbb") pyc->addToFOList(*nfo);
 
  shared_ptr<MomentumConstraint> pzc = make_shared<MomentumConstraint>(0, 0, 0, 1, 0);
  pzc->setName("sum(p_z)");
  for (auto j : *jfo_perm) pzc->addToFOList(*j);
  for (auto l : *lfo) pzc->addToFOList(*l);
  if (m_signature == "lvqqbb" || m_signature == "tvqqbb") pzc->addToFOList(*nfo);
  
  shared_ptr<MomentumConstraint> ec = make_shared<MomentumConstraint>(1, 0, 0, 0, E_lab);
  ec->setName("sum(E)");
  for (auto j : *jfo_perm) ec->addToFOList(*j);
  for (auto l : *lfo) ec->addToFOList(*l);
  if (m_signature == "lvqqbb" || m_signature == "tvqqbb") ec->addToFOList(*nfo);
  
  fourmomentum.push_back(pxc->getValue());
  fourmomentum.push_back(pyc->getValue());
  fourmomentum.push_back(pzc->getValue());
  fourmomentum.push_back(ec->getValue());
  
    shared_ptr<MassConstraint> h1 = make_shared<MassConstraint>(125.);
    h1->addToFOList (*jfo_perm->at(0), 1);
    h1->addToFOList (*jfo_perm->at(1), 1);
    h1->setName("h1 mass");  
    shared_ptr<MassConstraint> h2 = make_shared<MassConstraint>(125.);
    h2->addToFOList (*jfo_perm->at(2), 1);
    h2->addToFOList (*jfo_perm->at(3), 1);
    h2->setName("h2 mass");
    shared_ptr<MassConstraint> z = make_shared<MassConstraint>(91.2);
    if (m_signature == "llbbbb") {
      z->addToFOList(*lfo->at(0), 1);
      z->addToFOList(*lfo->at(1), 1);
    } else if (m_signature == "vvbbbb") {
      z->addToFOList(*zfo, 1);      
    } else if (m_signature == "qqbbbb") {
      z->addToFOList(*jfo_perm->at(4), 1);
      z->addToFOList(*jfo_perm->at(5), 1);
    }
    z->setName("z mass");  
    shared_ptr<MassConstraint> hh = make_shared<MassConstraint>(250.);
    hh->addToFOList (*jfo_perm->at(0), 1);
    hh->addToFOList (*jfo_perm->at(1), 1);
    hh->addToFOList (*jfo_perm->at(2), 1);
    hh->addToFOList (*jfo_perm->at(3), 1);
    hh->setName("hh mass");
    shared_ptr<MassConstraint> zhh = make_shared<MassConstraint>(250.);
    if (m_signature == "llbbbb") {
      zhh->addToFOList(*lfo->at(0), 1);
      zhh->addToFOList(*lfo->at(1), 1);
    } else if (m_signature == "vvbbbb") {
      zhh->addToFOList(*zfo, 1);      
    } else if (m_signature == "qqbbbb") {
      zhh->addToFOList(*jfo_perm->at(4), 1);
      zhh->addToFOList(*jfo_perm->at(5), 1);
    }
    zhh->addToFOList (*jfo_perm->at(0), 1);
    zhh->addToFOList (*jfo_perm->at(1), 1);
    zhh->addToFOList (*jfo_perm->at(2), 1);
    zhh->addToFOList (*jfo_perm->at(3), 1);
    zhh->setName("zhh mass");
    masses.push_back(z->getMass(1));
    masses.push_back(h1->getMass(1));
    masses.push_back(h2->getMass(1));
    masses.push_back(hh->getMass(1));
    masses.push_back(zhh->getMass(1));

    std::vector<ROOT::Math::PxPyPzEVector> dijetMomenta = { v4(z.get()), v4(h1.get()), v4(h2.get()) };
    float p1st = 0;
    float cos1st = 0;

    for (ROOT::Math::PxPyPzEVector fourMom: dijetMomenta) {
      if (fourMom.P() > p1st) {
        p1st = fourMom.P();
        cos1st = cos(fourMom.Theta());
      }
    }

    m_p1stBeforeFit = p1st;
    m_cos1stBeforeFit = cos1st;

    return make_pair(masses,fourmomentum);
}

std::vector<double> ZHHKinFit::calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons) 
{
  std::vector<double> masses;
  vector<vector<unsigned int>> perms;
  if (m_fithypothesis == "ZZH" || m_fithypothesis == "MH") {
    perms = {
      {0, 1, 2, 3},
      {0, 2, 1, 3},
      {0, 3, 1, 2},
      {1, 2, 0, 3},
      {1, 3, 0, 2},
      {2, 3, 0, 1}
    };
  } else{
    perms = {
      {0, 1, 2, 3},
      {0, 2, 1, 3},
      {0, 3, 1, 2}
    };
  }
  double m1;
  double m2;
  if (m_fithypothesis == "ZZH") {
    m1 = 125.;
    m2 = 91.2;
  } else {
    m1 = 125.; 
    m2 = 125.; 
  }
  double z = 0. ;
  double h1 = 0. ;
  double h2 = 0. ;
  if (m_signature == "llbbbb") {
    z = inv_mass(leptons.at(0),leptons.at(1));
  } else if (m_signature == "vvbbbb") {
      //calculate 4-momentum of Z->invisible
    // correct for crossing angle
    float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
    double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);
    ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle,0.,0.,E_lab) ;
    ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
    for (int i_jet = 0; i_jet < m_nJets; i_jet++) {
      seenFourMomentum += ROOT::Math::PxPyPzEVector(jets[ i_jet ]->getMomentum()[0],jets[ i_jet ]->getMomentum()[1],jets[ i_jet ]->getMomentum()[2], jets[ i_jet ]->getEnergy());
    }
    ROOT::Math::PxPyPzEVector missingFourMomentum = ecms-seenFourMomentum;
    z = missingFourMomentum.M();
  }
  ROOT::Math::PxPyPzEVector hhFourMomentum(0.,0.,0.,0.);
  ROOT::Math::PxPyPzEVector zhhFourMomentum(0.,0.,0.,0.);
  for (auto jet : jets) {
    hhFourMomentum +=  ROOT::Math::PxPyPzEVector( jet->getMomentum()[0], jet->getMomentum()[1], jet->getMomentum()[2] , jet->getEnergy() );
  }
  zhhFourMomentum += hhFourMomentum;
  for (auto lepton : leptons) {
    zhhFourMomentum +=  ROOT::Math::PxPyPzEVector( lepton->getMomentum()[0], lepton->getMomentum()[1], lepton->getMomentum()[2] , lepton->getEnergy() );
  }  
  double hh = hhFourMomentum.M();
  double zhh = zhhFourMomentum.M();
  double chi2min = 99999. ;
  for (auto perm : perms) {
    double temp1 = inv_mass(jets.at(perm[0]),jets.at(perm[1]));
    double temp2 = inv_mass(jets.at(perm[2]),jets.at(perm[3]));
    double chi2;
    if (m_fithypothesis == "MH") {
      chi2 = (temp1-m1)*(temp1-m1);
    } else {      
      chi2 = (temp1-m1)*(temp1-m1)+(temp2-m2)*(temp2-m2);
    }
    if (chi2 < chi2min) {
      chi2min = chi2;
      h1 = temp1;
      h2 = temp2;
    }
  }
  masses.push_back(z);
  masses.push_back(h1);
  masses.push_back(h2);
  masses.push_back(hh);
  masses.push_back(zhh);

  streamlog_out(MESSAGE) << "masses from simple chi2:" << z << ", " << h1 << ", " << h2 << ", " << hh << ", " << zhh << std::endl ; 

  return masses;

}



std::vector<double> ZHHKinFit::calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type)
{
  std::vector<double> pulls;
  double start, fitted;
  double errfit, errmea, sigma;
  float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
  if (type == 1) getJetParameters(startobject, parameters, errors);
  if (type == 2) getLeptonParameters(startobject, parameters, errors);
  for (int ipar = 0; ipar < 3; ipar++) {
    fitted = fittedobject->getParam(ipar);
    start = parameters[ipar];
    errfit = fittedobject->getError(ipar);
    errmea = errors[ipar];
    sigma = errmea*errmea-errfit*errfit;
    //streamlog_out(MESSAGE) << "fitted = " << fitted << ", start = " << start << std::endl ;
    //streamlog_out(MESSAGE) << "errfit = " << errfit << ", errmea = " << errmea << ", sigma = " << sigma << std::endl ;
    if (sigma > 0) {
      sigma = sqrt(sigma);
    //streamlog_out(MESSAGE) << "pull = " << (fitted - start)/sigma << std::endl ;
      pulls.push_back((fitted - start)/sigma);
    }
    else {
      streamlog_out(DEBUG3) << "NOT GOOD...................." << std::endl ;
      pulls.push_back(-999.);
    }
  }
  return pulls;
}

double ZHHKinFit::calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects) {
  double chi2 = 0;
  for (auto it = fitobjects->begin(); it != fitobjects->end(); ++it) {
    shared_ptr<BaseFitObject> fo = *it;
    assert (fo);
    chi2 += fo->getChi2();
  }/*
  for (SoftConstraintIterator i = softconstraints.begin(); i != softconstraints.end(); ++i) {
    BaseSoftConstraint *bsc = *i;
    assert (bsc);
    chi2 += bsc->getChi2();
    }*/
  return chi2;
}

void ZHHKinFit::check( LCEvent* )
{
//	nothing to check here - could be used to fill checkplots in reconstruction processor
}


void ZHHKinFit::end()
{
//	streamlog_out(MESSAGE) << "# of events: " << m_nEvt << std::endl;
//	streamlog_out(ERROR) << "# of nucorrection: " << correction<< std::endl;
//	streamlog_out(ERROR) << "# of Covariance failed: " << nCo<< std::endl;
  if (m_pTFile != nullptr)
    m_pTFile->cd();
  
  m_pTTree->Write();

  if (m_pTFile != nullptr) {
    m_pTFile->Close();
    delete m_pTFile;
  }
}
