#include "ZHHllbbbbKinFit.h"

using namespace lcio ;
using namespace marlin ;
using namespace std ;
using namespace CLHEP ;

ZHHllbbbbKinFit aZHHllbbbbKinFit ;

ZHHllbbbbKinFit::ZHHllbbbbKinFit() :
Processor("ZHHllbbbbKinFit"),
m_nRun(0),
m_nEvt(0),
m_nRunSum(0),
m_nEvtSum(0),
m_Bfield(0.0),
c(0.0),
mm2m(0.0),
eV2GeV(0.0),
eB(0.0)
{

//	modify processor description
	_description = "ZHHllbbbbKinFit does a fit on 2 lepton + 4 jet events (Px, Py, Pz, E, M12 = MZ)" ;

//	register steering parameters: name, description, class-variable, default value

	registerInputCollection( 	LCIO::RECONSTRUCTEDPARTICLE,
					"isolatedleptonCollection" ,
					"Name of the Isolated Lepton collection"  ,
					m_inputIsolatedleptonCollection ,
					std::string("ISOLeptons")
				);

	registerInputCollection( 	LCIO::RECONSTRUCTEDPARTICLE,
					"JetCollectionName" ,
					"Name of the Jet collection"  ,
					m_inputJetCollection ,
					std::string("Durham4Jets")
				);

	registerInputCollection( 	LCIO::VERTEX,
					"SLDVertexCollection" ,
					"Name of Semi-Leptonic Decay Vertices Collection"  ,
					m_inputSLDVertexCollection ,
					std::string("SemiLeptonicDecayVertex")
				);

	registerInputCollection(	LCIO::LCRELATION,
					"JetSLDRelationCollection",
					"Name of the Jet-SemiLeptonicDecay Relation collection",
					m_inputJetSLDLink,
					std::string("JetSLDLinkName")
				);

	registerInputCollection(	LCIO::LCRELATION,
					"SLDNeutrinoRelationCollection",
					"Name of the JetSemiLeptonicDecayVertex-Neutrinos Relation collection",
					m_inputSLDNuLink,
					std::string("SLDNuLinkName")
				);

	registerInputCollection(       LCIO::LCRELATION,
                                        "recoNumcNuLinkName",
                                        "Name of the reconstructedNeutrino-trueNeutrino input Link collection",
                                        m_recoNumcNuLinkName,
                                        std::string("recoNumcNuLinkName")
					);

	registerInputCollection( 	LCIO::MCPARTICLE,
					"MCParticleCollection" ,
					"Name of the MCParticle collection"  ,
					_MCParticleColllectionName ,
					std::string("MCParticlesSkimmed")
					);

	registerInputCollection( 	LCIO::RECONSTRUCTEDPARTICLE,
					"RecoParticleCollection" ,
					"Name of the ReconstructedParticles input collection"  ,
					_recoParticleCollectionName ,
					std::string("PandoraPFOs")
				);

	registerInputCollection( 	LCIO::LCRELATION,
					"RecoMCTruthLink",
					"Name of the RecoMCTruthLink input collection"  ,
					_recoMCTruthLink,
					std::string("RecoMCTruthLink")
				);

	registerProcessorParameter(	"nJets",
					"Number of jet should be in the event",
					m_nAskedJets,
					int(4)
				);

	registerProcessorParameter(	"nIsoLeps",
					"Number of Isolated Leptons should be in the event",
					m_nAskedIsoLeps,
					int(2)
				);

	registerProcessorParameter(	"ECM" ,
					"Center-of-Mass Energy in GeV",
					m_ECM,
					float(500.f)
				);

	registerProcessorParameter(	"ISRPzMax" ,
					"Maximum possible energy for a single ISR photon",
					m_isrpzmax,
					float(125.6f)
				);

	registerProcessorParameter(     "SigmaInvPtScaleFactor" ,
				        "Factor for scaling up inverse pT error",
				        m_SigmaInvPtScaleFactor,
				        float(1.0f));

	registerProcessorParameter(	"SigmaEnergyScaleFactor" ,
					"Factor for scaling up energy error",
					m_SigmaEnergyScaleFactor,
					float(1.0f)
				);

	registerProcessorParameter(     "SigmaAnglesScaleFactor" ,
				        "Factor for scaling up angular errors",
				        m_SigmaAnglesScaleFactor,
				        float(1.0f));

	registerProcessorParameter(	"includeISR",
					"Include ISR in fit hypothesis; false: without ISR , true: with ISR",
					m_fitISR,
					bool(true)
				);

	registerProcessorParameter(	"fitter" ,
					"0 = OPALFitter, 1 = NewFitter, 2 = NewtonFitter",
					m_fitter,
					int(0)
				);

	registerProcessorParameter(     "fithypothesis",
                                        "name of the fit hypothesis",
                                        m_fithypothesis,
                                        std::string("")
					);

	registerProcessorParameter(	"outputFilename",
					"name of output root file",
					m_outputFile,
					std::string("")
				);

	registerProcessorParameter(	"traceall" ,
					"set true if every event should be traced",
					m_traceall,
					(bool)false
				);

	registerProcessorParameter(	"ievttrace" ,
					"number of individual event to be traced",
					m_ievttrace,
					(int)0
				);

	// Outputs: Fitted jet and leptons and their prefit counterparts, the neutrino correction, and pulls
	registerInputCollection( 	LCIO::RECONSTRUCTEDPARTICLE,
					"outputLeptonCollection" ,
					"Name of output fitted lepton collection"  ,
					m_outputLeptonCollection ,
					std::string("ISOLeptons_KinFit")
				);

	registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"outputJetCollection",
					"Name of output fitted jet collection",
					m_outputJetCollection,
					std::string("Durham_4JetsKinFit")
				);
	
	registerInputCollection( 	LCIO::RECONSTRUCTEDPARTICLE,
					"outputStartLeptonCollection" ,
					"Name of output prefit lepton collection"  ,
					m_outputStartLeptonCollection ,
					std::string("ISOLeptons_PreFit")
				);

	registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"outputStartJetCollection",
					"Name of output prefit jet collection",
					m_outputStartJetCollection,
					std::string("Durham_4JetsPreFit")
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

	registerOutputCollection( LCIO::LCFLOATVEC,
				  "LeptonPullsOutputCollection",
				  "Output LeptonPulls (invPt, theta, phi)  Collection" ,
				  _OutLeptonPullsCol,
				  std::string("LeptonPulls"));

	registerOutputCollection( LCIO::LCFLOATVEC,
				  "JetPullsOutputCollection",
				  "Output JetPulls (E, theta, phi)  Collection" ,
				  _OutJetPullsCol,
				  std::string("JetPulls"));
}

