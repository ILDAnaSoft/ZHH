#include "ZHHBaseKinfitProcessor.h"
#include "ZinvisibleFitObject.h"

ZHHBaseKinfitProcessor::ZHHBaseKinfitProcessor(const std::string& name) : Processor(name),
m_pTFile(NULL),
MODE_IS_NMC(false),
MODE_IS_ZHH(false),
MODE_IS_ZZH(false),
MODE_IS_ZZZ(false),
MODE_IS_ZZHsoft(false),
MODE_IS_MH(false),
MODE_IS_EQM(false),
m_nRun(0),
m_nEvt(0),
m_nRunSum(0),
m_nEvtSum(0),
m_Bfield(0.0),
c(0.0),
mm2m(0.0),
eV2GeV(0.0),
eB(0.0) {
  //	register steering parameters: name, description, class-variable, default value
  registerProcessorParameter(	"treeName",
    "name of the output ROOT ttree. useful when multiple KinFitProcessors are used",
    m_ttreeName,
    name
  );

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
					"isolatedleptonCollection" ,
					"Name of the Isolated Lepton collection"  ,
					m_inputIsolatedleptonCollection ,
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
          float(1.0f));

	registerProcessorParameter("SigmaEnergyScaleFactor" ,
					"Factor for scaling up energy error",
					m_SigmaEnergyScaleFactor,
					float(1.0f)
				);

	registerProcessorParameter("SigmaAnglesScaleFactor" ,
				        "Factor for scaling up angular errors",
				        m_SigmaAnglesScaleFactor,
				        float(1.0f));

	registerProcessorParameter("includeISR",
					"Include ISR in fit hypothesis; false: without ISR , true: with ISR",
					m_fitISR,
					bool(true)
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

	// Outputs: Fitted jet and leptons and their prefit counterparts, the neutrino correction, and pulls
	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
					"outputLeptonCollection" ,
					"Name of output fitted lepton collection"  ,
					m_outputLeptonCollection ,
					std::string("ISOLeptons_KinFit")
				);

	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
					"outputJetCollection",
					"Name of output fitted jet collection",
					m_outputJetCollection,
					std::string("Durham_4JetsKinFit")
				);
	
	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
					"outputStartLeptonCollection" ,
					"Name of output prefit lepton collection"  ,
					m_outputStartLeptonCollection ,
					std::string("ISOLeptons_PreFit")
				);

	registerOutputCollection(LCIO::RECONSTRUCTEDPARTICLE,
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

std::pair<MCParticle*,ReconstructedParticle*> ZHHBaseKinfitProcessor::getMCNeutrino(LCRelationNavigator* NuMCNav,
										    LCRelationNavigator* SLDNuNav,
										    EVENT::ReconstructedParticle* neutrino) {
  MCParticle* MCNu;
  ReconstructedParticle* BestNu = NULL;
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
  assert(BestNu != NULL);
  return std::make_pair(MCNu,BestNu);
}

pfoVectorVector ZHHBaseKinfitProcessor::getNeutrinosInJet( LCRelationNavigator* JetSLDNav , 
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

void ZHHBaseKinfitProcessor::getJetParameters(	ReconstructedParticle* jet, float (&parameters)[3], float (&errors)[3])
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

void ZHHBaseKinfitProcessor::getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] )
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

ReconstructedParticle* ZHHBaseKinfitProcessor::addNeutrinoCorrection(ReconstructedParticle* jet, pfoVector neutrinos) {
  std::vector< float > jetCovMat = jet->getCovMatrix();
  ROOT::Math::PxPyPzEVector jetFourMomentum(jet->getMomentum()[0],jet->getMomentum()[1],jet->getMomentum()[2], jet->getEnergy());
  for(auto nu : neutrinos) {
    jetFourMomentum += ROOT::Math::PxPyPzEVector( nu->getMomentum()[0], nu->getMomentum()[1], nu->getMomentum()[2] , nu->getEnergy() );

    std::transform(jetCovMat.begin(), jetCovMat.end(), nu->getCovMatrix().begin(), 
		   jetCovMat.begin(), std::plus<float>());
  }
  ReconstructedParticleImpl* rp = new ReconstructedParticleImpl(*dynamic_cast<ReconstructedParticleImpl*>(jet));
  rp->setEnergy(jetFourMomentum.E());
  float jetThreeMomentum[3] = { (float)jetFourMomentum.Px(), (float)jetFourMomentum.Py(), (float)jetFourMomentum.Pz()};
  rp->setMomentum(jetThreeMomentum);
  rp->setCovMatrix(jetCovMat);
  
  ReconstructedParticle* outrp = static_cast<ReconstructedParticle*>(rp);
  return outrp;
}

std::vector<double> ZHHBaseKinfitProcessor::calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type)
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

