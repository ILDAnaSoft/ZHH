#include "ZHHqqbbbbKinFit.h"

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

ZHHqqbbbbKinFit aZHHqqbbbbKinFit ;

ZHHqqbbbbKinFit::ZHHqqbbbbKinFit(): ZHHBaseKinfitProcessor("ZHHqqbbbbKinFit") {
	_description = "ZHHqqbbbbKinFit does a kinematic fit on 6 jet events";

  registerProcessorParameter("nJets",
    "Number of jet should be in the event",
    m_nAskedJets,
    int(6)
  );

  registerProcessorParameter("nIsoLeps",
    "Number of Isolated Leptons should be in the event",
    m_nAskedIsoLeps,
    int(0)
  );
}

void ZHHqqbbbbKinFit::initChannelValues()
{
  assignPermutations(6, m_fithypothesis);

	streamlog_out(DEBUG) << "   init finished  " << std::endl;

}

void ZHHqqbbbbKinFit::clearChannelValues() {
  // must be changed when properties are added which do not appear in base;
}

void ZHHqqbbbbKinFit::updateChannelValues( EVENT::LCEvent *pLCEvent )
{  
  LCCollection *inputJetCollection = NULL;
  LCCollection *inputLeptonCollection = NULL;
  LCCollection *inputSLDecayCollection = NULL;
  LCCollection *inputMCParticleCollection = NULL;

  /*
  IMPL::LCCollectionVec* outputLeptonCollection = NULL;
  outputLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  IMPL::LCCollectionVec* outputStartLeptonCollection = NULL;
  outputStartLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  IMPL::LCCollectionVec* outputStartJetCollection = NULL;
  outputStartJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  //LCCollectionVec *outputNuEnergyCollection = new LCCollectionVec(LCIO::LCFLOATVEC);
  */

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
    streamlog_out(DEBUG0) << "  getting mc particle collection: " << _MCParticleColllectionName << std::endl ;
    inputMCParticleCollection = pLCEvent->getCollection( _MCParticleColllectionName );
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
  //HH mass in hard process
  std::vector<MCParticle*> mchiggs{};
  for (int i = 0; i < inputMCParticleCollection->getNumberOfElements(); ++i) {
    MCParticle *mcp = dynamic_cast<EVENT::MCParticle*>(inputMCParticleCollection->getElementAt(i));
    if (mcp->getPDG() == 25) mchiggs.push_back(mcp);
  }
  if (mchiggs.size() == 2) m_System23MassHardProcess = inv_mass(mchiggs.at(0), mchiggs.at(1)); 
  else streamlog_out(WARNING) << "////////////////////////////////////////////////// MC Higgs pair not found //////////////////////////////////////////////////" << endl;

  if ( m_nJets != m_nAskedJets || m_nIsoLeps != m_nAskedIsoLeps ) {
    streamlog_out(MESSAGE) << "Skipping event: Jets(Actual:Required)=(" << m_nJets << " : " << m_nAskedJets << ") |  ISOLeptons(Actual:Required)=(" << m_nIsoLeps << ":" << m_nAskedIsoLeps << ")" << std::endl;
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

  FitResult woNuFitResult = performFIT( Jets, Leptons, traceEvent, true );
  BaseFitter* woNuFitter = woNuFitResult.fitter.get();
 
 streamlog_out(MESSAGE) << "Performed fit without neutrino correction" << endl;
  if (!woNuFitResult.fitter) {
    streamlog_out(MESSAGE) << "Did not find a functioning fit" << endl;
  } else {
    //Fill root branches
    streamlog_out(MESSAGE) << "Getting fitobjects now... ";
    auto fitobjects_woNu = woNuFitResult.fitobjects;
    vector<unsigned int> perm_woNu;
    streamlog_out(MESSAGE) << "Fitter contains " << fitobjects_woNu->size() << " fitobjects : ";
    for (auto it = fitobjects_woNu->begin(); it != fitobjects_woNu->end(); ++it) {
      streamlog_out(MESSAGE) << (*it)->getName() << " ";
      perm_woNu.push_back((unsigned int)((string)(*it)->getName()).back()-48);
    }
    streamlog_out(MESSAGE) << endl;

    vector<double> startmasses_woNu = calculateInitialMasses(Jets, Leptons, perm_woNu);
    m_Boson1BeforeFit_woNu  = startmasses_woNu[0];
    m_Boson2BeforeFit_woNu = startmasses_woNu[1];
    m_Boson3BeforeFit_woNu = startmasses_woNu[2];
    m_System23MassBeforeFit_woNu = startmasses_woNu[3];
    m_System123MassBeforeFit_woNu = startmasses_woNu[4];

    streamlog_out(MESSAGE1) << "Boson1 mass prefit = " << m_Boson1BeforeFit_woNu << endl;
    streamlog_out(MESSAGE1) << "Boson2 mass prefit = " << m_Boson2BeforeFit_woNu << endl;
    streamlog_out(MESSAGE1) << "Boson3 mass prefit = " << m_Boson3BeforeFit_woNu << endl;
    streamlog_out(MESSAGE1) << "System23 mass prefit = " << m_System23MassBeforeFit_woNu << endl;
    streamlog_out(MESSAGE1) << "System123 mass prefit = " << m_System123MassBeforeFit_woNu << endl;
    streamlog_out(MESSAGE1) << "Boson1 mass postfit = " << m_Boson1AfterFit_woNu << endl;
    streamlog_out(MESSAGE1) << "Boson2 mass postfit = " << m_Boson2AfterFit_woNu << endl;
    streamlog_out(MESSAGE1) << "Boson3 mass postfit = " << m_Boson3AfterFit_woNu << endl;
    streamlog_out(MESSAGE1) << "System23 mass postfit = " << m_System23MassAfterFit_woNu << endl;
    streamlog_out(MESSAGE1) << "System123 mass postfit = " << m_System123MassAfterFit_woNu << endl;
    
    for (int i = 0; i < m_nJets; ++i) {   
      string fitname = "jet"+to_string(i);
      auto fitjet = find_if(fitobjects_woNu->begin(), fitobjects_woNu->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitjet == fitobjects_woNu->end()) {
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
      auto fitlepton = find_if(fitobjects_woNu->begin(), fitobjects_woNu->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
      if (fitlepton == fitobjects_woNu->end()) {
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
  pfoVectorVector neutrinos5 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[4]);
  pfoVectorVector neutrinos6 = getNeutrinosInJet(JetSLDNav , SLDNuNav , Jets[5]);
  pfoVectorVector bestNuSolutions = {};
  
  FitResult bestFitResult = woNuFitResult;

  pfoVector bestJets = {};
  if (woNuFitter && woNuFitter->getError()==0) {
    bestJets = {Jets[0], Jets[1], Jets[2], Jets[3], Jets[4], Jets[5]};
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
	  //LOOP 5
	  for(pfoVector nu5: combinations({}, neutrinos5, 0, {})) {
	    ReconstructedParticle* cjet5 = addNeutrinoCorrection(Jets[4],nu5);
	    gcJets.push_back(cjet5);
	    //LOOP 6
	    for(pfoVector nu6: combinations({}, neutrinos6, 0, {})) {
	      ReconstructedParticle* cjet6 = addNeutrinoCorrection(Jets[5],nu6);
	      gcJets.push_back(cjet6);
	      
	      std::vector< ReconstructedParticle* > CorrectedJets{cjet1, cjet2, cjet3, cjet4, cjet5, cjet6};
	      pfoVectorVector NuSolutions{nu1, nu2, nu3, nu4, nu5, nu6};
	      FitResult fitResult = performFIT( CorrectedJets, Leptons , traceEvent, false);
	      BaseFitter* fitter = fitResult.fitter.get();
	      
	      
	      if (fitter && fitter->getError() == 0) {
		// TODO the next line is just a test
		streamlog_out(MESSAGE) << "   OUTER chi2 " << fitter->getChi2() << endl;
		/*
		  for(auto it = fitResult.constraints->begin(); it != fitResult.constraints->end(); it++) {
		    streamlog_out(MESSAGE) << "   OUTER testing " << (*it)->getName() << endl;
		    if (strcmp((*it)->getName(), "h1 mass")==0) {
		    auto mc = dynamic_pointer_cast<MassConstraint>(*it);
		    streamlog_out(MESSAGE)<< "   OUTER higgs mass constraint: " << mc->getMass() << endl;
		    }
		    }*/
		/*for(auto it = fitter->getFitObjects()->begin(); it != fitter->getFitObjects()->end(); it++) {
		  streamlog_out(MESSAGE)<< "   OUTER FITTER FO : " << (*it)->getName() << endl;
		  }
		*/
		// TODO the next line is just a test
		/*for(auto it = fitResult.fitobjects->begin(); it != fitResult.fitobjects->end(); it++) streamlog_out(MESSAGE) << "   testing FO " << (*it)->getName() << endl;*/
		if(!bestFitResult.fitter || fitter->getChi2() < bestFitResult.fitter->getChi2()) {
		  streamlog_out(MESSAGE)<< "   New fit result is better than stored! Store the new one instead " << endl;
		  bestFitResult = fitResult;
		  bestJets = CorrectedJets;
		  bestNuSolutions = NuSolutions;
		  streamlog_out(MESSAGE) << " BestFit has error code: " << bestFitResult.fitter->getError() << endl;
		}
	      }
	    }//LOOP 6
	  }//LOOP 5
	}//LOOP 4
      }//LOOP 3
    }//LOOP 2
  }//LOOP 1
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
    
    vector<double> startmasses;
    std::tie(startmasses, m_FitChi2_byMass, m_bestMatchingByMass) = calculateMassesFromSimpleChi2Pairing(Jets, Leptons);

    m_Boson1BeforeFit  = startmasses[0];
    m_Boson2BeforeFit = startmasses[1];
    m_Boson3BeforeFit = startmasses[2];
    m_System23MassBeforeFit = startmasses[3];
    m_System123MassBeforeFit = startmasses[4];
    streamlog_out(MESSAGE) << "masses from simple chi2:" << m_Boson1BeforeFit << ", " << m_Boson2BeforeFit << ", " << m_Boson3BeforeFit << ", " << m_System23MassBeforeFit << ", " << m_System123MassBeforeFit << std::endl ; 

    m_pTTree->Fill();
    attachBestPermutation(inputJetCollection, m_bestMatchingByMass, "qq", false);
    return;
  }
  for (unsigned int i_jet =0; i_jet < bestJets.size(); i_jet++) {
    streamlog_out(MESSAGE)  << "After fit four-vector of jet"<< i_jet+1 <<": " << "[" << bestJets[ i_jet ]->getMomentum()[0] << ", " << bestJets[ i_jet]->getMomentum()[1] << ", " << bestJets[ i_jet ]->getMomentum()[2] << ", " << bestJets[ i_jet ]->getEnergy() << "]" << std::endl ;
  }
  //Fill root branches
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
  m_Boson1BeforeFit = startmasses[0];
  m_Boson2BeforeFit = startmasses[1];
  m_Boson3BeforeFit = startmasses[2];
  m_System23MassBeforeFit = startmasses[3];
  m_System123MassBeforeFit = startmasses[4];

  streamlog_out(MESSAGE1) << "Boson1 mass prefit = " << m_Boson1BeforeFit << endl;
  streamlog_out(MESSAGE1) << "Boson2 mass prefit = " << m_Boson2BeforeFit << endl;
  streamlog_out(MESSAGE1) << "Boson3 mass prefit = " << m_Boson3BeforeFit << endl;
  streamlog_out(MESSAGE1) << "System23 mass prefit = " << m_System23MassBeforeFit << endl;
  streamlog_out(MESSAGE1) << "System123 mass prefit = " << m_System123MassBeforeFit << endl;
  streamlog_out(MESSAGE1) << "Boson1 mass postfit = " << m_Boson1AfterFit << endl;
  streamlog_out(MESSAGE1) << "Boson2 mass postfit = " << m_Boson2AfterFit << endl;
  streamlog_out(MESSAGE1) << "Boson3 mass postfit = " << m_Boson3AfterFit << endl;
  streamlog_out(MESSAGE1) << "System23 mass postfit = " << m_System23MassAfterFit << endl;
  streamlog_out(MESSAGE1) << "System123 mass postfit = " << m_System123MassAfterFit << endl;

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
    string fitname = "jet"+to_string(i);
    auto fitjet = find_if(fitobjects->begin(), fitobjects->end(), [&fitname](const std::shared_ptr<BaseFitObject> obj) {return obj->getName() == fitname;});
    if (fitjet == fitobjects->end()) {
      streamlog_out(MESSAGE) << "Did not find " << fitname <<endl;
      continue;
    }
    std::shared_ptr<JetFitObject> castfitjet =  std::dynamic_pointer_cast<JetFitObject>(*fitjet);
    vector<double> pulls = calculatePulls(castfitjet, bestJets[i], 1);

    m_v4_postfit_jets.push_back(ROOT::Math::PxPyPzEVector(castfitjet->getPx(), castfitjet->getPy(), castfitjet->getPz(), castfitjet->getE()));
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

    m_v4_postfit_leptons.push_back(ROOT::Math::PxPyPzEVector(castfitlepton->getPx(), castfitlepton->getPy(), castfitlepton->getPz(), castfitlepton->getE()));
    m_pullLeptonInvPt.push_back(pulls[0]);
    m_pullLeptonTheta.push_back(pulls[1]);
    m_pullLeptonPhi.push_back(pulls[2]);
  }
  
  /*
    m_Boson1BeforeFit // "Boson1BeforeFit/F" );
    m_Boson2BeforeFit // "Boson2BeforeFit/F" );
    m_Boson3BeforeFit // "Boson3BeforeFit/F" );
  */
  streamlog_out(DEBUG) << "Pulls have been calculated" << std::endl;
  //Fill output collections
  //TO DO: DO NOT USE Jets VECTOR SINCE IT POINTS TO RECONSTRUCTED PARTICLES AND OVERWRITES THEM FOR NEXT KINFIT!!!!!!!!
  //for(unsigned int i = 0; i<bestJets.size(); i++) {
  //  dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setMomentum(bestJets[i]->getMomentum());
  //  dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setEnergy(bestJets[i]->getEnergy());
  //  dynamic_cast<ReconstructedParticleImpl*>(Jets[i])->setCovMatrix(bestJets[i]->getCovMatrix());
  //  outputJetCollection->addElement( Jets[i] );
  //}
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

  //pLCEvent->addCollection( outputJetCollection , m_outputJetCollection.c_str() );
  //streamlog_out(DEBUG0) << " Output Jet collection added to event" << std::endl;
  //pLCEvent->addCollection( outputNuEnergyCollection, m_outputNuEnergyCollection.c_str() );
  //streamlog_out(DEBUG0) << " Output true and reco Nu collection added to event" << std::endl;
  m_pTTree->Fill();
  attachBestPermutation(inputJetCollection, m_bestMatchingKinfit, "qq", true);
  fillOutputCollections(pLCEvent);
}