void ZHHllbbbbKinFit::init()
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

	m_pTFile = new TFile(m_outputFile.c_str(),"recreate");

	m_pTTree = new TTree("eventTree","eventTree");
	m_pTTree->SetDirectory(m_pTFile);
	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");
	m_pTTree->Branch("nJets",&m_nJets,"nJets/I") ;
	m_pTTree->Branch("nIsoLeptons",&m_nIsoLeps,"nIsoLeptons/I") ;
	m_pTTree->Branch("nSLDecayBHadron",&m_nSLDecayBHadron,"nSLDecayBHadron/I") ;
	m_pTTree->Branch("nSLDecayCHadron",&m_nSLDecayCHadron,"nSLDecayCHadron/I") ;
	m_pTTree->Branch("nSLDecayTauLepton",&m_nSLDecayTauLepton,"nSLDecayTauLepton/I") ;
	m_pTTree->Branch("nSLDecayTotal",&m_nSLDecayTotal,"nSLDecayTotal/I") ;
	m_pTTree->Branch("nCorrectedSLD",&m_nCorrectedSLD,"nCorrectedSLD/I") ;
	m_pTTree->Branch( "FitErrorCode_woNu" , &m_FitErrorCode_woNu , "FitErrorCode_woNu/I" );
	m_pTTree->Branch( "ZMassBeforeFit_woNu" , &m_ZMassBeforeFit_woNu , "ZMassBeforeFit_woNu/F" );
	m_pTTree->Branch( "H1MassBeforeFit_woNu" , &m_H1MassBeforeFit_woNu , "H1MassBeforeFit_woNu/F" );
	m_pTTree->Branch( "H2MassBeforeFit_woNu" , &m_H2MassBeforeFit_woNu , "H2MassBeforeFit_woNu/F" );
	m_pTTree->Branch( "ZMassAfterFit_woNu" , &m_ZMassAfterFit_woNu , "ZMassAfterFit_woNu/F" );
	m_pTTree->Branch( "H1MassAfterFit_woNu" , &m_H1MassAfterFit_woNu , "H1MassAfterFit_woNu/F" );
	m_pTTree->Branch( "H2MassAfterFit_woNu" , &m_H2MassAfterFit_woNu , "H2MassAfterFit_woNu/F" );
	m_pTTree->Branch( "FitProbability_woNu" , &m_FitProbability_woNu , "FitProbability_woNu/F" );
	m_pTTree->Branch( "FitChi2_woNu" , &m_FitChi2_woNu , "FitChi2_woNu/F" );
	m_pTTree->Branch( "pullJetEnergy_woNu" , &m_pullJetEnergy_woNu );
	m_pTTree->Branch( "pullJetTheta_woNu" , &m_pullJetTheta_woNu );
	m_pTTree->Branch( "pullJetPhi_woNu" , &m_pullJetPhi_woNu );
	m_pTTree->Branch( "pullLeptonInvPt_woNu" , &m_pullLeptonInvPt_woNu );
	m_pTTree->Branch( "pullLeptonTheta_woNu" , &m_pullLeptonTheta_woNu );
	m_pTTree->Branch( "pullLeptonPhi_woNu" , &m_pullLeptonPhi_woNu );
	/*	m_pTTree->Branch( "FitErrorCode_wNu" , &m_FitErrorCode_wNu , "FitErrorCode_wNu/I" );
	m_pTTree->Branch( "ZMassBeforeFit_wNu" , &m_ZMassBeforeFit_wNu , "ZMassBeforeFit_wNu/F" );
	m_pTTree->Branch( "H1MassBeforeFit_wNu" , &m_H1MassBeforeFit_wNu , "H1MassBeforeFit_wNu/F" );
	m_pTTree->Branch( "H2MassBeforeFit_wNu" , &m_H2MassBeforeFit_wNu , "H2MassBeforeFit_wNu/F" );
	m_pTTree->Branch( "ZMassAfterFit_wNu" , &m_ZMassAfterFit_wNu , "ZMassAfterFit_wNu/F" );
	m_pTTree->Branch( "H1MassAfterFit_wNu" , &m_H1MassAfterFit_wNu , "H1MassAfterFit_wNu/F" );
	m_pTTree->Branch( "H2MassAfterFit_wNu" , &m_H2MassAfterFit_wNu , "H2MassAfterFit_wNu/F" );
	m_pTTree->Branch( "FitProbability_wNu" , &m_FitProbability_wNu , "FitProbability_wNu/F" );
	m_pTTree->Branch( "FitChi2_wNu" , &m_FitChi2_wNu , "FitChi2_wNu/F" );
	m_pTTree->Branch( "pullJetEnergy_wNu" , &m_pullJetEnergy_wNu );
	m_pTTree->Branch( "pullJetTheta_wNu" , &m_pullJetTheta_wNu );
	m_pTTree->Branch( "pullJetPhi_wNu" , &m_pullJetPhi_wNu );
	m_pTTree->Branch( "pullLeptonInvPt_wNu" , &m_pullLeptonInvPt_wNu );
	m_pTTree->Branch( "pullLeptonTheta_wNu" , &m_pullLeptonTheta_wNu );
	m_pTTree->Branch( "pullLeptonPhi_wNu" , &m_pullLeptonPhi_wNu );*/
	m_pTTree->Branch( "FitErrorCode" , &m_FitErrorCode , "FitErrorCode/I" );
	m_pTTree->Branch( "ZMassBeforeFit" , &m_ZMassBeforeFit , "ZMassBeforeFit/F" );
	m_pTTree->Branch( "H1MassBeforeFit" , &m_H1MassBeforeFit , "H1MassBeforeFit/F" );
	m_pTTree->Branch( "H2MassBeforeFit" , &m_H2MassBeforeFit , "H2MassBeforeFit/F" );
	m_pTTree->Branch( "ZMassAfterFit" , &m_ZMassAfterFit , "ZMassAfterFit/F" );
	m_pTTree->Branch( "H1MassAfterFit" , &m_H1MassAfterFit , "H1MassAfterFit/F" );
	m_pTTree->Branch( "H2MassAfterFit" , &m_H2MassAfterFit , "H2MassAfterFit/F" );
	m_pTTree->Branch( "FitProbability" , &m_FitProbability , "FitProbability/F" );
	m_pTTree->Branch( "FitChi2" , &m_FitChi2 , "FitChi2/F" );
	m_pTTree->Branch( "pullJetEnergy" , &m_pullJetEnergy );
	m_pTTree->Branch( "pullJetTheta" , &m_pullJetTheta );
	m_pTTree->Branch( "pullJetPhi" , &m_pullJetPhi );
	m_pTTree->Branch( "pullLeptonInvPt" , &m_pullLeptonInvPt );
	m_pTTree->Branch( "pullLeptonTheta" , &m_pullLeptonTheta );
	m_pTTree->Branch( "pullLeptonPhi" , &m_pullLeptonPhi );
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

	streamlog_out(DEBUG) << "   init finished  " << std::endl;

}