double ZHHBaseKinfitProcessor::calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects) {
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

/*
 * Given a vector A of n vectors we want to return
 * all possible vectors of length n where the first
 * element is from A[0], the second from A[1] etc.
 */
std::vector<std::vector<EVENT::ReconstructedParticle*>> ZHHBaseKinfitProcessor::combinations(pfoVectorVector collector,
											     pfoVectorVector sets, 
											     int n,
											     pfoVector combo) {
  if (n == (int)sets.size()) {
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

void ZHHBaseKinfitProcessor::end()
{
//	streamlog_out(MESSAGE) << "# of events: " << m_nEvt << std::endl;
//	streamlog_out(ERROR) << "# of nucorrection: " << correction<< std::endl;
//	streamlog_out(ERROR) << "# of Covariance failed: " << nCo<< std::endl;
  if (m_pTFile != NULL) {
    m_pTFile->cd();
  }
  m_pTTree->Write();

  if (m_pTFile != NULL) {
    m_pTFile->Close();
    delete m_pTFile;
  }
}

void ZHHBaseKinfitProcessor::assignPermutations(size_t njets, string fithypothesis) {
  if (njets == 4) {
    if (fithypothesis == "ZZH" || fithypothesis == "ZZHsoft" || fithypothesis == "MH") {
      perms = {
        {0, 1, 2, 3},
        {0, 2, 1, 3},
        {0, 3, 1, 2},
        {1, 2, 0, 3},
        {1, 3, 0, 2},
        {2, 3, 0, 1}
      };
    } else if (fithypothesis == "ZHH" || fithypothesis == "EQM") {
      perms = {
        {0, 1, 2, 3}, 
        {0, 2, 1, 3}, 
        {0, 3, 1, 2}
      };
    } else {
      perms = {{0, 1, 2, 3}};
    }
  } else if (njets == 6) {
    if (fithypothesis == "ZZH" || fithypothesis == "ZZHsoft" || fithypothesis == "MH" || fithypothesis == "ZHH" || fithypothesis == "EQM") {
      //TO DO: redo permutations for each hypothesis
      perms = {
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
    } else {
      perms = {{0, 1, 2, 3, 4, 5}};
    }
  } else
    throw EVENT::Exception("Only njets=4 and njets=6 are implemented");
};


void ZHHBaseKinfitProcessor::attachBestPermutation(LCCollection *jets, vector<unsigned short> bestperm, string parameterSuffix, bool fromKinfit) {
  const EVENT::IntVec intvec(bestperm.begin(), bestperm.end());

  jets->parameters().setValues(std::string("best_perm_").append(parameterSuffix), intvec);
  jets->parameters().setValue(std::string("best_perm_").append(parameterSuffix).append("_from_kinfit"), fromKinfit ? 2 : 1);
};

// returns the jets in the same order as they appeared in the source collection
void ZHHBaseKinfitProcessor::fillOutputCollections(
  EVENT::LCEvent *pLCEvent
) {
  IMPL::LCCollectionVec* outputJetCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
  IMPL::LCCollectionVec* outputLeptonCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);

  float momentum [3];
  for(unsigned int i = 0; i < m_v4_postfit_jets.size(); i++) {
    ReconstructedParticleImpl * newJet = new ReconstructedParticleImpl();

    momentum[0] = m_v4_postfit_jets[i].Px();
    momentum[1] = m_v4_postfit_jets[i].Py();
    momentum[2] = m_v4_postfit_jets[i].Pz();

    newJet->setMomentum(momentum);
    newJet->setEnergy(m_v4_postfit_jets[i].E());
    //newJet->setCovMatrix(bestjets[i]->getCovMatrix());

    outputJetCollection->addElement( newJet );
  }

  for(unsigned int i = 0; i < m_v4_postfit_leptons.size(); i++) {
    ReconstructedParticleImpl * newLep = new ReconstructedParticleImpl();

    momentum[0] = m_v4_postfit_leptons[i].Px();
    momentum[1] = m_v4_postfit_leptons[i].Py();
    momentum[2] = m_v4_postfit_leptons[i].Pz();

    newLep->setMomentum(momentum);
    newLep->setEnergy(m_v4_postfit_leptons[i].E());

    outputLeptonCollection->addElement( newLep );
  }

  pLCEvent->addCollection( outputJetCollection , m_outputJetCollection.c_str() );
  pLCEvent->addCollection( outputLeptonCollection , m_outputLeptonCollection.c_str() );
};

void ZHHBaseKinfitProcessor::clearBaseValues() {
	m_v4_postfit_jets.clear();
	m_v4_postfit_leptons.clear();

  	m_nJets = 0;
	m_nIsoLeps = 0;
	m_nSLDecayBHadron = 0;
	m_nSLDecayCHadron = 0;
	m_nSLDecayTauLepton = 0;
	m_nSLDecayTotal = 0;
	m_nCorrectedSLD = 0;
	m_ISREnergyTrue = 0.0;
	m_BSEnergyTrue = 0.0;
	m_System23MassHardProcess = 0.0;

	m_FitErrorCode_woNu = 1;
	m_Boson1BeforeFit_woNu = 0.0;
	m_Boson2BeforeFit_woNu = 0.0;
	m_Boson3BeforeFit_woNu = 0.0;
	m_System23MassBeforeFit_woNu = 0.0;
	m_System123MassBeforeFit_woNu = 0.0;
	m_ISREnergyBeforeFit_woNu = 0.0;
	m_p1stBeforeFit_woNu = 0.0;
	m_cos1stBeforeFit_woNu = 0.0;

	m_Boson1AfterFit_woNu = 0.0;
	m_Boson2AfterFit_woNu = 0.0;
	m_Boson3AfterFit_woNu = 0.0;
	m_System23MassAfterFit_woNu = 0.0;
	m_System123MassAfterFit_woNu = 0.0;
	m_ISREnergyAfterFit_woNu = 0.0;
	m_p1stAfterFit_woNu = 0.0;
	m_cos1stAfterFit_woNu = 0.0;
	m_FitProbability_woNu = 0.0;
	m_FitChi2_woNu = 0.0;
	m_FitChi2_byMass = 0.0;

	m_bestMatchingKinfit.clear();
	m_bestMatchingByMass.clear();
	m_pullJetEnergy_woNu.clear();
	m_pullJetTheta_woNu.clear();
	m_pullJetPhi_woNu.clear();
	m_pullLeptonInvPt_woNu.clear();
	m_pullLeptonTheta_woNu.clear();
	m_pullLeptonPhi_woNu.clear();
	/*m_FitErrorCode_wNu = 1;
	m_Boson1BeforeFit_wNu = 0.0;
	m_Boson2BeforeFit_wNu = 0.0;
	m_Boson3BeforeFit_wNu = 0.0;
	m_Boson1AfterFit_wNu = 0.0;
	m_Boson2AfterFit_wNu = 0.0;
	m_Boson3AfterFit_wNu = 0.0;
	m_FitProbability_wNu = 0.0;
	m_FitChi2_wNu = 0.0;
	m_pullJetEnergy_wNu.clear();
	m_pullJetTheta_wNu.clear();
	m_pullJetPhi_wNu.clear();
	m_pullLeptonInvPt_wNu.clear();
	m_pullLeptonTheta_wNu.clear();
	m_pullLeptonPhi_wNu.clear();*/

	m_FitErrorCode = 1;
	m_Boson1BeforeFit = 0.0;
	m_Boson2BeforeFit = 0.0;
	m_Boson3BeforeFit = 0.0;
	m_System23MassBeforeFit = 0.0;
	m_System123MassBeforeFit = 0.0;
	m_ISREnergyBeforeFit = 0.0;
	m_p1stBeforeFit = 0.0;
	m_cos1stBeforeFit = 0.0;

	m_Boson1AfterFit = 0.0;
	m_Boson2AfterFit = 0.0;
	m_Boson3AfterFit = 0.0;
	m_System23MassAfterFit = 0.0;
	m_System123MassAfterFit = 0.0;
	m_ISREnergyAfterFit = 0.0;
	m_p1stAfterFit = 0.0;
	m_cos1stAfterFit = 0.0;
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
};

void ZHHBaseKinfitProcessor::init() {
	streamlog_out(DEBUG) << "   init called  " << std::endl;
	this->clearBaseValues();

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
	// = 2*alpha/pi*( ln(s/m_e^2)-1 )
	ISRPzMaxB = std::pow((double)m_isrpzmax,b);

	// get masses from EventObservablesBase
	kMassTop = EventObservablesBase::kMassTop;
	kMassZ = EventObservablesBase::kMassZ;
	kMassW = EventObservablesBase::kMassW;
	kMassH = EventObservablesBase::kMassH;

	printParameters();

	// setting the mode, and the names of constraints to calculate the associated boson momenta (for pmax and cos theta)
	if (m_fithypothesis == "NMC")    { MODE = MODE_NMC    ; MODE_IS_NMC     = true; } else
	if (m_fithypothesis == "ZHH")    { MODE = MODE_ZHH    ; MODE_IS_ZHH     = true; m_readContraints = {"z1 mass", "h2 mass", "h1 mass"}; } else
	if (m_fithypothesis == "ZZH")    { MODE = MODE_ZZH    ; MODE_IS_ZZH     = true; m_readContraints = {"z1 mass", "z3 mass", "h1 mass"}; } else
	if (m_fithypothesis == "ZZZ")    { MODE = MODE_ZZZ    ; MODE_IS_ZZZ     = true; m_readContraints = {"z1 mass", "z3 mass", "z2 mass"}; } else
	if (m_fithypothesis == "ZZHsoft"){ MODE = MODE_ZZHsoft; MODE_IS_ZZHsoft = true; m_readContraints = {"z1 mass", "z3 mass", "h1 mass"}; } else
	if (m_fithypothesis == "MH")     { MODE = MODE_MH     ; MODE_IS_MH      = true; } else
	if (m_fithypothesis == "EQM")    { MODE = MODE_EQM    ; MODE_IS_EQM     = true; } else
		throw EVENT::Exception("Unimplemented fithypothesis");

	// assign m_dijetTargets for simple chi square calculation
	m_dijetTargets = { 23, 25, 25 }; // dummy values, obly used for readout. getMass() does not actually use the mass given when constructing the MassConstraint

	if (MODE_IS_MH)  { m_nDijets = 1; m_dijetTargets = { 25 }; m_constraintTypes = {0}; };
	if (MODE_IS_EQM) { m_constraintTypes = {2}; };

	if (MODE_IS_ZHH)     { m_nDijets = 3; m_dijetTargets = { 23, 25, 25 }; m_constraintTypes = {0, 0, 0}; } else 
	if (MODE_IS_ZZH)     { m_nDijets = 3; m_dijetTargets = { 23, 23, 25 }; m_constraintTypes = {0, 0, 0}; } else
	if (MODE_IS_ZZZ)     { m_nDijets = 3; m_dijetTargets = { 23, 23, 23 }; m_constraintTypes = {0, 0, 0}; }
	if (MODE_IS_ZZHsoft) { m_nDijets = 3; m_dijetTargets = { 23, 23, 25 }; m_constraintTypes = {0, 1, 0}; }

	m_nDijets = m_dijetTargets.size(); // MH not implemented for simpleChi2 (considered not needed)

	float bosonMass;
	m_dijet23MassSum = 0; // inv mass of last two dijets
	m_dijet123MassSum = 0; // inv mass of all dijets
	for (unsigned short i = 0; i < m_nDijets; i++) {
		switch (m_dijetTargets[i]) {
			case 25: bosonMass = kMassH; break;
			case 23: bosonMass = kMassZ; break;
			case 24: bosonMass = kMassW; break;
			case 6: bosonMass = kMassTop; break;
			default: throw EVENT::Exception("Unimplemented dijet target");
		}

		m_dijetMasses.push_back(bosonMass);

		if (i >= m_nDijets - 2) m_dijet23MassSum += bosonMass;
		if (i < 3) m_dijet123MassSum += bosonMass;
	}

  	m_pTTree = new TTree(m_ttreeName.c_str(), m_ttreeName.c_str());

	if (m_outputFile.size()) {
		m_pTFile = new TFile(m_outputFile.c_str(), "recreate");
		m_pTTree->SetDirectory(m_pTFile);
	}

  	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");
	m_pTTree->Branch("nJets",&m_nJets,"nJets/I") ;
	m_pTTree->Branch("nIsoLeptons",&m_nIsoLeps,"nIsoLeptons/I") ;
	m_pTTree->Branch("nSLDecayBHadron",&m_nSLDecayBHadron,"nSLDecayBHadron/I") ;
	m_pTTree->Branch("nSLDecayCHadron",&m_nSLDecayCHadron,"nSLDecayCHadron/I") ;
	m_pTTree->Branch("nSLDecayTauLepton",&m_nSLDecayTauLepton,"nSLDecayTauLepton/I") ;
	m_pTTree->Branch("nSLDecayTotal",&m_nSLDecayTotal,"nSLDecayTotal/I") ;
	m_pTTree->Branch("nCorrectedSLD",&m_nCorrectedSLD,"nCorrectedSLD/I") ;
	m_pTTree->Branch("ISREnergyTrue",&m_ISREnergyTrue,"ISREnergyTrue/F") ;
	m_pTTree->Branch("BSEnergyTrue",&m_BSEnergyTrue,"BSEnergyTrue/F") ;
	m_pTTree->Branch("System23MassHardProcess",&m_System23MassHardProcess,"System23MassHardProcess/F") ;

	m_pTTree->Branch("FitErrorCode_woNu" , &m_FitErrorCode_woNu , "FitErrorCode_woNu/I" );
	m_pTTree->Branch("Boson1BeforeFit_woNu" , &m_Boson1BeforeFit_woNu , "Boson1BeforeFit_woNu/F" );
	m_pTTree->Branch("Boson2BeforeFit_woNu" , &m_Boson2BeforeFit_woNu , "Boson2BeforeFit_woNu/F" );
	m_pTTree->Branch("Boson3BeforeFit_woNu" , &m_Boson3BeforeFit_woNu , "Boson3BeforeFit_woNu/F" );
	m_pTTree->Branch("System23MassBeforeFit_woNu" , &m_System23MassBeforeFit_woNu , "System23MassBeforeFit_woNu/F" );
	m_pTTree->Branch("System123MassBeforeFit_woNu" , &m_System123MassBeforeFit_woNu , "System123MassBeforeFit_woNu/F" );
	m_pTTree->Branch("ISREnergyBeforeFit_woNu" , &m_ISREnergyBeforeFit_woNu , "ISREnergyBeforeFit_woNu/F" );

	m_pTTree->Branch("Boson1AfterFit_woNu" , &m_Boson1AfterFit_woNu , "Boson1AfterFit_woNu/F" );
	m_pTTree->Branch("Boson2AfterFit_woNu" , &m_Boson2AfterFit_woNu , "Boson2AfterFit_woNu/F" );
	m_pTTree->Branch("Boson3AfterFit_woNu" , &m_Boson3AfterFit_woNu , "Boson3AfterFit_woNu/F" );
	m_pTTree->Branch("System23MassAfterFit_woNu" , &m_System23MassAfterFit_woNu , "System23MassAfterFit_woNu/F" );
	m_pTTree->Branch("System123MassAfterFit_woNu" , &m_System123MassAfterFit_woNu , "System123MassAfterFit_woNu/F" );
	m_pTTree->Branch("ISREnergyAfterFit_woNu" , &m_ISREnergyAfterFit_woNu , "ISREnergyAfterFit_woNu/F" );
	m_pTTree->Branch("FitProbability_woNu" , &m_FitProbability_woNu , "FitProbability_woNu/F" );
	m_pTTree->Branch("FitChi2_woNu" , &m_FitChi2_woNu , "FitChi2_woNu/F" );
	m_pTTree->Branch("FitChi2_byMass" , &m_FitChi2_byMass , "FitChi2_byMass/F" );

	m_pTTree->Branch("p1stAfterFit_woNu" , &m_p1stAfterFit_woNu , "p1stAfterFit_woNu/F" );
	m_pTTree->Branch("cos1stAfterFit_woNu" , &m_cos1stAfterFit_woNu , "cos1stAfterFit_woNu/F" );

  	m_pTTree->Branch("p1stAfterFit" , &m_p1stAfterFit , "p1stAfterFit/F" );
	m_pTTree->Branch("cos1stAfterFit" , &m_cos1stAfterFit , "cos1stAfterFit/F" );

	m_pTTree->Branch("BestMatchingKinfit" , &m_bestMatchingKinfit );
	m_pTTree->Branch("BestMatchingByMass" , &m_bestMatchingByMass );

	m_pTTree->Branch("pullJetEnergy_woNu" , &m_pullJetEnergy_woNu );
	m_pTTree->Branch("pullJetTheta_woNu" , &m_pullJetTheta_woNu );
	m_pTTree->Branch("pullJetPhi_woNu" , &m_pullJetPhi_woNu );
	m_pTTree->Branch("pullLeptonInvPt_woNu" , &m_pullLeptonInvPt_woNu );
	m_pTTree->Branch("pullLeptonTheta_woNu" , &m_pullLeptonTheta_woNu );
	m_pTTree->Branch("pullLeptonPhi_woNu" , &m_pullLeptonPhi_woNu );

	/*	m_pTTree->Branch("FitErrorCode_wNu" , &m_FitErrorCode_wNu , "FitErrorCode_wNu/I" );
	m_pTTree->Branch("Boson1BeforeFit_wNu" , &m_Boson1BeforeFit_wNu , "Boson1BeforeFit_wNu/F" );
	m_pTTree->Branch("Boson2BeforeFit_wNu" , &m_Boson2BeforeFit_wNu , "Boson2BeforeFit_wNu/F" );
	m_pTTree->Branch("Boson3BeforeFit_wNu" , &m_Boson3BeforeFit_wNu , "Boson3BeforeFit_wNu/F" );
	m_pTTree->Branch("Boson1AfterFit_wNu" , &m_Boson1AfterFit_wNu , "Boson1AfterFit_wNu/F" );
	m_pTTree->Branch("Boson2AfterFit_wNu" , &m_Boson2AfterFit_wNu , "Boson2AfterFit_wNu/F" );
	m_pTTree->Branch("Boson3AfterFit_wNu" , &m_Boson3AfterFit_wNu , "Boson3AfterFit_wNu/F" );
	m_pTTree->Branch("FitProbability_wNu" , &m_FitProbability_wNu , "FitProbability_wNu/F" );
	m_pTTree->Branch("FitChi2_wNu" , &m_FitChi2_wNu , "FitChi2_wNu/F" );
	m_pTTree->Branch("pullJetEnergy_wNu" , &m_pullJetEnergy_wNu );
	m_pTTree->Branch("pullJetTheta_wNu" , &m_pullJetTheta_wNu );
	m_pTTree->Branch("pullJetPhi_wNu" , &m_pullJetPhi_wNu );
	m_pTTree->Branch("pullLeptonInvPt_wNu" , &m_pullLeptonInvPt_wNu );
	m_pTTree->Branch("pullLeptonTheta_wNu" , &m_pullLeptonTheta_wNu );
	m_pTTree->Branch("pullLeptonPhi_wNu" , &m_pullLeptonPhi_wNu );*/

	m_pTTree->Branch("FitErrorCode" , &m_FitErrorCode , "FitErrorCode/I" );
	m_pTTree->Branch("Boson1BeforeFit" , &m_Boson1BeforeFit , "Boson1BeforeFit/F" );
	m_pTTree->Branch("Boson2BeforeFit" , &m_Boson2BeforeFit , "Boson2BeforeFit/F" );
	m_pTTree->Branch("Boson3BeforeFit" , &m_Boson3BeforeFit , "Boson3BeforeFit/F" );
	m_pTTree->Branch("System23MassBeforeFit" , &m_System23MassBeforeFit , "System23MassBeforeFit/F" );
	m_pTTree->Branch("System123MassBeforeFit" , &m_System123MassBeforeFit , "System123MassBeforeFit/F" );
	m_pTTree->Branch("ISREnergyBeforeFit" , &m_ISREnergyBeforeFit , "ISREnergyBeforeFit/F" );

	m_pTTree->Branch("Boson1AfterFit" , &m_Boson1AfterFit , "Boson1AfterFit/F" );
	m_pTTree->Branch("Boson2AfterFit" , &m_Boson2AfterFit , "Boson2AfterFit/F" );
	m_pTTree->Branch("Boson3AfterFit" , &m_Boson3AfterFit , "Boson3AfterFit/F" );
	m_pTTree->Branch("System23MassAfterFit" , &m_System23MassAfterFit , "System23MassAfterFit/F" );
	m_pTTree->Branch("System123MassAfterFit" , &m_System123MassAfterFit , "System123MassAfterFit/F" );
	m_pTTree->Branch("ISREnergyAfterFit" , &m_ISREnergyAfterFit , "ISREnergyAfterFit/F" );
	m_pTTree->Branch("FitProbability" , &m_FitProbability , "FitProbability/F" );
	m_pTTree->Branch("FitChi2" , &m_FitChi2 , "FitChi2/F" );

	m_pTTree->Branch("pullJetEnergy" , &m_pullJetEnergy );
	m_pTTree->Branch("pullJetTheta" , &m_pullJetTheta );
	m_pTTree->Branch("pullJetPhi" , &m_pullJetPhi );
	m_pTTree->Branch("pullLeptonInvPt" , &m_pullLeptonInvPt );
	m_pTTree->Branch("pullLeptonTheta" , &m_pullLeptonTheta );
	m_pTTree->Branch("pullLeptonPhi" , &m_pullLeptonPhi );
	m_pTTree->Branch("TrueNeutrinoEnergy", &m_TrueNeutrinoEnergy );
	m_pTTree->Branch("RecoNeutrinoEnergy", &m_RecoNeutrinoEnergy );
	m_pTTree->Branch("RecoNeutrinoEnergyKinfit", &m_RecoNeutrinoEnergyKinfit );
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

	initChannelValues();

	clearChannelValues();
	clearBaseValues();
};

void ZHHBaseKinfitProcessor::processEvent( LCEvent* pLCEvent ) {
	streamlog_out(MESSAGE1) << "processing event " << m_nEvt << " in run " << m_nRun << std::endl;

	m_nRun = pLCEvent->getRunNumber();
	m_nEvt = pLCEvent->getEventNumber();

	updateChannelValues(pLCEvent);

	clearChannelValues();
	clearBaseValues();
};

void ZHHBaseKinfitProcessor::processRunHeader(LCRunHeader* run)
{
  (void) run;
	m_nRun++ ;
}

void ZHHBaseKinfitProcessor::assignPostFitMasses(FitResult fitResult, bool woNu) {
	streamlog_out(MESSAGE1) << "AssignPostFitMasses " << (woNu ? "WITHOUT" : "WITH") << " neutrino correction" << std::endl;
	streamlog_out(MESSAGE1) << "(error code, fit prob, chi2) = (" << fitResult.fitter->getError() << ", " << fitResult.fitter->getProbability() << ", " << fitResult.fitter->getChi2() << ")" << endl;

	auto constraints = fitResult.constraints;
	streamlog_out(MESSAGE) << constraints.size() << " constraints set:" << std::endl;
	
	for (unsigned short constraint_idx = 0; constraint_idx < constraints.size(); constraint_idx++) {
		streamlog_out(MESSAGE) << " Constraint [" << constraint_idx << "]: ";
		if (constraints[constraint_idx] != nullptr) {
			streamlog_out(MESSAGE) << constraints[constraint_idx]->getName() << " M=" << (dynamic_pointer_cast<MassConstraint>(constraints[constraint_idx])->getMass()) << std::endl;
		} else {
			streamlog_out(MESSAGE) << "Not set" << std::endl;
		}
	}

	shared_ptr<MassConstraint> mc_boson1;
	shared_ptr<MassConstraint> mc_boson2;
	shared_ptr<MassConstraint> mc_boson3;
	shared_ptr<MassConstraint> mc_system23 = dynamic_pointer_cast<MassConstraint>(constraints[3]);
	shared_ptr<MassConstraint> mc_system123 = dynamic_pointer_cast<MassConstraint>(constraints[4]);

	if (constraints[0] != nullptr)
		mc_boson1 = dynamic_pointer_cast<MassConstraint>(constraints[0]);

	if (constraints[1] != nullptr) 
		mc_boson2 = dynamic_pointer_cast<MassConstraint>(constraints[1]);

	if (constraints[2] != nullptr) 
		mc_boson3 = dynamic_pointer_cast<MassConstraint>(constraints[2]);

	// find p1st and cos1st
	float p1st = 0.0;
	float cos1st = 0.0;
	unsigned short highest_idx = -1;

	ROOT::Math::PxPyPzEVector tempvec (0., 0., 0., 0.);

	for (unsigned short i = 0; i < 3; i++) {
		if (constraints[i] != nullptr) {
			shared_ptr<ParticleConstraint> mc = dynamic_pointer_cast<ParticleConstraint>(constraints[i]);
			double *temp = mc->getFourMomentum(1);

			tempvec.SetPxPyPzE(temp[1], temp[2], temp[3], temp[0]);
			//streamlog_out(MESSAGE) << "CURRENT MOMENTUM : '" << tempvec.P() << std::endl;

			if (tempvec.P() > p1st) {
				p1st = tempvec.P();
				cos1st = cos(tempvec.Theta());
				highest_idx = i;
			}

			delete temp;
		}
	}

	if (p1st > 0)
		streamlog_out(MESSAGE) << "Found p1st from constraint " << highest_idx << " ("<< constraints[highest_idx]->getName() <<") with p1st=" << p1st << std::endl;

	m_bestMatchingKinfit = fitResult.permutation;

	if (woNu) {
		m_FitErrorCode_woNu = fitResult.fitter->getError();
		m_FitProbability_woNu = fitResult.fitter->getProbability();
		m_FitChi2_woNu = fitResult.fitter->getChi2();

		m_Boson1AfterFit_woNu = mc_boson1 != nullptr ? mc_boson1->getMass() : 0;
		m_Boson2AfterFit_woNu = mc_boson2 != nullptr ? mc_boson2->getMass() : 0;
		m_Boson3AfterFit_woNu = mc_boson3 != nullptr ? mc_boson3->getMass() : 0;
		m_System23MassAfterFit_woNu = mc_system23->getMass();
		m_System123MassAfterFit_woNu = mc_system123->getMass();
		m_p1stAfterFit_woNu = p1st;
		m_cos1stAfterFit_woNu = cos1st;
	} else {
		m_FitErrorCode = fitResult.fitter->getError();
		m_FitProbability = fitResult.fitter->getProbability();
		m_FitChi2 = fitResult.fitter->getChi2();

		m_Boson1AfterFit = mc_boson1 != nullptr ? mc_boson1->getMass() : 0;
		m_Boson2AfterFit = mc_boson2 != nullptr ? mc_boson2->getMass() : 0;
		m_Boson3AfterFit = mc_boson3 != nullptr ? mc_boson3->getMass() : 0;
		m_System23MassAfterFit = mc_system23->getMass();
		m_System123MassAfterFit = mc_system123->getMass();
		m_p1stAfterFit = p1st;
		m_cos1stAfterFit = cos1st;
	}
	
	streamlog_out(MESSAGE) << endl;
}

void ZHHBaseKinfitProcessor::check( LCEvent* event )
{
  (void) event;
}

SimpleChi2Result ZHHBaseKinfitProcessor::simpleChi2Pairing(pfoVector jets) {
	std::vector<unsigned short> jet_matching;
	std::vector<float> dijet_masses(m_nDijets, 0.);
	std::vector<unsigned short> dijetTargets = m_dijetTargets;
	float chi2min;

	if (m_nDijets == 3 && m_nAskedJets == 4) {
		// Z mass is calculated in the channel specific processors
		// this is only needed for vv; for qq the below works "as is"
		if (m_dijetTargets[0] == 23) {
			dijetTargets = {m_dijetTargets[1], m_dijetTargets[2]};
		} else
			throw EVENT::Exception("Unimplemented dijet target");
	}

	if (!(MODE_IS_NMC || MODE_IS_EQM || MODE_IS_ZZHsoft)) {	
		streamlog_out(MESSAGE) << "dijetTargets.size() = " << dijetTargets.size() << std::endl;
		std::tie(jet_matching, dijet_masses, chi2min) = EventObservablesBase::pairJetsByMass(jets, dijetTargets);
	}

	for (auto mass: dijet_masses) {
		streamlog_out(MESSAGE) << "Dijet mass: " << mass << std::endl;
	}

	return std::make_tuple(dijet_masses, chi2min, jet_matching);
};

std::vector<double> ZHHBaseKinfitProcessor::calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm)
{
	std::vector<double> masses(5, 0.);
	shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
	shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();
	shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo = make_shared<vector<shared_ptr<LeptonFitObject>>>();
	
	shared_ptr<MassConstraint> z1;
	shared_ptr<ZinvisibleFitObject> zfo;
	shared_ptr<MassConstraint> system23;
	shared_ptr<MassConstraint> system123;
  
	//Set JetFitObjects
	for (unsigned int i_jet =0; i_jet < jets.size(); i_jet++) {
		float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
		getJetParameters( jets[ i_jet ] , parameters , errors );
		auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
		jfo->push_back(j);
		const string name = "jet"+to_string(i_jet);
		j->setName(name.c_str());
	}
  
	for (auto i = perm.begin(); i != perm.begin()+m_nJets ; ++i) {
		//for(auto i : perm) {
		streamlog_out(MESSAGE) << " Picking jet for start masses " << *i << std::endl ;
		auto jsp = make_shared<JetFitObject>(*jfo->at(*i));
		jfo_perm->push_back(jsp);
	}

	z1 = make_shared<MassConstraint>(kMassZ);
	z1->setName("boson0 (z) mass");

	system23 = make_shared<MassConstraint>(m_dijet23MassSum);
	system23->setName("system23 mass");

	system123 = make_shared<MassConstraint>(m_dijet123MassSum);
	system123->setName("system123 mass");
  
	if (channel() == CHANNEL_LL) {
		//Set LeptonFitObjects
		for (unsigned int i_lep =0; i_lep < leptons.size(); i_lep++) {
			float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
			getLeptonParameters( leptons[ i_lep ] , parameters , errors );
			auto l = make_shared<LeptonFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , leptons[ i_lep ]->getMass() );
			lfo->push_back(l);
			const string name = "lepton"+to_string(i_lep);
			l->setName(name.c_str());
		}

		z1->addToFOList(*lfo->at(0), 1);
		z1->addToFOList(*lfo->at(1), 1);

		system123->addToFOList(*lfo->at(0), 1);
		system123->addToFOList(*lfo->at(1), 1);
	} else if (channel() == CHANNEL_VV) {
		ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);

		for (int i_jet = 0; i_jet < m_nJets; i_jet++)
			seenFourMomentum += v4(jets[i_jet]);

		ROOT::Math::PxPyPzMVector ZinvFourMomentum(-seenFourMomentum.Px(), -seenFourMomentum.Pz(), -seenFourMomentum.Pz(), kMassZ); // M_Z PDG average in 2024 review    
		zfo = make_shared<ZinvisibleFitObject> (ZinvFourMomentum.E(), ZinvFourMomentum.Theta(), ZinvFourMomentum.Phi(), 1.0, 0.1, 0.1, kMassZ);
		zfo->setName("Zinvisible");

		z1->addToFOList(*zfo, 1);
		system123->addToFOList(*zfo, 1);
	} else if (channel() == CHANNEL_QQ) {
		z1->addToFOList (*jfo_perm->at(4), 1);
		z1->addToFOList (*jfo_perm->at(5), 1);
	}

	unsigned short dijet_idx = 0;
	if (m_nDijets && m_dijetTargets[0] == 23) {
		streamlog_out(MESSAGE) << "Adding mass constraint for boson z1 with mass " << z1->getMass() << " GeV" << std::endl ;
		masses[0] = z1->getMass();
		dijet_idx = 1;
	}

	unsigned short jet_idx = 0;
	for (; dijet_idx < m_nDijets; dijet_idx++) {
		shared_ptr<MassConstraint> mc = make_shared<MassConstraint>(m_dijetMasses[dijet_idx]);
		mc->setName((std::string("boson") + std::to_string(dijet_idx) + " mass").c_str());
		mc->addToFOList(*jfo_perm->at(jet_idx), 1);
		mc->addToFOList(*jfo_perm->at(jet_idx + 1), 1);

		streamlog_out(MESSAGE) << "Adding mass constraint for boson " << dijet_idx << " (m=" << m_dijetMasses[dijet_idx] << " GeV) and jets (" << jet_idx << "," << (jet_idx + 1) << ")" << std::endl ;
		
		masses[dijet_idx] = mc->getMass();
		jet_idx += 2;
	}

	for (unsigned short i = 0; i < jets.size(); i++) {
		system123->addToFOList (*jfo_perm->at(i), 1);

		if (i < 4) {
			system23->addToFOList (*jfo_perm->at(i), 1); 
		}
	}

	streamlog_out(MESSAGE) << "system123->getName() = " << system123->getName() << std::endl ;
	
    masses[3] = system23->getMass(1);
    masses[4] = system123->getMass(1);

    return masses;
}

