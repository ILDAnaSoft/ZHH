#include "JetErrorAnalysis.h"
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

JetErrorAnalysis aJetErrorAnalysis ;

JetErrorAnalysis::JetErrorAnalysis() : Processor("JetErrorAnalysis"),
				       m_nRun(0),
				       m_nEvt(0),
				       m_nRunSum(0),
				       m_nEvtSum(0),
				       m_nTrueJets(0),
				       m_nRecoJets(0)

{

  // modify processor description
  _description = "JetErrorAnalysis does whatever it does ..." ;


  // register steering parameters: name, description, class-variable, default value


  // Inputs: MC-particles, Reco-particles, the link between the two

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoJetCollection" ,
			   "Name of the input Reconstructed Jet collection"  ,
			   m_recoJetCollectionName ,
			   string("Durham_nJets")
			   );

  registerInputCollection( LCIO::MCPARTICLE,
			   "MCParticleCollection" ,
			   "Name of the MCParticle collection"  ,
			   _MCParticleColllectionName ,
			   string("MCParticlesSkimmed")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoParticleCollection" ,
			   "Name of the ReconstructedParticles input collection"  ,
			   _recoParticleCollectionName ,
			   string("PandoraPFOs")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "RecoMCTruthLink",
			   "Name of the RecoMCTruthLink input collection"  ,
			   _recoMCTruthLink,
			   string("RecoMCTruthLink")
			   );

  // Inputs: True jets (as a recoparticle, will be the sum of the _reconstructed particles_
  // created by the true particles in each true jet, in the RecoMCTruthLink sense.
  // link jet-to-reco particles, link jet-to-MC-particles.

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "TrueJets" ,
			   "Name of the TrueJetCollection input collection",
			   _trueJetCollectionName ,
			   string("TrueJets")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "FinalColourNeutrals" ,
			   "Name of the FinalColourNeutralCollection input collection"  ,
			   _finalColourNeutralCollectionName ,
			   string("FinalColourNeutrals")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "InitialColourNeutrals" ,
			   "Name of the InitialColourNeutralCollection input collection"  ,
			   _initialColourNeutralCollectionName ,
			   string("InitialColourNeutrals")
			   );


  registerInputCollection( LCIO::LCRELATION,
			   "TrueJetPFOLink" ,
			   "Name of the TrueJetPFOLink input collection"  ,
			   _trueJetPFOLink,
			   string("TrueJetPFOLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "TrueJetMCParticleLink" ,
			   "Name of the TrueJetMCParticleLink input collection"  ,
			   _trueJetMCParticleLink,
			   string("TrueJetMCParticleLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "FinalElementonLink rueJetMCParticleLink" ,
			   "Name of the  FinalElementonLink input collection",
			   _finalElementonLink,
			   string("FinalElementonLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "InitialElementonLink" ,
			   "Name of the  InitialElementonLink input collection"  ,
			   _initialElementonLink,
			   string("InitialElementonLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			 "FinalColourNeutralLink" ,
			 "Name of the  FinalColourNeutralLink input collection"  ,
			 _finalColourNeutralLink,
			 string("FinalColourNeutralLink")
			 );

 registerInputCollection( LCIO::LCRELATION,
			  "InitialColourNeutralLink" ,
			  "Name of the  InitialColourNeutralLink input collection"  ,
			  _initialColourNeutralLink,
			  string("InitialColourNeutralLink")
			  );

 registerProcessorParameter("matchMethod",
			    "0 = by angular space, 1 = by leading particle",
			    m_matchMethod,
			    int(0)
			    );

 // Outputs: Normalised Residuals                                                                                                                          
 registerOutputCollection( LCIO::LCFLOATVEC,
                           "JetResidualsOutputCollection",
                           "Output JetResiduals (E, theta, phi)  Collection" ,
                           _OutJetResidualsCol,
                           string("JetResiduals"));

}


void JetErrorAnalysis::init()
{

streamlog_out(DEBUG6) << "   init called  " << endl ;

}

void JetErrorAnalysis::Clear()
{
  streamlog_out(DEBUG) << "   Clear called  " << endl;
  m_nTrueJets = 0;
  m_nRecoJets = 0;

}

void JetErrorAnalysis::processRunHeader()
{
  m_nRun++ ;
}


void JetErrorAnalysis::processEvent( LCEvent* pLCEvent)
{
  this->Clear();
  LCCollection *recoJetCol{};
  //LCCollection *trueJetCol{};
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  string trueJetType[6]{ "hadronic (string)" , "leptonic" , "hadronic(cluster)" , "ISR" , "overlay" , "M.E. photon" };
  string icnType[6]{ "quark pair" , "lepton pair" , "quark pair" , "ISR" , "???" , "M.E. photon" };
  streamlog_out(MESSAGE) << "" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////Processing event: " << m_nEvt << "////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;

  try {
    recoJetCol= pLCEvent->getCollection( m_recoJetCollectionName );
    TrueJet_Parser* trueJet= this;
    trueJet->getall(pLCEvent);
    
    m_nRecoJets = recoJetCol->getNumberOfElements();
    streamlog_out(DEBUG3) << "Number of Reconstructed Jets: " << m_nRecoJets << endl;
    
    int njets = trueJet->njets();
    streamlog_out(DEBUG3) << "Number of True Jets: " << njets << endl;
    //vector<int> trueHadronicJetIndices; trueHadronicJetIndices.clear();
    //vector<int> recoJetIndices; recoJetIndices.clear();
    for (int i_jet = 0 ; i_jet < njets ; i_jet++ ) {
      if ( type_jet( i_jet ) == 1 ) {
	++m_nTrueJets;
	//	trueHadronicJetIndices.push_back( i_jet );
      }
    }
    streamlog_out(DEBUG3) << "Number of True Hadronic Jets(type = 1): " << m_nTrueJets << endl;
    if ( m_nRecoJets == m_nTrueJets ) {

      LCCollectionVec *OutJetResidualsCol = new LCCollectionVec(LCIO::LCFLOATVEC);

      vector<ReconstructedParticle*> recoJets; //figure out m_nRecoJets def, might need to change to vector 
      vector<vector<double>> jetResiduals;
      for (unsigned int i=0; i<m_nRecoJets; ++i) {
	recoJets.push_back((ReconstructedParticle*) recoJetCol->getElementAt(i));
	jetResiduals.push_back({0.0,0.0,0.0});
      }
      bool foundTrueJets = true;
      if (m_matchMethod == 0) {
	getJetResidualsByAngularSpace(pLCEvent, recoJets, jetResiduals);
      }
      if (m_matchMethod == 1) {
	foundTrueJets = getJetResidualsByLeadingParticle(pLCEvent, recoJets, jetResiduals);
      }
      if (foundTrueJets) {
	for (unsigned int i=0; i<m_nRecoJets; ++i) {
	  LCFloatVec *JetResiduals = new LCFloatVec;
	  JetResiduals->push_back(jetResiduals[i][0]);
	  JetResiduals->push_back(jetResiduals[i][1]);
	  JetResiduals->push_back(jetResiduals[i][2]); 
	  OutJetResidualsCol->addElement(JetResiduals);
	}
      }
      pLCEvent->addCollection(OutJetResidualsCol, _OutJetResidualsCol.c_str() );
    
      /*
	  vector<int> arr(m_nRecoJets);
	  
	  // Setting values
	  for (int i_array = 0; i_array < m_nRecoJets; i_array++) {
	    arr[i_array] = i_array;
	  }
	  sort(arr.begin(),arr.begin()+m_nRecoJets);
	  streamlog_out(DEBUG) << "Array of indices: [ " << endl;
	  for (int i_array = 0; i_array < m_nRecoJets; i_array++) streamlog_out(DEBUG)<< arr[i_array] << " ";
	  streamlog_out(DEBUG) << "]" << endl;

	  float SmallestSumCosAngle = 99999.0;
	  vector<int> matchedRecoJetIndices(m_nRecoJets);
	  do {
	    float sumcosangle = 0.0;

	    streamlog_out(DEBUG) << "Permutation of indices: [ "<< endl;
	    for (int i_array = 0; i_array < m_nRecoJets; i_array++) streamlog_out(DEBUG)<< arr[i_array] << " ";
	    streamlog_out(DEBUG) << "]" << endl;

	    for ( int i_Jet = 0 ; i_Jet < m_nTrueJets ; ++i_Jet ) {
	      TVector3 trueJetMomentum(ptrueseen(trueHadronicJetIndices[i_Jet])[0], ptrueseen(trueHadronicJetIndices[i_Jet])[1], ptrueseen(trueHadronicJetIndices[i_Jet])[2]);
	      TVector3 trueJetMomentumUnit = trueJetMomentum; trueJetMomentumUnit.SetMag(1.0);
	      ReconstructedParticle *recoJet = dynamic_cast<ReconstructedParticle*>( recoJetCol->getElementAt( arr[i_Jet] ) );
	      TVector3 recoJetMomentum( recoJet->getMomentum() );
	      TVector3 recoJetMomentumUnit = recoJetMomentum; recoJetMomentumUnit.SetMag(1.0);
	      streamlog_out(DEBUG) << "true momentum = (" << trueJetMomentum(0) << ", " << trueJetMomentum(1) << ", " << trueJetMomentum(2) << ")" << endl;
	      streamlog_out(DEBUG) << "reco momentum = (" << recoJetMomentum(0) << ", " << recoJetMomentum(1) << ", " << recoJetMomentum(2) << ")" << endl;
	      streamlog_out(DEBUG) << "angle = " << acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit )) << endl;
	      sumcosangle += acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit ));
	    }
	    streamlog_out(DEBUG) << "Sum of angles: " << sumcosangle << endl;
	    if (sumcosangle<SmallestSumCosAngle) {
	      SmallestSumCosAngle = sumcosangle;
	      for (int i_array = 0; i_array < m_nRecoJets; i_array++)
		{
		  matchedRecoJetIndices[i_array] = arr[i_array];
		}
	    }
	  } while (next_permutation(arr.begin(),arr.begin()+m_nRecoJets));


	  for ( int i_Jet = 0 ; i_Jet < m_nTrueJets ; ++i_Jet )
	    {
	      TVector3 trueJetMomentum( ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] , ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] , ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] );
	      streamlog_out(DEBUG2) << "True(seen) Jet Momentum[ " << trueHadronicJetIndices[ i_Jet ] << " ]: (" << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] << " , " << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] << " , " << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] << ")" << endl;
	      TVector3 trueJetMomentumUnit = trueJetMomentum; trueJetMomentumUnit.SetMag(1.0);
	      streamlog_out(DEBUG2) << "True(seen) Jet [ " << trueHadronicJetIndices[ i_Jet ] << " ] is matched with RecoJet [ " << matchedRecoJetIndices[i_Jet] << " ]" << endl;
	      recoJetIndices.push_back( matchedRecoJetIndices[i_Jet] );
	      TLorentzVector trueJetFourMomentum( p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] , p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] , p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 3 ] , p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] );
	      ReconstructedParticle *recoJet = dynamic_cast<ReconstructedParticle*>( recoJetCol->getElementAt( matchedRecoJetIndices[i_Jet] ) );
	      TLorentzVector recoJetFourMomentum( recoJet->getMomentum()[ 0 ] , recoJet->getMomentum()[ 1 ] , recoJet->getMomentum()[ 2 ] , recoJet->getEnergy() );
	      double recoJetPx = recoJetFourMomentum.Px();
	      double recoJetPy = recoJetFourMomentum.Py();
	      double recoJetPz = recoJetFourMomentum.Pz();
	      double recoJetPt = sqrt( pow( recoJetPx , 2 ) + pow( recoJetPy , 2 ) );
	      double recoJetPt2 = pow( recoJetPx , 2 ) + pow( recoJetPy , 2 );
	      double recoJetP2 = pow( recoJetPx , 2 ) + pow( recoJetPy , 2 ) + pow( recoJetPz , 2 );
	      double recoJetTheta = recoJetFourMomentum.Theta();
              double recoJetPhi = recoJetFourMomentum.Phi();

	      double sigmaPx2 = recoJet->getCovMatrix()[ 0 ];
	      double sigmaPxPy = recoJet->getCovMatrix()[ 1 ];
	      double sigmaPy2 = recoJet->getCovMatrix()[ 2 ];
	      double sigmaPxPz = recoJet->getCovMatrix()[ 3 ];
	      double sigmaPyPz = recoJet->getCovMatrix()[ 4 ];
	      double sigmaPz2 = recoJet->getCovMatrix()[ 5 ];
	      double dTheta_dPx = recoJetPx * recoJetPz / ( recoJetP2 * recoJetPt );
	      double dTheta_dPy = recoJetPy * recoJetPz / ( recoJetP2 * recoJetPt );
	      double dTheta_dPz = -recoJetPt / recoJetP2;
	      double dPhi_dPx = -recoJetPy / recoJetPt2;
	      double dPhi_dPy = recoJetPx / recoJetPt2; //correction added to Yasser's calculations
	      double sigmaTheta = sqrt( fabs( sigmaPx2 * pow( dTheta_dPx , 2 ) + sigmaPy2 * pow( dTheta_dPy , 2 ) + sigmaPz2 * pow( dTheta_dPz , 2 ) + 2 * ( sigmaPxPy * dTheta_dPx * dTheta_dPy ) +2 * ( sigmaPxPz * dTheta_dPx * dTheta_dPz ) + 2 * ( sigmaPyPz * dTheta_dPy * dTheta_dPz ) ) );
	      double sigmaPhi = sqrt( fabs( sigmaPx2 * pow( dPhi_dPx , 2 ) + sigmaPy2 * pow( dPhi_dPy , 2 ) + 2 * ( sigmaPxPy * dPhi_dPx * dPhi_dPy ) ) );
	      streamlog_out(DEBUG7) << "Reco jet energy = " << recoJetFourMomentum.E() << "True jet energy = " << trueJetFourMomentum.E() << "sigma = " << recoJet->getCovMatrix()[ 9 ] << endl;

	      double trueJetPx = trueJetFourMomentum.Px();
              double trueJetPy = trueJetFourMomentum.Py();
              double trueJetPz = trueJetFourMomentum.Pz();
              //double trueJetE = trueJetFourMomentum.E();                                                                                                                      
              double trueJetTheta = trueJetFourMomentum.Theta();
              double trueJetPhi = trueJetFourMomentum.Phi();
              TVector3 trueP( trueJetPx , trueJetPy , trueJetPz );
              TVector3 truePunit = trueP; truePunit.SetMag(1.0);
              TVector3 truePt( trueJetPx , trueJetPy , 0.0 );
              TVector3 truePtunit = truePt; truePtunit.SetMag(1.0);

              TVector3 recoP( recoJetPx , recoJetPy , recoJetPz );
              TVector3 recoPunit = recoP; recoPunit.SetMag(1.0);
              TVector3 recoProtated = recoP; recoProtated.SetMag(1.0); recoProtated.SetPhi( trueJetPhi );
              TVector3 recoPt( recoJetPx , recoJetPy , 0.0 );
              TVector3 recoPtunit = recoPt; recoPtunit.SetMag(1.0);

	      double ThetaResidual = ( ( recoJetTheta - trueJetTheta ) > 0 ? acos( truePunit.Dot(recoProtated) ) : -1 * acos( truePunit.Dot(recoProtated) ) );

              double PhiResidual = ( ( recoJetPhi - trueJetPhi ) > 0 ? acos( truePtunit.Dot(recoPtunit) ) : -1 * acos( truePtunit.Dot(recoPtunit) ) );

	      float pullE = (recoJetFourMomentum.E() - trueJetFourMomentum.E() ) / sqrt( recoJet->getCovMatrix()[ 9 ] );
	      //float pullTheta = (recoJetFourMomentum.Theta() - trueJetFourMomentum.Theta() ) / sigmaTheta;
	      //float pullPhi = (recoJetFourMomentum.Phi() - trueJetFourMomentum.Phi() ) / sigmaPhi ;
	      float pullTheta = ThetaResidual / sigmaTheta;
              float pullPhi = PhiResidual / sigmaPhi ;
	      PullE->push_back(pullE);
	      PullTheta->push_back(pullTheta);
	      PullPhi->push_back(pullPhi);
	    }
	  OutputPullECol->addElement(PullE);
	  OutputPullThetaCol->addElement(PullTheta);
	  OutputPullPhiCol->addElement(PullPhi);
	  pLCEvent->addCollection(OutputPullECol, _OutPullECol.c_str() );
	  pLCEvent->addCollection(OutputPullThetaCol, _OutPullThetaCol.c_str() );
	  pLCEvent->addCollection(OutputPullPhiCol, _OutPullPhiCol.c_str() );
      */
    }
    m_nEvtSum++;
    m_nEvt++ ;
  }
  catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "Check : Input collections not found in event " << m_nEvt << endl;
  }

}

void JetErrorAnalysis::getJetResidualsByAngularSpace( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<vector<double>> &JetResiduals)
{

  assert(recoJets.size()==m_nRecoJets);
  vector<vector<double>> tempResiduals;

  TrueJet_Parser* trueJet = this;
  trueJet->getall( pLCEvent );

  int njets = trueJet->njets();
  vector<int> trueHadronicJetIndices; 
  vector<int> recoJetIndices; 
  for (int i_jet = 0 ; i_jet < njets ; i_jet++ ) {
    if ( type_jet( i_jet ) == 1 ) {
      trueHadronicJetIndices.push_back( i_jet );
    }
  }
  
  vector<int> arr(m_nRecoJets);
  for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++) {
    arr[i_array] = i_array;
  }
  sort(arr.begin(),arr.begin()+m_nRecoJets);
  streamlog_out(DEBUG) << "Array of indices: [ " << endl;
  for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++) streamlog_out(DEBUG)<< arr[i_array] << " ";
  streamlog_out(DEBUG) << "]" << endl;
  
  float SmallestSumCosAngle = 99999.0;
  vector<int> matchedRecoJetIndices(m_nRecoJets);
  do {
    float sumcosangle = 0.0;
    
    streamlog_out(DEBUG) << "Permutation of indices: [ "<< endl;
    for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++) streamlog_out(DEBUG)<< arr[i_array] << " ";
    streamlog_out(DEBUG) << "]" << endl;
    
    for (unsigned int i_Jet = 0 ; i_Jet < m_nRecoJets ; ++i_Jet ) {
      TVector3 trueJetMomentum(ptrueseen(trueHadronicJetIndices[i_Jet])[0], ptrueseen(trueHadronicJetIndices[i_Jet])[1], ptrueseen(trueHadronicJetIndices[i_Jet])[2]);
      TVector3 trueJetMomentumUnit = trueJetMomentum; trueJetMomentumUnit.SetMag(1.0);
      TVector3 recoJetMomentum( recoJets.at(arr[i_Jet])->getMomentum() );
      TVector3 recoJetMomentumUnit = recoJetMomentum; recoJetMomentumUnit.SetMag(1.0);
      streamlog_out(DEBUG) << "true momentum = (" << trueJetMomentum(0) << ", " << trueJetMomentum(1) << ", " << trueJetMomentum(2) << ")" << endl;
      streamlog_out(DEBUG) << "reco momentum = (" << recoJetMomentum(0) << ", " << recoJetMomentum(1) << ", " << recoJetMomentum(2) << ")" << endl;
      streamlog_out(DEBUG) << "angle = " << acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit )) << endl;
      sumcosangle += acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit ));
    }
    streamlog_out(DEBUG) << "Sum of angles: " << sumcosangle << endl;
    if (sumcosangle<SmallestSumCosAngle) {
      SmallestSumCosAngle = sumcosangle;
      for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++)
	{
	  matchedRecoJetIndices[i_array] = arr[i_array];
	}
    }
  } while (next_permutation(arr.begin(),arr.begin()+m_nRecoJets));
  
  for (unsigned int i_Jet = 0 ; i_Jet < m_nRecoJets ; ++i_Jet ) {
    streamlog_out(DEBUG2) << "True(seen) Jet Momentum[ " << trueHadronicJetIndices[ i_Jet ] << " ]: (" 
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] << " , " 
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] << " , "
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] << ")" << endl;
    streamlog_out(DEBUG2) << "True(seen) Jet [ " << trueHadronicJetIndices[ i_Jet ] << " ] is matched with RecoJet [ " << matchedRecoJetIndices[i_Jet] << " ]" << endl;
    recoJetIndices.push_back( matchedRecoJetIndices[i_Jet] );
    TLorentzVector trueJetFourMomentum( p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] , 
					p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] , 
					p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 3 ] , 
					p4trueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] );
    vector<double> jetResiduals( 3, 0.0);
    calculateResiduals( recoJets.at(matchedRecoJetIndices[i_Jet]), trueJetFourMomentum, jetResiduals);
    tempResiduals.push_back(jetResiduals);
  }
  assert (tempResiduals.size() == JetResiduals.size() );
  JetResiduals = tempResiduals;
  
}