void ZHHllbbbbKinFit::Clear()
{
	streamlog_out(DEBUG) << "   Clear called  " << std::endl;

	m_nJets = 0;
	m_nIsoLeps = 0;
	m_nSLDecayBHadron = 0;
	m_nSLDecayCHadron = 0;
	m_nSLDecayTauLepton = 0;
	m_nSLDecayTotal = 0;
	m_nCorrectedSLD = 0;
	m_FitErrorCode_woNu = 1;
	m_ZMassBeforeFit_woNu = 0.0;
	m_H1MassBeforeFit_woNu = 0.0;
	m_H2MassBeforeFit_woNu = 0.0;
	m_ZMassAfterFit_woNu = 0.0;
	m_H1MassAfterFit_woNu = 0.0;
	m_H2MassAfterFit_woNu = 0.0;
	m_FitProbability_woNu = 0.0;
	m_FitChi2_woNu = 0.0;
	m_pullJetEnergy_woNu.clear();
	m_pullJetTheta_woNu.clear();
	m_pullJetPhi_woNu.clear();
	m_pullLeptonInvPt_woNu.clear();
	m_pullLeptonTheta_woNu.clear();
	m_pullLeptonPhi_woNu.clear();
	/*m_FitErrorCode_wNu = 1;
	m_ZMassBeforeFit_wNu = 0.0;
	m_H1MassBeforeFit_wNu = 0.0;
	m_H2MassBeforeFit_wNu = 0.0;
	m_ZMassAfterFit_wNu = 0.0;
	m_H1MassAfterFit_wNu = 0.0;
	m_H2MassAfterFit_wNu = 0.0;
	m_FitProbability_wNu = 0.0;
	m_FitChi2_wNu = 0.0;
	m_pullJetEnergy_wNu.clear();
	m_pullJetTheta_wNu.clear();
	m_pullJetPhi_wNu.clear();
	m_pullLeptonInvPt_wNu.clear();
	m_pullLeptonTheta_wNu.clear();
	m_pullLeptonPhi_wNu.clear();*/
	m_FitErrorCode = 1;
	m_ZMassBeforeFit = 0.0;
	m_H1MassBeforeFit = 0.0;
	m_H2MassBeforeFit = 0.0;
	m_ZMassAfterFit = 0.0;
	m_H1MassAfterFit = 0.0;
	m_H2MassAfterFit = 0.0;
	m_FitProbability = 0.0;
	m_FitChi2 = 0.0;
	m_pullJetEnergy.clear();
	m_pullJetTheta.clear();
	m_pullJetPhi.clear();
	m_pullLeptonInvPt.clear();
	m_pullLeptonTheta.clear();
	m_pullLeptonPhi.clear();
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

}

void ZHHllbbbbKinFit::processRunHeader()
{
	m_nRun++ ;
}