ZHHBaseKinfitProcessor::FitResult ZHHBaseKinfitProcessor::performFIT(pfoVector jets, pfoVector leptons, bool traceEvent, bool woNu) {
	streamlog_out(MESSAGE6) << "performFIT " << (woNu ? "WITHOUT" : "WITH") <<" neutrino correction"  << std::endl ; //changed from debug level

	shared_ptr<vector<shared_ptr<JetFitObject>>> jfo = make_shared<vector<shared_ptr<JetFitObject>>>();
	shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo = make_shared<vector<shared_ptr<LeptonFitObject>>>();

	vector<shared_ptr<BaseConstraint>> target_constraints;
	vector<shared_ptr<BaseHardConstraint>> mass_constraints(5, nullptr);
	shared_ptr<ISRPhotonFitObject> photon;
	shared_ptr<BaseFitter> fitter;

	shared_ptr<MassConstraint> z1;
	shared_ptr<ZinvisibleFitObject> zfo;
	shared_ptr<MassConstraint> system23;
	shared_ptr<MassConstraint> system123;

	double bestProb = -1;
	double bestChi2 = 9999999999999.;
	FitResult bestFitResult;

	//////////////////////////////////////////////////////////////////////////////////////
	//////												  							//////
	//////					Set JetFitObjects and LeptonFitObjects					//////
	//////												  							//////
	//////////////////////////////////////////////////////////////////////////////////////
	for (size_t i_jet =0; i_jet < jets.size(); i_jet++) {
		streamlog_out(MESSAGE6) << "get jet"<< i_jet+1 <<" parameters"  << std::endl ; //changed from debug level
		float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
		getJetParameters( jets[ i_jet ] , parameters , errors );
		auto j = make_shared<JetFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , jets[ i_jet ]->getMass() );
		jfo->push_back(j);
		const string name = "jet"+to_string(i_jet);
		j->setName(name.c_str());
		//streamlog_out(MESSAGE)  << " start four-vector of jet"<< i_jet+1 <<": " << *j  << std::endl ;
		streamlog_out(DEBUG)  << " start four-vector of jet"<< i_jet+1 <<": " << "[" << jets[ i_jet ]->getMomentum()[0] << ", " << jets[ i_jet ]->getMomentum()[1] << ", " << jets[ i_jet ]->getMomentum()[2] << ", " << jets[ i_jet ]->getEnergy() << "]" << std::endl ;
	}

	for (size_t i_lep =0; i_lep < leptons.size(); i_lep++) {
		streamlog_out(MESSAGE6) << "get lepton"<< i_lep+1 <<" parameters"  << std::endl ; //changed from debug level 
		float parameters[ 3 ]{ 0.0 } , errors[ 3 ]{ 0.0 };
		getLeptonParameters( leptons[ i_lep ] , parameters , errors );
		auto l = make_shared<LeptonFitObject> ( parameters[ 0 ] , parameters[ 1 ] , parameters[ 2 ] , errors[ 0 ] , errors[ 1 ] , errors[ 2 ] , leptons[ i_lep ]->getMass() );
		lfo->push_back(l);
		const string name = "lepton"+to_string(i_lep);
		l->setName(name.c_str());
		//streamlog_out(MESSAGE)  << " start four-vector of lepton"<< i_lep+1 <<": " << *l  << std::endl ;
		streamlog_out(DEBUG)  << " start four-vector of lepton"<< i_lep+1 <<": " << "[" << leptons[ i_lep ]->getMomentum()[0] << ", " << leptons[ i_lep ]->getMomentum()[1] << ", " << leptons[ i_lep ]->getMomentum()[2] << ", " << leptons[ i_lep ]->getEnergy() << "]"  << std::endl ;
	}

	assert(m_nJets == m_nAskedJets);

	for (unsigned int iperm = 0; iperm < perms.size(); iperm++) {
		target_constraints.clear();
		mass_constraints.clear();
		photon.reset();
		fitter.reset();
		z1.reset();
		zfo.reset();
		system23.reset();
		system123.reset();

		streamlog_out(MESSAGE) << " ================================================= " << std::endl ;
		streamlog_out(MESSAGE) << " iperm = " << iperm << std::endl ;

		shared_ptr<vector<shared_ptr<JetFitObject>>> jfo_perm = make_shared<vector<shared_ptr<JetFitObject>>>();
		shared_ptr<vector<shared_ptr<BaseFitObject>>> fos = make_shared<vector<shared_ptr<BaseFitObject>>>();

		streamlog_out(MESSAGE) << " Picking jets ";
		for(auto i : perms[iperm]) {
			streamlog_out(MESSAGE) << i << " ";
			auto jsp = make_shared<JetFitObject>(*jfo->at(i));
			jfo_perm->push_back(jsp);
		}
		streamlog_out(MESSAGE) << std::endl ;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////												    //////
		//////					Set Constraints Before Fit				    //////
		//////												    //////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
		float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
		shared_ptr<MomentumConstraint> pxc = make_shared<MomentumConstraint>( 0 , 1 , 0 , 0 , target_p_due_crossing_angle);//Factor for: (energy sum, px sum, py sum,pz sum,target value of sum)    
		pxc->setName("sum(p_x)");
			
		shared_ptr<MomentumConstraint> pyc = make_shared<MomentumConstraint>(0, 0, 1, 0, 0);
		pyc->setName("sum(p_y)");
			
		shared_ptr<MomentumConstraint> pzc = make_shared<MomentumConstraint>(0, 0, 0, 1, 0);
		pzc->setName("sum(p_z)");
			
		double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.); //TODO: check equation
		shared_ptr<MomentumConstraint> ec = make_shared<MomentumConstraint>(1, 0, 0, 0, E_lab);
		ec->setName("sum(E)");

		target_constraints.push_back(pxc);
		target_constraints.push_back(pyc);
		target_constraints.push_back(pzc);
		target_constraints.push_back(ec);

		// create fitter
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

		streamlog_out(DEBUG)  << "Start four-vector of" << std::endl;
		for (auto j : *jfo_perm) {
			streamlog_out(DEBUG)  << " jet " << j->getName() << ": " << *j  << std::endl ;  //changed from debug level 

			fos->push_back(j);
			pxc->addToFOList(*j);
			pyc->addToFOList(*j);
			pzc->addToFOList(*j); 
			ec->addToFOList(*j);
			fitter->addFitObject(*j);
		}

		if( m_fitISR ) {
			photon = make_shared<ISRPhotonFitObject>(0., 0., -pzc->getValue(), b, ISRPzMaxB);
			photon->setName("photon");
			if( m_fitISR ) {
				streamlog_out(DEBUG)  << " ISR photon: " << *(photon) << std::endl ; //changed from debug level 
				fos->push_back(photon);
				pxc->addToFOList(*(photon));
				pyc->addToFOList(*(photon));
				pzc->addToFOList(*(photon));
				ec->addToFOList(*(photon));
			}

			fitter->addFitObject( *(photon) );
			streamlog_out(MESSAGE8) << "ISR added to fit"  << std::endl ; //changed from debug level 
		}

		// conservation of four momenta
		fitter->addConstraint( pxc.get() );
		fitter->addConstraint( pyc.get() );
		fitter->addConstraint( pzc.get() );
		fitter->addConstraint( ec.get() );

		// for extraction of results
		z1 = make_shared<MassConstraint>(kMassZ);
		z1->setName("z1 mass");

		system23 = make_shared<MassConstraint>(m_dijet23MassSum);
		system23->setName("system23 mass");

		system123 = make_shared<MassConstraint>(m_dijet123MassSum);
		system123->setName("system123 mass");
		
		if (channel() == CHANNEL_VV) {
			shared_ptr<vector<shared_ptr<ZinvisibleFitObject>>> zfo_perm = make_shared<vector<shared_ptr<ZinvisibleFitObject>>>();

			ROOT::Math::PxPyPzEVector seenFourMomentum(0.,0.,0.,0.);
			for (int i_jet = 0; i_jet < m_nJets; i_jet++)
				seenFourMomentum += v4(jets[ i_jet ]);
			
			ROOT::Math::PxPyPzMVector ZinvFourMomentum(-seenFourMomentum.Px(), -seenFourMomentum.Pz(), -seenFourMomentum.Pz(), kMassZ); // M_Z PDG average in 2024 review
			zfo = make_shared<ZinvisibleFitObject> (ZinvFourMomentum.E(), ZinvFourMomentum.Theta(), ZinvFourMomentum.Phi(), 1.0, 0.1, 0.1, kMassZ);
			zfo->setName("Zinvisible");

			zfo_perm->push_back(make_shared<ZinvisibleFitObject>(*zfo)); // only one ZFO, no loop needed

			for(auto z : *zfo_perm) {
				fos->push_back(z);
				pxc->addToFOList(*z);
				pyc->addToFOList(*z);
				pzc->addToFOList(*z);
				ec->addToFOList(*z);
				fitter->addFitObject(*z);
			}

			z1->addToFOList(*zfo_perm->at(0), 1);
		} else if (channel() == CHANNEL_LL) {
			shared_ptr<vector<shared_ptr<LeptonFitObject>>> lfo_perm = make_shared<vector<shared_ptr<LeptonFitObject>>>();

			for(size_t i = 0; i < leptons.size(); ++i) {
				auto lsp = make_shared<LeptonFitObject>(*lfo->at(i));
				lfo_perm->push_back(lsp);
			}

			for(auto l : *lfo_perm) {
				fos->push_back(l);
				pxc->addToFOList(*l);
				pyc->addToFOList(*l);
				pzc->addToFOList(*l);
				ec->addToFOList(*l);
				fitter->addFitObject(*l);
			}

			z1->addToFOList (*lfo_perm->at(0), 1);
    		z1->addToFOList (*lfo_perm->at(1), 1);
		}  else if (channel() == CHANNEL_QQ) {
			streamlog_out(MESSAGE8) << "QQ " << *jfo_perm->at(4) << " " << *jfo_perm->at(5) << endl;
			z1->addToFOList (*jfo_perm->at(4), 1);
			z1->addToFOList (*jfo_perm->at(5), 1);
		}
			
		streamlog_out(MESSAGE8)  << "	Value of E_lab before adding ISR: " << E_lab << std::endl ;  //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before adding ISR: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pxc before adding ISR: " << pxc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pyc before adding ISR: " << pyc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pzc before adding ISR: " << pzc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of ec before adding ISR: " << ec->getValue() << std::endl ; //changed from debug level 

		streamlog_out(MESSAGE8)  << "	Value of E_lab before fit: " << E_lab << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of target_p_due_crossing_angle before fit: " << target_p_due_crossing_angle << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pxc after adding ISR before fit: " << pxc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pyc after adding ISR before fit: " << pyc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of pzc after adding ISR before fit: " << pzc->getValue() << std::endl ; //changed from debug level 
		streamlog_out(MESSAGE8)  << "	Value of ec after adding ISR before fit: " << ec->getValue() << std::endl ; //changed from debug level 

		//To be added to fit, depending on fit hypothesis
		unsigned short constraint_idx = m_constraintTypes.size() < 3 ? 0 : (m_constraintTypes.size() - 2);
		unsigned short dijet_idx = 0;
		for (; constraint_idx < m_constraintTypes.size(); constraint_idx++) {
			if (m_constraintTypes[constraint_idx] == 0) {
				// MassConstraint
				float dijet_mass = m_dijetMasses[constraint_idx];
				shared_ptr<MassConstraint> mc = make_shared<MassConstraint>(dijet_mass);
				mc->setName((std::string("boson ") + std::to_string(dijet_idx) + " mass").c_str());
				mc->addToFOList(*jfo_perm->at(dijet_idx * 2), 1);
				mc->addToFOList(*jfo_perm->at(dijet_idx * 2 + 1), 1);

				streamlog_out(MESSAGE) << "Adding mass constraint "<< mc->getName() <<" for boson " << dijet_idx << " (m=" << dijet_mass << " GeV) and jets (" << (dijet_idx * 2) << "," << (dijet_idx * 2 + 1) << ")" << std::endl ;
				
				target_constraints.push_back( mc );
				fitter->addConstraint( mc.get() );
			} else if (m_constraintTypes[constraint_idx] == 1) {
				shared_ptr<SoftGaussMassConstraint> zmsoft = make_shared<SoftGaussMassConstraint>(2.4952/2, kMassZ); 
				zmsoft->setName("soft z mass");

				streamlog_out(MESSAGE) << "Adding ZMsoft constraint "<< zmsoft->getName() <<" for jets (" <<  (dijet_idx * 2) << "," << (dijet_idx * 2 + 1) << ")" << std::endl ;

				zmsoft->addToFOList (*jfo_perm->at(dijet_idx * 2), 1);
				zmsoft->addToFOList (*jfo_perm->at(dijet_idx * 2 + 1), 1);

				target_constraints.push_back( zmsoft );
				fitter->addConstraint( zmsoft.get() );
			} else if (m_constraintTypes[constraint_idx] == 2) {
				shared_ptr<MassConstraint> eqm = make_shared<MassConstraint>(0.);
				eqm->setName("equal mass");

				streamlog_out(MESSAGE) << "Adding EQM constraint "<< eqm->getName() <<" for jets (0,1,2,3)" << std::endl ;

				eqm->addToFOList (*jfo_perm->at(0), 1);
				eqm->addToFOList (*jfo_perm->at(1), 1);
				eqm->addToFOList (*jfo_perm->at(2), 2);
				eqm->addToFOList (*jfo_perm->at(3), 2);

				target_constraints.push_back( eqm );				
				fitter->addConstraint( eqm.get() );
			}

			dijet_idx += 1;			
		}

		// do the fit
		auto fitconstraints = fitter->getConstraints();
		auto fitsoftconstraints = fitter->getSoftConstraints();

		streamlog_out(MESSAGE8) << "(" << fitconstraints->size() << ", " << fitsoftconstraints->size() << ")=(nHard, nSoft) constraints added to fit:" << std::endl ; //changed from debug level 
		for (auto constraint: *fitconstraints)
			streamlog_out(MESSAGE8) << "HARD " << constraint->getName() << " constr value = " << (constraint)->getValue() << std::endl; //changed from debug level 

		for (auto it = fitsoftconstraints->begin(); it != fitsoftconstraints->end(); ++it)
			streamlog_out(MESSAGE8) << "SOFT " << (*it)->getName() << " constr value = " << (*it)->getValue() << std::endl; //changed from debug level 

		float fitProbability = fitter->fit();

		// evaluate the outputs
		if(fitter->getError() == 0) {
			if (fitter->getChi2() < bestChi2) {
				//if(fitProbability > bestProb) {
				streamlog_out(MESSAGE) << "fit probability: " << fitProbability << " is better than " << bestProb << " use that one" << std::endl;
				streamlog_out(MESSAGE) << "fit chi2 = " << fitter->getChi2() << std::endl;
				streamlog_out(MESSAGE) << "error code: " << fitter->getError() << std::endl;
				bestProb = fitProbability;
				bestChi2 = fitter->getChi2();

				if( m_fitISR ) {
					streamlog_out(MESSAGE)  << "After fit four-vector of ISR photon: " << *(photon) << std::endl ; //changed from debug level
					streamlog_out(MESSAGE)  << "After fit ISR energy = " << photon->getE() << std::endl;
				}

				mass_constraints = {nullptr, nullptr, nullptr, nullptr, nullptr};
				unsigned short dijet_idx = 0;
				if (m_nDijets && m_dijetTargets[0] == 23) {
					streamlog_out(MESSAGE) << "Adding helper mass constraint for boson z1 with mass " << z1->getMass() << " GeV" << std::endl ;
					mass_constraints[0] = z1;
					dijet_idx = 1;
				}

				unsigned short jet_idx = 0;
				for (; dijet_idx < m_nDijets; dijet_idx++) {
					shared_ptr<MassConstraint> mc = make_shared<MassConstraint>(m_dijetMasses[dijet_idx]);
					mc->setName((std::string("boson") + std::to_string(dijet_idx) + " mass").c_str());
					mc->addToFOList(*jfo_perm->at(jet_idx), 1);
					mc->addToFOList(*jfo_perm->at(jet_idx + 1), 1);

					streamlog_out(MESSAGE) << "Adding helper mass constraint for boson " << dijet_idx << " (m=" << m_dijetMasses[dijet_idx] << " GeV) and jets (" << jet_idx << "," << (jet_idx + 1) << ")" << std::endl ;
					
					mass_constraints[dijet_idx] = mc;
					jet_idx += 2;
				}

				for (unsigned short i = 0; i < jets.size(); i++) {
					system123->addToFOList (*jfo_perm->at(i), 1);

					if (i < 4)
						system23->addToFOList (*jfo_perm->at(i), 1); 
				}

				mass_constraints[3] = system23;
				mass_constraints[4] = system123;
		  
				FitResult fitresult = FitResult(fitter, mass_constraints, fos, perms[iperm]);
		  		bestFitResult = fitresult;

				m_FitErrorCode = bestFitResult.fitter->getError();
				m_FitProbability = bestFitResult.fitter->getProbability();
				m_FitChi2 = bestFitResult.fitter->getChi2();
				m_bestMatchingKinfit = bestFitResult.permutation;
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

	if (bestFitResult.fitter != nullptr) {
		assignPostFitMasses(bestFitResult, woNu);
	}

	return bestFitResult;
}