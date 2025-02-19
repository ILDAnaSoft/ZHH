#include "ZHHBaseKinfitProcessor.h"

ZHHBaseKinfitProcessor::ZHHBaseKinfitProcessor(const std::string& name) : Processor(name),
m_pTFile(NULL),
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


void ZHHBaseKinfitProcessor::attachBestPermutation(LCCollection *jets, vector<unsigned int> bestperm, string parameterSuffix, bool fromKinfit) {
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
	m_HHMassHardProcess = 0.0;

	m_FitErrorCode_woNu = 1;
	m_ZMassBeforeFit_woNu = 0.0;
	m_Z2MassBeforeFit_woNu = 0.0;
	m_H1MassBeforeFit_woNu = 0.0;
	m_H2MassBeforeFit_woNu = 0.0;
	m_HHMassBeforeFit_woNu = 0.0;
	m_ZHHMassBeforeFit_woNu = 0.0;
	m_ISREnergyBeforeFit_woNu = 0.0;
	m_p1stBeforeFit_woNu = 0.0;
	m_cos1stBeforeFit_woNu = 0.0;

	m_ZMassAfterFit_woNu = 0.0;
	m_Z2MassAfterFit_woNu = 0.0;
	m_H1MassAfterFit_woNu = 0.0;
	m_H2MassAfterFit_woNu = 0.0;
	m_HHMassAfterFit_woNu = 0.0;
	m_ZHHMassAfterFit_woNu = 0.0;
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
	m_Z2MassBeforeFit = 0.0;
	m_H1MassBeforeFit = 0.0;
	m_H2MassBeforeFit = 0.0;
	m_HHMassBeforeFit = 0.0;
	m_ZHHMassBeforeFit = 0.0;
	m_ISREnergyBeforeFit = 0.0;
	m_p1stBeforeFit = 0.0;
	m_cos1stBeforeFit = 0.0;

	m_ZMassAfterFit = 0.0;
	m_Z2MassAfterFit = 0.0;
	m_H1MassAfterFit = 0.0;
	m_H2MassAfterFit = 0.0;
	m_HHMassAfterFit = 0.0;
	m_ZHHMassAfterFit = 0.0;
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
//	  = 2*alpha/pi*( ln(s/m_e^2)-1 )
	ISRPzMaxB = std::pow((double)m_isrpzmax,b);

	printParameters();

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
	m_pTTree->Branch("HHMassHardProcess",&m_HHMassHardProcess,"HHMassHardProcess/F") ;

	m_pTTree->Branch("FitErrorCode_woNu" , &m_FitErrorCode_woNu , "FitErrorCode_woNu/I" );
	m_pTTree->Branch("ZMassBeforeFit_woNu" , &m_ZMassBeforeFit_woNu , "ZMassBeforeFit_woNu/F" );
	m_pTTree->Branch("H1MassBeforeFit_woNu" , &m_H1MassBeforeFit_woNu , "H1MassBeforeFit_woNu/F" );
	m_pTTree->Branch("H2MassBeforeFit_woNu" , &m_H2MassBeforeFit_woNu , "H2MassBeforeFit_woNu/F" );
	m_pTTree->Branch("HHMassBeforeFit_woNu" , &m_HHMassBeforeFit_woNu , "HHMassBeforeFit_woNu/F" );
	m_pTTree->Branch("ZHHMassBeforeFit_woNu" , &m_ZHHMassBeforeFit_woNu , "ZHHMassBeforeFit_woNu/F" );
	m_pTTree->Branch("ISREnergyBeforeFit_woNu" , &m_ISREnergyBeforeFit_woNu , "ISREnergyBeforeFit_woNu/F" );

	m_pTTree->Branch("ZMassAfterFit_woNu" , &m_ZMassAfterFit_woNu , "ZMassAfterFit_woNu/F" );
	m_pTTree->Branch("H1MassAfterFit_woNu" , &m_H1MassAfterFit_woNu , "H1MassAfterFit_woNu/F" );
	m_pTTree->Branch("H2MassAfterFit_woNu" , &m_H2MassAfterFit_woNu , "H2MassAfterFit_woNu/F" );
	m_pTTree->Branch("HHMassAfterFit_woNu" , &m_HHMassAfterFit_woNu , "HHMassAfterFit_woNu/F" );
	m_pTTree->Branch("ZHHMassAfterFit_woNu" , &m_ZHHMassAfterFit_woNu , "ZHHMassAfterFit_woNu/F" );
	m_pTTree->Branch("ISREnergyAfterFit_woNu" , &m_ISREnergyAfterFit_woNu , "ISREnergyAfterFit_woNu/F" );
	m_pTTree->Branch("p1stAfterFit_woNu" , &m_p1stAfterFit_woNu , "p1stAfterFit_woNu/F" );
	m_pTTree->Branch("cos1stAfterFit_woNu" , &m_cos1stAfterFit_woNu , "cos1stAfterFit_woNu/F" );
	m_pTTree->Branch("FitProbability_woNu" , &m_FitProbability_woNu , "FitProbability_woNu/F" );
	m_pTTree->Branch("FitChi2_woNu" , &m_FitChi2_woNu , "FitChi2_woNu/F" );
	m_pTTree->Branch("FitChi2_byMass" , &m_FitChi2_byMass , "FitChi2_byMass/F" );

	m_pTTree->Branch("BestMatchingKinfit" , &m_bestMatchingKinfit );
	m_pTTree->Branch("BestMatchingByMass" , &m_bestMatchingByMass );

	m_pTTree->Branch("pullJetEnergy_woNu" , &m_pullJetEnergy_woNu );
	m_pTTree->Branch("pullJetTheta_woNu" , &m_pullJetTheta_woNu );
	m_pTTree->Branch("pullJetPhi_woNu" , &m_pullJetPhi_woNu );
	m_pTTree->Branch("pullLeptonInvPt_woNu" , &m_pullLeptonInvPt_woNu );
	m_pTTree->Branch("pullLeptonTheta_woNu" , &m_pullLeptonTheta_woNu );
	m_pTTree->Branch("pullLeptonPhi_woNu" , &m_pullLeptonPhi_woNu );

	/*	m_pTTree->Branch("FitErrorCode_wNu" , &m_FitErrorCode_wNu , "FitErrorCode_wNu/I" );
	m_pTTree->Branch("ZMassBeforeFit_wNu" , &m_ZMassBeforeFit_wNu , "ZMassBeforeFit_wNu/F" );
	m_pTTree->Branch("H1MassBeforeFit_wNu" , &m_H1MassBeforeFit_wNu , "H1MassBeforeFit_wNu/F" );
	m_pTTree->Branch("H2MassBeforeFit_wNu" , &m_H2MassBeforeFit_wNu , "H2MassBeforeFit_wNu/F" );
	m_pTTree->Branch("ZMassAfterFit_wNu" , &m_ZMassAfterFit_wNu , "ZMassAfterFit_wNu/F" );
	m_pTTree->Branch("H1MassAfterFit_wNu" , &m_H1MassAfterFit_wNu , "H1MassAfterFit_wNu/F" );
	m_pTTree->Branch("H2MassAfterFit_wNu" , &m_H2MassAfterFit_wNu , "H2MassAfterFit_wNu/F" );
	m_pTTree->Branch("FitProbability_wNu" , &m_FitProbability_wNu , "FitProbability_wNu/F" );
	m_pTTree->Branch("FitChi2_wNu" , &m_FitChi2_wNu , "FitChi2_wNu/F" );
	m_pTTree->Branch("pullJetEnergy_wNu" , &m_pullJetEnergy_wNu );
	m_pTTree->Branch("pullJetTheta_wNu" , &m_pullJetTheta_wNu );
	m_pTTree->Branch("pullJetPhi_wNu" , &m_pullJetPhi_wNu );
	m_pTTree->Branch("pullLeptonInvPt_wNu" , &m_pullLeptonInvPt_wNu );
	m_pTTree->Branch("pullLeptonTheta_wNu" , &m_pullLeptonTheta_wNu );
	m_pTTree->Branch("pullLeptonPhi_wNu" , &m_pullLeptonPhi_wNu );*/

	m_pTTree->Branch("FitErrorCode" , &m_FitErrorCode , "FitErrorCode/I" );
	m_pTTree->Branch("ZMassBeforeFit" , &m_ZMassBeforeFit , "ZMassBeforeFit/F" );
	m_pTTree->Branch("H1MassBeforeFit" , &m_H1MassBeforeFit , "H1MassBeforeFit/F" );
	m_pTTree->Branch("H2MassBeforeFit" , &m_H2MassBeforeFit , "H2MassBeforeFit/F" );
	m_pTTree->Branch("HHMassBeforeFit" , &m_HHMassBeforeFit , "HHMassBeforeFit/F" );
	m_pTTree->Branch("ZHHMassBeforeFit" , &m_ZHHMassBeforeFit , "ZHHMassBeforeFit/F" );
	m_pTTree->Branch("ISREnergyBeforeFit" , &m_ISREnergyBeforeFit , "ISREnergyBeforeFit/F" );

	m_pTTree->Branch("ZMassAfterFit" , &m_ZMassAfterFit , "ZMassAfterFit/F" );
	m_pTTree->Branch("H1MassAfterFit" , &m_H1MassAfterFit , "H1MassAfterFit/F" );
	m_pTTree->Branch("H2MassAfterFit" , &m_H2MassAfterFit , "H2MassAfterFit/F" );
	m_pTTree->Branch("HHMassAfterFit" , &m_HHMassAfterFit , "HHMassAfterFit/F" );
	m_pTTree->Branch("ZHHMassAfterFit" , &m_ZHHMassAfterFit , "ZHHMassAfterFit/F" );
	m_pTTree->Branch("ISREnergyAfterFit" , &m_ISREnergyAfterFit , "ISREnergyAfterFit/F" );
	m_pTTree->Branch("p1stAfterFit" , &m_p1stAfterFit , "p1stAfterFit/F" );
	m_pTTree->Branch("cos1stAfterFit" , &m_cos1stAfterFit , "cos1stAfterFit/F" );
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
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
  streamlog_out(MESSAGE1) << "	////////////////////////////////// processing event " << m_nEvt << " in run " << m_nRun << " /////////////////////////////////////////////////" << std::endl;
  streamlog_out(MESSAGE1) << "	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;

  updateChannelValues(pLCEvent);
  
  clearChannelValues();
  clearBaseValues();
};

void ZHHBaseKinfitProcessor::processRunHeader(LCRunHeader* run)
{
  (void) run;
	m_nRun++ ;
}

void ZHHBaseKinfitProcessor::check( LCEvent* event )
{
  (void) event;
}