void ZHHllbbbbKinFit::processEvent( EVENT::LCEvent *pLCEvent )
{
  this->Clear();
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
  streamlog_out(MESSAGE1) << "	////////////////////////////////// processing event " << m_nEvt << " in run " << m_nRun << " /////////////////////////////////////////////////" << std::endl;
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
  
  LCCollection *inputJetCollection = NULL;
  LCCollection *inputLeptonCollection = NULL;
  LCCollection *inputSLDecayCollection = NULL;
  IMPL::LCCollectionVec* outputLeptonCollection = NULL;
  outputLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  outputLeptonCollection->setSubset( true );
  IMPL::LCCollectionVec* outputJetCollection = NULL;
  outputJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  outputJetCollection->setSubset( true );
  IMPL::LCCollectionVec* outputStartLeptonCollection = NULL;
  outputStartLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  outputStartLeptonCollection->setSubset( true );
  IMPL::LCCollectionVec* outputStartJetCollection = NULL;
  outputStartJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  outputStartJetCollection->setSubset( true );
  LCCollectionVec *outputNuEnergyCollection = new LCCollectionVec(LCIO::LCFLOATVEC);

  LCRelationNavigator* JetSLDNav = NULL;
  LCRelationNavigator* SLDNuNav = NULL;
  LCRelationNavigator* NuMCNav = NULL;
  try {
    streamlog_out(DEBUG0) << "	getting jet collection: " << m_inputJetCollection << std::endl ;
    inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    streamlog_out(DEBUG0) << "	getting isolated lepton collection: " << m_inputIsolatedleptonCollection << std::endl ;
    inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
    streamlog_out(DEBUG0) << "	getting semi-leptonic vertex collection: " << m_inputSLDVertexCollection << std::endl ;
    inputSLDecayCollection = pLCEvent->getCollection( m_inputSLDVertexCollection );
    JetSLDNav = new LCRelationNavigator( pLCEvent->getCollection( m_inputJetSLDLink ) );
    SLDNuNav = new LCRelationNavigator( pLCEvent->getCollection( m_inputSLDNuLink ) );
    //not needed for the fit itself, only to find corresponding MCParticle for each neutrino:
    NuMCNav = new LCRelationNavigator( pLCEvent->getCollection( m_recoNumcNuLinkName ) );
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_nEvt << std::endl;
    return;
  }
  m_nCorrectedSLD = inputSLDecayCollection->getNumberOfElements();
  m_nSLDecayBHadron = inputSLDecayCollection->getParameters().getIntVal( "nBHadronSLD_found" );
  m_nSLDecayCHadron = inputSLDecayCollection->getParameters().getIntVal( "nCHadronSLD_found" );
  m_nSLDecayTauLepton = inputSLDecayCollection->getParameters().getIntVal( "nTauLeptonSLD_found" );
  m_nSLDecayTotal = inputSLDecayCollection->getParameters().getIntVal( "nTotalSLD_found" );
  m_nJets = inputJetCollection->getNumberOfElements();
  m_nIsoLeps = inputLeptonCollection->getNumberOfElements();
  streamlog_out(MESSAGE8) << "	Number of jets: " << m_nJets << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of isolatedLeptons: " << m_nIsoLeps << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of found semi-leptonic decays: " << m_nSLDecayTotal << std::endl ;
  streamlog_out(MESSAGE8) << "	Number of corrected semi-leptonic decays: " << m_nCorrectedSLD << std::endl ;
  if ( m_nJets != m_nAskedJets || m_nIsoLeps != m_nAskedIsoLeps ) {
    m_pTTree->Fill();
    return;
  }
 

  //&& m_nCorrectedSLD == m_nSLDecayTotal )	
  bool traceEvent = false;
  if ( pLCEvent->getEventNumber() == m_ievttrace || m_traceall ) traceEvent = true;
  
  std::vector< ReconstructedParticle* > Leptons{};
  for (int i_lep = 0; i_lep < m_nIsoLeps; i_lep++) {
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
  }
  
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||  KINFIT WITHOUT NEUTRINO COORECTION  ||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;

  FitResult woNuFitResult = performFIT( Jets, Leptons, traceEvent );
  BaseFitter* woNuFitter = woNuFitResult.fitter.get();
 
 streamlog_out(MESSAGE) << "Performed fit without neutrino correction" << endl;
  if (!woNuFitResult.fitter) {
    streamlog_out(MESSAGE) << "Did not find a functioning fit" << endl;
  } else {
  //Fill root branches
    m_FitErrorCode_woNu = woNuFitResult.fitter->getError();
    m_FitProbability_woNu = woNuFitResult.fitter->getProbability();
    m_FitChi2_woNu = woNuFitResult.fitter->getChi2();
    streamlog_out(MESSAGE) << "error code = " << woNuFitResult.fitter->getError() << endl;
    streamlog_out(MESSAGE) << "fit prob = " << woNuFitResult.fitter->getProbability() << endl;
    streamlog_out(MESSAGE) << "fit chi2 = " << woNuFitResult.fitter->getChi2()<< endl;
    streamlog_out(MESSAGE) << "Getting constraints now... ";
    auto constraints = woNuFitResult.constraints;
    streamlog_out(MESSAGE) << "Fitter contains " << constraints->size() << " constraints : ";
    for (auto it = constraints->begin(); it != constraints->end(); ++it) {
      streamlog_out(MESSAGE) << (*it)->getName() << " ";
      if (strcmp((*it)->getName(), "z mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_ZMassAfterFit_woNu = mc->getMass();
      }
      if (strcmp((*it)->getName(), "h1 mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_H1MassAfterFit_woNu = mc->getMass();
      }
      if (strcmp((*it)->getName(), "h2 mass")==0) {
	auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	m_H2MassAfterFit_woNu = mc->getMass();
      }
    }
    streamlog_out(MESSAGE) << endl;
    streamlog_out(MESSAGE) << "Z mass postfit = " << m_ZMassAfterFit_woNu << endl;
    streamlog_out(MESSAGE) << "H1 mass postfit = " << m_H1MassAfterFit_woNu << endl;
    streamlog_out(MESSAGE) << "H2 mass postfit = " << m_H2MassAfterFit_woNu << endl;
    
    streamlog_out(MESSAGE) << "Getting fitobjects now... ";
    auto fitobjects = woNuFitResult.fitobjects;
    //vector<unsigned int> perm_woNu;
    streamlog_out(MESSAGE) << "Fitter contains " << fitobjects->size() << " fitobjects : ";
    for (auto it = fitobjects->begin(); it != fitobjects->end(); ++it) {
      streamlog_out(MESSAGE) << (*it)->getName() << " ";
      //perm_woNu.pushback(((*it)->getName()).back())
    }
    streamlog_out(MESSAGE) << endl;
    for (int i = 0; i < m_nJets; ++i) {   
      string fitname = "jet"+to_string(i);
      auto fitjet = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitjet == fitobjects->end()) {
	streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
	continue;
      }
      std::shared_ptr<JetFitObject> castfitjet =  std::dynamic_pointer_cast<JetFitObject>(*fitjet);
      vector<double> pulls = calculatePulls(castfitjet, Jets[i], 1);
      m_pullJetEnergy_woNu.push_back(pulls[0]);
      m_pullJetTheta_woNu.push_back(pulls[1]);
      m_pullJetPhi_woNu.push_back(pulls[2]);
    }
    for (int i = 0; i < m_nIsoLeps; ++i) {   
      string fitname = "lepton"+to_string(i);
      auto fitlepton = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitlepton == fitobjects->end()) {
	streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
	continue;
      }
      std::shared_ptr<LeptonFitObject> castfitlepton =  std::dynamic_pointer_cast<LeptonFitObject>(*fitlepton);
      vector<double> pulls = calculatePulls(castfitlepton, Leptons[i], 2);
      m_pullLeptonInvPt_woNu.push_back(pulls[0]);
      m_pullLeptonTheta_woNu.push_back(pulls[1]);
      m_pullLeptonPhi_woNu.push_back(pulls[2]);
    }
  }
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||  FEED NEUTRINO COORECTION TO KINFIT  ||||||||||||||||||||||||||||" << std::endl ;
  streamlog_out(MESSAGE) << "	||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl ;
  
  //Vector of vectors for each SLD containings its corresponding neutrino-solutions
  //{{0,+,_,...}_SLD1, {{0,+,_,...}_SLD2}, ...}
  pfoVectorVector neutrinos1 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[0]); 
  pfoVectorVector neutrinos2 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[1]);
  pfoVectorVector neutrinos3 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[2]);
  pfoVectorVector neutrinos4 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[3]);
  pfoVectorVector bestNuSolutions = {};
  
  FitResult bestFitResult = woNuFitResult;

  pfoVector bestJets = {};
  if (woNuFitter && woNuFitter->getError()==0) {
    bestJets = {Jets[0], Jets[1], Jets[2], Jets[3]};
  };
  
  pfoVector gcJets;
  
  //For each jet loop over all possible combinations of the SLDcorrection sets
  //LOOP 1
  for(pfoVector nu1: combinations({}, neutrinos1, 0, {})) {
    ReconstructedParticle* cjet1 = addNeutrinoCorrection(Jets[0],nu1);
    gcJets.push_back(cjet1);
    //LOOP 2
    for(pfoVector nu2: combinations({}, neutrinos2, 0, {})) {
      ReconstructedParticle* cjet2 = addNeutrinoCorrection(Jets[1], nu2);
      gcJets.push_back(cjet2);
      //LOOP 3
      for(pfoVector nu3: combinations({}, neutrinos3, 0, {})) {
	ReconstructedParticle* cjet3 = addNeutrinoCorrection(Jets[2],nu3);
	gcJets.push_back(cjet3);
	//LOOP 4
	for(pfoVector nu4: combinations({}, neutrinos4, 0, {})) {
	  ReconstructedParticle* cjet4 = addNeutrinoCorrection(Jets[3],nu4);
	  gcJets.push_back(cjet4);
	  
	  std::vector< ReconstructedParticle* > CorrectedJets{cjet1, cjet2, cjet3, cjet4};
	  pfoVectorVector NuSolutions{nu1, nu2, nu3, nu4};
	  FitResult fitResult = performFIT( CorrectedJets, Leptons , traceEvent);
	  BaseFitter* fitter = fitResult.fitter.get();

	  
	  if (fitter && fitter->getError() == 0) {
	    // TODO the next line is just a test
	    streamlog_out(MESSAGE) << "   OUTER chi2 " << fitter->getChi2() << endl;
	    
	    for(auto it = fitResult.constraints->begin(); it != fitResult.constraints->end(); it++) {
	      streamlog_out(MESSAGE) << "   OUTER testing " << (*it)->getName() << endl;
	      if (strcmp((*it)->getName(), "h1 mass")==0) {
		auto mc = dynamic_pointer_cast<MassConstraint>(*it);
		streamlog_out(MESSAGE)<< "   OUTER higgs mass constraint: " << mc->getMass() << endl;
	      }
	    }
	    for(auto it = fitter->getFitObjects()->begin(); it != fitter->getFitObjects()->end(); it++) {
	      streamlog_out(MESSAGE)<< "   OUTER FITTER FO : " << (*it)->getName() << endl;
	    }
	    
	    // TODO the next line is just a test
	    for(auto it = fitResult.fitobjects->begin(); it != fitResult.fitobjects->end(); it++) streamlog_out(MESSAGE) << "   testing FO " << (*it)->getName() << endl;
	    if(!bestFitResult.fitter) {
	      bestFitResult = fitResult;
	      bestJets = CorrectedJets;
	      bestNuSolutions = NuSolutions;
	      continue;
	    }
	    if(fitter->getChi2() < bestFitResult.fitter->getChi2()) {
	      streamlog_out(MESSAGE)<< "   New fit result is better than stored! Store the new one instead " << endl;
	      bestFitResult = fitResult;
	      bestJets = CorrectedJets;
	      bestNuSolutions = NuSolutions;
	      streamlog_out(MESSAGE) << " BestFit has error code: " << bestFitResult.fitter->getError() << endl;
	    }
	  }
	}
      }
    }
  }
  /*
   * Clean up all jets we don't use for best solution
   * DISABLE THIS FOR NOW, AS DELETING ALSO DELETES MEMORY
   * FROM THE ORIGINAL JET WHICH UNDER SOME CIRCUMSTANCES
   * MIGHT BE IN USE
   */
  /*for(auto gcJet : gcJets) {
    bool isNeeded = false;
    for(auto bestJet : bestJets) {
      if(bestJet == gcJet) {
	isNeeded = true;
	continue;
      }
    }
    if(!isNeeded) delete gcJet;
    }*/

  streamlog_out(MESSAGE) << "Performed fit" << endl;
  if (!bestFitResult.fitter) {
    streamlog_out(MESSAGE) << "Did not find a functioning fit" << endl;
    m_pTTree->Fill();
    return;
  }
  
  //Fill root branches
  m_FitErrorCode = bestFitResult.fitter->getError();
  m_FitProbability = bestFitResult.fitter->getProbability();
  m_FitChi2 = bestFitResult.fitter->getChi2();
  streamlog_out(MESSAGE1) << "error code = " << bestFitResult.fitter->getError() << endl;
  streamlog_out(MESSAGE1) << "fit prob = " << bestFitResult.fitter->getProbability() << endl;
  streamlog_out(MESSAGE1) << "fit chi2 = " << bestFitResult.fitter->getChi2()<< endl;
  streamlog_out(MESSAGE) << "Getting constraints now... ";
  auto constraints = bestFitResult.constraints;
  streamlog_out(MESSAGE) << "Fitter contains " << constraints->size() << " constraints : ";
  for (auto it = constraints->begin(); it != constraints->end(); ++it) {
    streamlog_out(MESSAGE) << (*it)->getName() << " ";
    if (strcmp((*it)->getName(), "z mass")==0) {
      auto mc = dynamic_pointer_cast<MassConstraint>(*it);
      m_ZMassAfterFit = mc->getMass();
    }
    if (strcmp((*it)->getName(), "h1 mass")==0) {
      auto mc = dynamic_pointer_cast<MassConstraint>(*it);
      m_H1MassAfterFit = mc->getMass();
    }
    if (strcmp((*it)->getName(), "h2 mass")==0) {
      auto mc = dynamic_pointer_cast<MassConstraint>(*it);
      m_H2MassAfterFit = mc->getMass();
    }
  }
  streamlog_out(MESSAGE) << endl;

  streamlog_out(MESSAGE) << "Getting fitobjects now... ";
  auto fitobjects = bestFitResult.fitobjects;
  vector<unsigned int> perm;
  streamlog_out(MESSAGE1) << "Fitter contains " << fitobjects->size() << " fitobjects : ";
  for (auto it = fitobjects->begin(); it != fitobjects->end(); ++it) {
    streamlog_out(MESSAGE1) << (*it)->getName() << " ";
    perm.push_back((unsigned int)((string)(*it)->getName()).back()-48);
  }
  streamlog_out(MESSAGE1) << endl;

  streamlog_out(MESSAGE1) << "ladida checking permutations: "; 
  for (auto idx: perm) streamlog_out(MESSAGE1) << idx << " ";
  streamlog_out(MESSAGE1) << endl;

  vector<double> startmasses = calculateInitialMasses(bestJets, Leptons, perm);
  m_ZMassBeforeFit  = startmasses[0];
  m_H1MassBeforeFit = startmasses[1];
  m_H2MassBeforeFit = startmasses[2];

  streamlog_out(MESSAGE1) << "Z mass prefit = " << m_ZMassBeforeFit << endl;
  streamlog_out(MESSAGE1) << "H1 mass prefit = " << m_H1MassBeforeFit << endl;
  streamlog_out(MESSAGE1) << "H2 mass prefit = " << m_H2MassBeforeFit << endl;
  streamlog_out(MESSAGE1) << "Z mass postfit = " << m_ZMassAfterFit << endl;
  streamlog_out(MESSAGE1) << "H1 mass postfit = " << m_H1MassAfterFit << endl;
  streamlog_out(MESSAGE1) << "H2 mass postfit = " << m_H2MassAfterFit << endl;


  for (int i = 0; i < m_nJets; ++i) {   
    string fitname = "jet"+to_string(i);
    auto fitjet = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
    if (fitjet == fitobjects->end()) {
      streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
      continue;
    }
    std::shared_ptr<JetFitObject> castfitjet =  std::dynamic_pointer_cast<JetFitObject>(*fitjet);
    vector<double> pulls = calculatePulls(castfitjet, bestJets[i], 1);
    m_pullJetEnergy.push_back(pulls[0]);
    m_pullJetTheta.push_back(pulls[1]);
    m_pullJetPhi.push_back(pulls[2]);
  }
  for (int i = 0; i < m_nIsoLeps; ++i) {   
    string fitname = "lepton"+to_string(i);
    auto fitlepton = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
    if (fitlepton == fitobjects->end()) {
      streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
      continue;
    }
    std::shared_ptr<LeptonFitObject> castfitlepton =  std::dynamic_pointer_cast<LeptonFitObject>(*fitlepton);
    vector<double> pulls = calculatePulls(castfitlepton, Leptons[i], 2);
    m_pullLeptonInvPt.push_back(pulls[0]);
    m_pullLeptonTheta.push_back(pulls[1]);
    m_pullLeptonPhi.push_back(pulls[2]);
  }
  
  /*
    m_ZMassBeforeFit // "ZMassBeforeFit/F" );
    m_H1MassBeforeFit // "H1MassBeforeFit/F" );
    m_H2MassBeforeFit // "H2MassBeforeFit/F" );
  */
  streamlog_out(DEBUG) << "Pulls have been calculated" << std::endl;
  //Fill output collections
  for(unsigned int i = 0; i<bestJets.size(); i++) {
    dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setMomentum(bestJets[i]->getMomentum());
    dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setEnergy(bestJets[i]->getEnergy());
    dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setCovMatrix(bestJets[i]->getCovMatrix());
    outputJetCollection->addElement( Jets[i] );
  }

  for (auto neutrinos : bestNuSolutions) {
    for (auto nu : neutrinos) {
      std::pair<MCParticle*,ReconstructedParticle*> nupair = getMCNeutrino(NuMCNav, SLDNuNav, nu);
      m_TrueNeutrinoEnergy.push_back(nupair.first->getEnergy());
      m_RecoNeutrinoEnergy.push_back(nupair.second->getEnergy());
      m_RecoNeutrinoEnergyKinfit.push_back(nu->getEnergy());
      //LCFloatVec *NuEnergy = new LCFloatVec;
      //NuEnergy->push_back( mcnu->getEnergy() );
      //NuEnergy->push_back( nu->getEnergy() );
      //outputNuEnergyCollection->addElement(NuEnergy);
    }
  }
  streamlog_out(MESSAGE) << "Looped over neutrinos" << std::endl;

  pLCEvent->addCollection( outputJetCollection , m_outputJetCollection.c_str() );
  streamlog_out(DEBUG0) << " Output Jet collection added to event" << std::endl;
  //pLCEvent->addCollection( outputNuEnergyCollection, m_outputNuEnergyCollection.c_str() );
  //streamlog_out(DEBUG0) << " Output true and reco Nu collection added to event" << std::endl;
  m_pTTree->Fill();
}