std::tuple<std::vector<double>, double, std::vector<unsigned short>>
  ZHHqqbbbbKinFit::calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons) 
{
  (void) leptons;

  std::vector<double> masses (5, 0);
  std::vector<unsigned short> bestperm;

  std::vector<ROOT::Math::PxPyPzEVector> fourVecs = v4(jets);
  ROOT::Math::PxPyPzEVector zhhFourMomentum(0.,0.,0.,0.);
  for (auto jet_v4 : fourVecs)
    zhhFourMomentum += jet_v4;
  double zhh = zhhFourMomentum.M();

  // TODO: use b-tagging information; e.g. use only 4 out of 6 jets with highest b-tag
  float chi2min = 99999.;
  std::vector<float> dijet_masses;

  if (m_nDijets == 3) {
    std::tie(dijet_masses, chi2min, bestperm) = simpleChi2Pairing(jets);

    masses[0] = dijet_masses[0];
    masses[1] = dijet_masses[1];
    masses[2] = dijet_masses[2];
    masses[3] = (fourVecs[bestperm[0]] + fourVecs[bestperm[1]] + fourVecs[bestperm[2]] + fourVecs[bestperm[3]]).M();
  }
  masses[4] = zhh;

  streamlog_out(MESSAGE) << "masses from simple chi2:" << masses[0] << ", " << masses[1] << ", " << masses[2] << ", " << zhh << std::endl ;

  return std::make_tuple(masses, chi2min, bestperm);
}