bool JetErrorAnalysis::getJetResidualsByLeadingParticle( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<vector<double>> &JetResiduals)
{
  assert(recoJets.size()==m_nRecoJets);
  vector<vector<double>> tempResiduals;

  TrueJet_Parser* trueJet = this;
  trueJet->getall( pLCEvent );

  streamlog_out(DEBUG0) << " looking for leading particles in jets " << endl;
  for (unsigned int i=0; i<m_nRecoJets; ++i) {
    ReconstructedParticle* leadingParticleJet = nullptr;
    float leadingEnergyJet = 0.0;

    streamlog_out(DEBUG0) << " Jet[" << i << "] has "<< ( recoJets.at(i)->getParticles() ).size() << " particles" << endl;    
    for ( unsigned int i_par = 0 ; i_par < ( recoJets.at(i)->getParticles() ).size() ; ++i_par ) {
      ReconstructedParticle* pfo = ( ReconstructedParticle* )recoJets.at(i)->getParticles()[ i_par ];
      if ( abs( pfo->getType() ) == 12 || abs( pfo->getType() ) == 14 || abs( pfo->getType() ) == 16 ) continue;
      if ( pfo->getEnergy() > leadingEnergyJet ) {
	leadingParticleJet = pfo;
	leadingEnergyJet = pfo->getEnergy();
	streamlog_out(DEBUG0) << " So far, the energy of leading particle is: " << leadingEnergyJet << " GeV" << endl;
      }
    }
    if (leadingParticleJet == nullptr) {
      return false;
    }
    LCObjectVec jetvec = reltjreco->getRelatedFromObjects( leadingParticleJet );
    streamlog_out(DEBUG0) << jetvec.size() << " true Jet found for leading particle of jet[" << i << "]" << endl;
    if (jetvec.size() == 0) {
      return false;
    }   
    int trueJet_index = jetindex( dynamic_cast<ReconstructedParticle*>( jetvec[ 0 ] ) ); //truejet function
    streamlog_out(DEBUG0) << " true Jet[ " << trueJet_index << " ] has the leading particle of jet[" << i << "]" << endl;
    streamlog_out(DEBUG4) << "----------------------------------------------------------------------" << endl;
    streamlog_out(DEBUG4) << "    trueJet[" << i << "] TYPE:  " << type_jet( trueJet_index ) << endl;
    streamlog_out(DEBUG4) << "    trueJet[" << i << "] (Px,Py,Pz,E):  " 
			  << ptrue( trueJet_index )[ 0 ] << " , " 
			  << ptrue( trueJet_index )[ 1 ] << " , " 
			  << ptrue( trueJet_index )[ 2 ] << " , " 
			  << Etrue( trueJet_index ) << endl;
    streamlog_out(DEBUG4) << "----------------------------------------------------------------------" << endl;

    TLorentzVector trueJetFourMomentum( ptrue( trueJet_index )[ 0 ] , ptrue( trueJet_index )[ 1 ] , ptrue( trueJet_index )[ 2 ], Etrue( trueJet_index ) );
    vector<double> jetResiduals( 3, 0.0);
    calculateResiduals( recoJets.at(i), trueJetFourMomentum, jetResiduals);
    tempResiduals.push_back(jetResiduals);
  }
  assert (tempResiduals.size() == JetResiduals.size() );
  JetResiduals = tempResiduals;

  return true;
}