ReconstructedParticle* ZHHllbbbbKinFit::addNeutrinoCorrection(ReconstructedParticle* jet, pfoVector neutrinos) {
  std::vector< float > jetCovMat = jet->getCovMatrix();
  ROOT::Math::PxPyPzEVector jetFourMomentum(jet->getMomentum()[0],jet->getMomentum()[1],jet->getMomentum()[2], jet->getEnergy());
  for(auto nu : neutrinos) {
    jetFourMomentum += ROOT::Math::PxPyPzEVector( nu->getMomentum()[0], nu->getMomentum()[1], nu->getMomentum()[2] , nu->getEnergy() );

    std::transform(jetCovMat.begin(), jetCovMat.end(), nu->getCovMatrix().begin(), 
		   jetCovMat.begin(), std::plus<float>());
  }
  ReconstructedParticleImpl* rp = new ReconstructedParticleImpl(*dynamic_cast<ReconstructedParticleImpl*>(jet));
  rp->setEnergy(jetFourMomentum.E());
  float jetThreeMomentum[3] = { jetFourMomentum.Px(), jetFourMomentum.Py(), jetFourMomentum.Pz()};
  rp->setMomentum(jetThreeMomentum);
  rp->setCovMatrix(jetCovMat);
  
  ReconstructedParticle* outrp = static_cast<ReconstructedParticle*>(rp);
  return outrp;
}

/*
 * Given a vector A of n vectors we want to return
 * all possible vectors of length n where the first
 * element is from A[0], the second from A[1] etc.
 */
std::vector<std::vector<EVENT::ReconstructedParticle*>> ZHHllbbbbKinFit::combinations(pfoVectorVector collector,
											     pfoVectorVector sets, 
											     int n,
											     pfoVector combo) {
  if (n == sets.size()) {
    collector.push_back({combo});
    return collector;
  }

  for (auto c : sets.at(n)) {
    combo.push_back(c);
    collector = combinations(collector, sets, n + 1, combo);
    combo.pop_back();
  }
  return collector;
}


std::vector<std::vector<EVENT::ReconstructedParticle*>> ZHHllbbbbKinFit::getNeutrinosInJet( LCRelationNavigator* JetSLDNav , 
												   LCRelationNavigator* SLDNuNav , 
												   EVENT::ReconstructedParticle* jet) {
  pfoVectorVector output;
  const EVENT::LCObjectVec& SLDVertices = JetSLDNav->getRelatedToObjects( jet );
  streamlog_out(MESSAGE) << "Number of SLD vertices: " << SLDVertices.size() << std::endl;
  for ( auto sldVertex : SLDVertices )
    {
      pfoVector neutrinosThisVertex;
      const EVENT::LCObjectVec& neutrinos = SLDNuNav->getRelatedToObjects( sldVertex );
      for(auto neutrino : neutrinos) {
	ReconstructedParticle* nu = static_cast<ReconstructedParticle*>(neutrino);
	neutrinosThisVertex.push_back(nu);
      }
      output.push_back(neutrinosThisVertex);
    }
  return output;
}

std::pair<MCParticle*,ReconstructedParticle*> ZHHllbbbbKinFit::getMCNeutrino(LCRelationNavigator* NuMCNav,
										    LCRelationNavigator* SLDNuNav,
										    EVENT::ReconstructedParticle* neutrino) {
  MCParticle* MCNu;
  ReconstructedParticle* BestNu;
  const EVENT::LCObjectVec& SLDVertex = SLDNuNav->getRelatedFromObjects( neutrino );
  streamlog_out(MESSAGE) << " Got SLDVertices " << SLDVertex.size() << std::endl;
  const EVENT::LCObjectVec& neutrinos = SLDNuNav->getRelatedToObjects( SLDVertex.at(0) );
  streamlog_out(MESSAGE) << " Got neutrinos " << neutrinos.size() << std::endl;
  const EVENT::LCObjectVec& MCP = NuMCNav->getRelatedToObjects( neutrinos.at(0) );
  streamlog_out(MESSAGE) << " Got MC particles " << MCP.size() << std::endl;
  MCNu = (MCParticle*) MCP.at(0);
  float chi2min = 9999999;
  for (auto nu : neutrinos) {
    float chi2 = (MCNu->getEnergy()-static_cast<ReconstructedParticle*>(nu)->getEnergy())*(MCNu->getEnergy()-static_cast<ReconstructedParticle*>(nu)->getEnergy());
    if (chi2<chi2min) {
      chi2min=chi2;
      BestNu = (ReconstructedParticle*) nu;
    }
  }
  return make_pair(MCNu,BestNu);
}