void JetErrorAnalysis::calculateResiduals( EVENT::ReconstructedParticle* recoJet, TLorentzVector trueJetFourMomentum, vector<double>&jetResiduals ) 
{
  TLorentzVector recoJetFourMomentum( recoJet->getMomentum()[ 0 ] , recoJet->getMomentum()[ 1 ] , recoJet->getMomentum()[ 2 ], recoJet->getEnergy() );
  double recoJetPx = recoJetFourMomentum.Px();
  double recoJetPy = recoJetFourMomentum.Py();
  double recoJetPz = recoJetFourMomentum.Pz();
  double recoJetPt2 = pow( recoJetPx , 2 ) + pow( recoJetPy , 2 );
  double recoJetPt = sqrt( recoJetPt2 );
  double recoJetP2 = pow( recoJetPx , 2 ) + pow( recoJetPy , 2 ) + pow( recoJetPz , 2 );
  
  double sigmaPx2 = recoJet->getCovMatrix()[ 0 ];
  double sigmaPxPy = recoJet->getCovMatrix()[ 1 ];
  double sigmaPy2 = recoJet->getCovMatrix()[ 2 ];
  double sigmaPxPz = recoJet->getCovMatrix()[ 3 ];
  double sigmaPyPz = recoJet->getCovMatrix()[ 4 ];
  double sigmaPz2 = recoJet->getCovMatrix()[ 5 ];

  double dTheta_dPx = recoJetPx * recoJetPz / ( recoJetP2 * recoJetPt );
  double dTheta_dPy = recoJetPy * recoJetPz / ( recoJetP2 * recoJetPt );
  double dTheta_dPz = -recoJetPt / recoJetP2;
  double dPhi_dPx = -recoJetPy / recoJetPt2;
  double dPhi_dPy = recoJetPx / recoJetPt2; //correction added to Yasser's calculations

  double sigmaE = sqrt( recoJet->getCovMatrix()[ 9 ] );
  double sigmaTheta = sqrt( fabs( sigmaPx2 * pow( dTheta_dPx , 2 ) + sigmaPy2 * pow( dTheta_dPy , 2 ) + sigmaPz2 * pow( dTheta_dPz , 2 ) + 
					    2.0 * ( sigmaPxPy * dTheta_dPx * dTheta_dPy ) + 2.0 * ( sigmaPxPz * dTheta_dPx * dTheta_dPz ) + 2.0 * ( sigmaPyPz * dTheta_dPy * dTheta_dPz ) ) );
  double sigmaPhi = sqrt( fabs( sigmaPx2 * pow( dPhi_dPx , 2 ) + sigmaPy2 * pow( dPhi_dPy , 2 ) + 2.0 * ( sigmaPxPy * dPhi_dPx * dPhi_dPy ) ) );

  TVector3 jetTrueMomentumUnit(trueJetFourMomentum.Px(), trueJetFourMomentum.Py(), trueJetFourMomentum.Pz()); 
  jetTrueMomentumUnit.SetMag( 1.0 );
  TVector3 jetTruePtUnit(trueJetFourMomentum.Px(), trueJetFourMomentum.Py(), 0.0); 
  jetTruePtUnit.SetMag( 1.0 );
  TVector3 jetRecoMomentumUnitRotated(recoJetFourMomentum.Px(), recoJetFourMomentum.Py(), recoJetFourMomentum.Pz()); 
  jetRecoMomentumUnitRotated.SetMag( 1.0 );
  jetRecoMomentumUnitRotated.SetPhi( trueJetFourMomentum.Phi() ); 
  TVector3 jetRecoPtUnit( recoJetFourMomentum.Px() , recoJetFourMomentum.Py() , 0.0 ); 
  jetRecoPtUnit.SetMag( 1.0 );

  double jetEnergyResidual = recoJetFourMomentum.E() - trueJetFourMomentum.E();
  double jetThetaResidual = ( recoJetFourMomentum.Theta() >= trueJetFourMomentum.Theta() ? acos( jetTrueMomentumUnit.Dot( jetRecoMomentumUnitRotated ) ) : -1.0 * acos( jetTrueMomentumUnit.Dot( jetRecoMomentumUnitRotated ) ) );
  double jetPhiResidual = ( recoJetFourMomentum.Phi() >= trueJetFourMomentum.Phi() ? acos( jetTruePtUnit.Dot( jetRecoPtUnit ) ) : -1.0 * acos( jetTruePtUnit.Dot( jetRecoPtUnit ) ) );

  double NormResidualEnergy = jetEnergyResidual / sigmaE;
  double NormResidualTheta = jetThetaResidual / sigmaTheta;
  double NormResidualPhi = jetPhiResidual /sigmaPhi;

  assert(jetResiduals.size() == 3);
  jetResiduals.at(0) = NormResidualEnergy;
  jetResiduals.at(1) = NormResidualTheta;
  jetResiduals.at(2) = NormResidualPhi;

}

void JetErrorAnalysis::check()
{

}

void JetErrorAnalysis::end()
{

}