ZHHllbbbbKinFit::FitResult ZHHllbbbbKinFit::performFIT( pfoVector jets, 
								      pfoVector leptons,
								      bool traceEvent) {
  shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
  shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo= make_shared<vector<shared_ptr<LeptonFitObject>>>();
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////												  //////
  //////					Set JetFitObjects					  //////
  //////												  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
    streamlog_out(DEBUG6) << "get jet"<< i_jet+1 <<" parameters"  << std::endl ;
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getJetParameters( jets[ i_jet ] , parameters , errors );
    auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
    jfo->push_back(j);
    const string name = "jet"+to_string(i_jet);
    j->setName(name.c_str());
    streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << *j  << std::endl ;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //////							         				  //////
  //////					Set LeptonFitObjects					  //////
  //////												  //////
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  for (unsigned int i_lep =0; i_lep < leptons.size(); i_lep++) {
    streamlog_out(DEBUG6) << "get lepton"<< i_lep+1 <<" parameters"  << std::endl ;
    float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
    getLeptonParameters( leptons[ i_lep ] , parameters , errors );
    auto l = make_shared<LeptonFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , leptons[ i_lep ]->getMass() );
    lfo->push_back(l);
    const string name = "lepton"+to_string(i_lep);
    l->setName(name.c_str());
    streamlog_out(MESSAGE)  << " start four-vector of lepton"<< i_lep+1 <<": " << *l  << std::endl ;
  }
  
  const int NJETS = 4;
  const int NLEPTONS = 2;
  
  double bestProb = -1;
  double bestChi2 = 9999999999999.;
  FitResult bestFitResult;

  assert(NJETS==4);
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

  for (unsigned int iperm = 0; iperm < perms.size(); iperm++) {
    streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
    streamlog_out(MESSAGE) << " iperm = " << iperm << std::endl ;

    shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();
    shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo_perm = make_shared<vector<shared_ptr<LeptonFitObject>>>();

    // important: (re-)set fitjets array!                                                                                                       // keep track of newly created heap particles
    shared_ptr<vector<shared_ptr<BaseFitObject>>> fos = make_shared<vector<shared_ptr<BaseFitObject>>>();
    for(auto i : perms[iperm]) {
      streamlog_out(MESSAGE) << " Picking jet " << i << std::endl ;
      auto jsp = make_shared<JetFitObject>(*jfo->at(i));
      jfo_perm->push_back(jsp);
    }
    for(int i = 0; i < NLEPTONS; ++i) {
      auto lsp = make_shared<LeptonFitObject>(*lfo->at(i));
      lfo_perm->push_back(lsp);
    }
    for(auto j : *jfo_perm) fos->push_back(j);
    for(auto l : *lfo_perm) fos->push_back(l);

    for (auto j : *jfo_perm) 
      streamlog_out(DEBUG)  << "start four-vector of jet " << j->getName() << ": " << *j  << std::endl ;
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
    
    shared_ptr<MomentumConstraint> pyc = make_shared<MomentumConstraint>(0, 0, 1, 0, 0);
    pyc->setName("sum(p_y)");
    for (auto j : *jfo_perm) pyc->addToFOList(*j);
    for (auto l : *lfo_perm) pyc->addToFOList(*l);
    
    shared_ptr<MomentumConstraint> pzc = make_shared<MomentumConstraint>(0, 0, 0, 1, 0);
    pzc->setName("sum(p_z)");
    for (auto j : *jfo_perm) pzc->addToFOList(*j);
    for (auto l : *lfo_perm) pzc->addToFOList(*l);
    
    double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.); //TODO: check equation
    shared_ptr<MomentumConstraint> ec = make_shared<MomentumConstraint>(1, 0, 0, 0, E_lab);
    ec->setName("sum(E)");
    for (auto j : *jfo_perm) ec->addToFOList(*j);
    for (auto l : *lfo_perm) ec->addToFOList(*l);
    
    streamlog_out(DEBUG8)  << "	Value of E_lab before adding ISR: " << E_lab << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of target_p_due_crossing_angle before adding ISR: " << target_p_due_crossing_angle << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pxc before adding ISR: " << pxc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pyc before adding ISR: " << pyc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pzc before adding ISR: " << pzc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of ec before adding ISR: " << ec->getValue() << std::endl ;
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////													//////
    //////					Set ISR PhotonFitObjects					//////
    //////													//////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // TODO might have to make this into shared_ptr too to ensure cleanup
    ISRPhotonFitObject *photon = new ISRPhotonFitObject (0., 0., -pzc->getValue(), b, ISRPzMaxB);
    photon->setName("photon");
    if( m_fitISR ) {
      streamlog_out(MESSAGE)  << "start four-vector of ISR photon: " << *(photon) << std::endl ;
      pxc->addToFOList(*(photon));
      pyc->addToFOList(*(photon));
      pzc->addToFOList(*(photon));
      ec->addToFOList(*(photon));
    }
    streamlog_out(DEBUG8)  << "	Value of E_lab before fit: " << E_lab << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of target_p_due_crossing_angle before fit: " << target_p_due_crossing_angle << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pxc after adding ISR before fit: " << pxc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pyc after adding ISR before fit: " << pyc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of pzc after adding ISR before fit: " << pzc->getValue() << std::endl ;
    streamlog_out(DEBUG8)  << "	Value of ec after adding ISR before fit: " << ec->getValue() << std::endl ;
    
    //To be added to fit, depending on fit hypothesis
    shared_ptr<MassConstraint> h1m = make_shared<MassConstraint>(125.);
    h1m->addToFOList (*jfo_perm->at(0), 1);
    h1m->addToFOList (*jfo_perm->at(1), 1);
    h1m->setName("higgs1 mass");
    //shared_ptr<SoftGaussMassConstraint> h2m = make_shared<SoftGaussMassConstraint>(125.,3.); 
    shared_ptr<MassConstraint> h2m = make_shared<MassConstraint>(125.);
    h2m->addToFOList (*jfo_perm->at(2), 1);
    h2m->addToFOList (*jfo_perm->at(3), 1);
    h2m->setName("higgs2 mass");
    shared_ptr<SoftGaussMassConstraint> zmsoft = make_shared<SoftGaussMassConstraint>(2.4952/2,91.2); 
    zmsoft->addToFOList (*jfo_perm->at(2), 1);
    zmsoft->addToFOList (*jfo_perm->at(3), 1);
    zmsoft->setName("soft z mass");
    shared_ptr<MassConstraint> zm = make_shared<MassConstraint>(91.2);
    zm->addToFOList (*jfo_perm->at(2), 1);
    zm->addToFOList (*jfo_perm->at(3), 1);
    zm->setName("hard z mass");
    shared_ptr<MassConstraint> eqm = make_shared<MassConstraint>(0.);
    eqm->addToFOList (*jfo_perm->at(0), 1);
    eqm->addToFOList (*jfo_perm->at(1), 1);
    eqm->addToFOList (*jfo_perm->at(2), 2);
    eqm->addToFOList (*jfo_perm->at(3), 2);
    eqm->setName("equal mass");
    //Not part of fit hypothesis, added after fit:
    shared_ptr<MassConstraint> h1 = make_shared<MassConstraint>(125.);
    h1->addToFOList (*jfo_perm->at(0), 1);
    h1->addToFOList (*jfo_perm->at(1), 1);
    h1->setName("h1 mass");  
    shared_ptr<MassConstraint> h2 = make_shared<MassConstraint>(125.);
    h2->addToFOList (*jfo_perm->at(2), 1);
    h2->addToFOList (*jfo_perm->at(3), 1);
    h2->setName("h2 mass");
    shared_ptr<MassConstraint> z = make_shared<MassConstraint>(91.2);
    z->addToFOList(*lfo_perm->at(0), 1);
    z->addToFOList(*lfo_perm->at(1), 1);
    z->setName("z mass");  

    streamlog_out(DEBUG) << "start mass of Z  : " << z->getMass(1) << std::endl ;
    streamlog_out(DEBUG) << "start mass of H 1: " << h1->getMass(1) << std::endl ;
    streamlog_out(DEBUG) << "start mass of H 2: " << h2->getMass(1) << std::endl ;
    
    shared_ptr<BaseFitter> fitter = NULL;
    if ( m_fitter == 1 ) {
      fitter = shared_ptr<BaseFitter>(new NewFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewFitterGSL>(fitter))->setDebug( 4 );    
      streamlog_out(DEBUG6) << "fit using GSL Fitter"  << std::endl ;
    } else if ( m_fitter == 2 ) {
      fitter = shared_ptr<BaseFitter>(new NewtonFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<NewtonFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(DEBUG6) << "fit using Newton GSL Fitter"  << std::endl ;
    } else {
      ////		OPALFitter has no method setDebug !
      fitter = shared_ptr<BaseFitter>(new OPALFitterGSL());
      if ( traceEvent ) (dynamic_pointer_cast<OPALFitterGSL>(fitter))->setDebug( 4 );
      streamlog_out(DEBUG6) << "fit using OPAL GSL Fitter"  << std::endl ;
    }

    for(auto j : *jfo_perm) fitter->addFitObject(*j);
    for(auto l : *lfo_perm) fitter->addFitObject(*l);

    if( m_fitISR ) {
      fitter->addFitObject( *(photon) );
      streamlog_out(DEBUG8) << "ISR added to fit"  << std::endl ;
    }

    fitter->addConstraint( pxc.get() );
    fitter->addConstraint( pyc.get() );
    fitter->addConstraint( pzc.get() );
    fitter->addConstraint( ec.get() );
    if (m_fithypothesis == "MH") {
      fitter->addConstraint( h1m.get() );
    } else if (m_fithypothesis == "ZHH") {
      fitter->addConstraint( h1m.get() );
      fitter->addConstraint( h2m.get() );
    } else if (m_fithypothesis == "ZZH") {
      fitter->addConstraint( h1m.get() );
      fitter->addConstraint( zm.get() );
    } else if (m_fithypothesis == "ZZHsoft") {
      fitter->addConstraint( h1m.get() );
      fitter->addConstraint( zmsoft.get() );
    } else {
      fitter->addConstraint( eqm.get() );
    }

    streamlog_out(DEBUG8) << "constraints added to fit:" << std::endl ;
    auto fitconstraints = fitter->getConstraints();
    auto fitsoftconstraints = fitter->getSoftConstraints();
    for (auto it = fitconstraints->begin(); it != fitconstraints->end(); ++it) {
      streamlog_out(DEBUG8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl;
    }
    for (auto it = fitsoftconstraints->begin(); it != fitsoftconstraints->end(); ++it) {
      streamlog_out(DEBUG8) << (*it)->getName() << " constraint value = " << (*it)->getValue() << std::endl;
    }

    //perform fit: 
    streamlog_out(MESSAGE) << "chi2 before fit" << calcChi2(fos) << endl; 
    float fitProbability = fitter->fit();
    streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    streamlog_out(MESSAGE) << "chi2 after fit (from calcchi2)" << (dynamic_pointer_cast<NewFitterGSL>(fitter))->calcChi2() << endl; 
    streamlog_out(MESSAGE) << "chi2 after fit (from fitter)" << fitter->getChi2() << endl; 
    streamlog_out(MESSAGE) << "chi2 after fit (from fitobjects)" << calcChi2(fos) << endl; 
    fitter->addConstraint( h1.get() );
    fitter->addConstraint( h2.get() );
    fitter->addConstraint( z.get() );
    streamlog_out(MESSAGE) << "chi2 after adding helper constraints" << fitter->getChi2() << endl; 
    shared_ptr<vector<shared_ptr<BaseHardConstraint>>> constraints = make_shared<vector<shared_ptr<BaseHardConstraint>>>();
    constraints->push_back(z);
    constraints->push_back(pxc);
    constraints->push_back(pyc);
    constraints->push_back(pzc);
    constraints->push_back(ec);
    constraints->push_back(h1);
    constraints->push_back(h2);
    //constraints->push_back(h);

    streamlog_out(DEBUG8) << "helper constraints added"  << std::endl ;

    //---------------------------
    if(fitter->getError()==0) {
      if (fitter->getChi2() < bestChi2) {
      //if(fitProbability > bestProb) {
	streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " is better than " << bestProb << " use that one" << endl;
	streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << endl;
	streamlog_out(MESSAGE) << "error code: " << fitter->getError() << endl;
	bestProb = fitProbability;
	bestChi2 = fitter->getChi2();

	FitResult fitresult(fitter, constraints, fos);
	for(auto it = fitresult.constraints->begin(); it != fitresult.constraints->end(); it++) {
	  streamlog_out(MESSAGE) << "   testing " << (*it)->getName() << endl;
	  if (strcmp((*it)->getName(), "h1 mass")==0) {
	    auto mc = dynamic_pointer_cast<MassConstraint>(*it);
	    streamlog_out(MESSAGE)<< "   higgs mass constraint: " << mc->getMass() << endl;
	  }
	}
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
  streamlog_out(MESSAGE) << "Converged on best fit with probability " << bestProb << endl;
  
  return bestFitResult;
}


void ZHHllbbbbKinFit::getJetParameters(	ReconstructedParticle* jet, float (&parameters)[3], float (&errors)[3])
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

void ZHHllbbbbKinFit::getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] )
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

std::vector<double> ZHHllbbbbKinFit::calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm)
{
  std::vector<double> masses;
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
  for (auto i = perm.begin(); i != perm.begin()+4 ; ++i) {
    //for(auto i : perm) {
    streamlog_out(MESSAGE) << " Picking jet for start masses " << *i << std::endl ;
    auto jsp = make_shared<JetFitObject>(*jfo->at(*i));
    jfo_perm->push_back(jsp);
  }
    shared_ptr<MassConstraint> h1 = make_shared<MassConstraint>(125.);
    h1->addToFOList (*jfo_perm->at(0), 1);
    h1->addToFOList (*jfo_perm->at(1), 1);
    h1->setName("h1 mass");  
    shared_ptr<MassConstraint> h2 = make_shared<MassConstraint>(125.);
    h2->addToFOList (*jfo_perm->at(2), 1);
    h2->addToFOList (*jfo_perm->at(3), 1);
    h2->setName("h2 mass");
    shared_ptr<MassConstraint> z = make_shared<MassConstraint>(91.2);
    z->addToFOList(*lfo->at(0), 1);
    z->addToFOList(*lfo->at(1), 1);
    z->setName("z mass");  
    masses.push_back(z->getMass(1));
    masses.push_back(h1->getMass(1));
    masses.push_back(h2->getMass(1));

    return masses;
}

std::vector<double> ZHHllbbbbKinFit::calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type)
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
    streamlog_out(DEBUG3) << "fitted = " << fitted << ", start = " << start << std::endl ;
    streamlog_out(DEBUG3) << "errfit = " << errfit << ", errmea = " << errmea << ", sigma = " << sigma << std::endl ;
    if (sigma > 0) {
      sigma = sqrt(sigma);
      pulls.push_back((fitted - start)/sigma);
    }
    else {
      streamlog_out(DEBUG3) << "NOT GOOD...................." << std::endl ;
      pulls.push_back(-4.5);
    }
  }
  return pulls;
}

double ZHHllbbbbKinFit::calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects) {
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

void ZHHllbbbbKinFit::check( LCEvent* )
{
//	nothing to check here - could be used to fill checkplots in reconstruction processor
}


void ZHHllbbbbKinFit::end()
{
//	streamlog_out(MESSAGE) << "# of events: " << m_nEvt << std::endl;
//	streamlog_out(ERROR) << "# of nucorrection: " << correction<< std::endl;
//	streamlog_out(ERROR) << "# of Covariance failed: " << nCo<< std::endl;
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;

}
