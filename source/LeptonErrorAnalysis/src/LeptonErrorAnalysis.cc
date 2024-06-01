#include "LeptonErrorAnalysis.h"
#include <stdlib.h>
#include <cmath>
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


using namespace lcio;
using namespace marlin;
using namespace std;

LeptonErrorAnalysis aLeptonErrorAnalysis;

LeptonErrorAnalysis::LeptonErrorAnalysis() : Processor("LeptonErrorAnalysis"),
					     m_nRun(0),
					     m_nEvt(0),
					     m_Bfield(0),
					     c(0),
					     mm2m(0),
					     eV2GeV(0),
					     eB(0)

{

  // modify processor description
  _description = "LeptonErrorAnalysis does whatever it does ...";


  // register steering parameters: name, description, class-variable, default value


  // Inputs: MC-particles, Reco-particles, the link between the two
  registerInputCollection( LCIO::MCPARTICLE,
			   "MCParticleCollection" ,
			   "Name of the MCParticle collection"  ,
			   _MCParticleCollectionName ,
			   std::string("MCParticlesSkimmed")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoParticleCollection" ,
			   "Name of the ReconstructedParticles input collection"  ,
			   _recoParticleCollectionName ,
			   std::string("PandoraPFOs")
			   );

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			   "inputIsolepCollection",
			   "Name of input isolated lepton collection",
			   m_inputIsolepCollection,
			   std::string("IsolatedLeptons")
			   );
  
  registerInputCollection(LCIO::LCRELATION,
			  "TrackMCTruthLinkCollection",
			  "Name of input TrackMCTruthLink Collection",
			  m_TrackMCTruthLinkCollection,
			  std::string("MarlinTrkTracksMCTruthLink")
			  );

  registerInputCollection(LCIO::LCRELATION,
			  "MCTruthTrackLinkCollection",
			  "Name of input MCTruthTrackLink Collection",
			  m_MCTruthTrackLinkCollection,
			  std::string("MCTruthMarlinTrkTracksLink")
			  );

 // Outputs: Normalised Residuals
 registerOutputCollection( LCIO::LCFLOATVEC,                                                                                                            
			   "LeptonResidualsOutputCollection",                                                                                                
			   "Output LeptonResiduals (invPt, theta, phi)  Collection" ,                                                                           
			   _OutLeptonResidualsCol,                                                                                                          
			   std::string("LeptonResiduals"));
}


void LeptonErrorAnalysis::init() {
  streamlog_out(DEBUG6) << "   init called  " << std::endl;
  m_Bfield = MarlinUtil::getBzAtOrigin();
  //m_Bfield = 3.5;                                                                                
  streamlog_out(DEBUG0) << " BField =  "<< m_Bfield << " Tesla" << std::endl;
  c = 2.99792458e8;
  mm2m = 1e-3;
  eV2GeV = 1e-9;
  eB = m_Bfield * c * mm2m * eV2GeV; 
}

void LeptonErrorAnalysis::Clear()
{

}

void LeptonErrorAnalysis::processRunHeader()
{
  m_nRun++;
}


void LeptonErrorAnalysis::processEvent( LCEvent* pLCEvent)
{

  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  streamlog_out(DEBUG2) << "Processing event " << pLCEvent->getEventNumber() << std::endl;
  LCCollection *LeptonCol{};
  //this->Clear();
  try
    {
      LeptonCol= pLCEvent->getCollection( m_inputIsolepCollection );
      int n_Leptons = LeptonCol->getNumberOfElements();
      if (n_Leptons == 2) {
        LCCollectionVec *OutLeptonResidualsCol = new LCCollectionVec(LCIO::LCFLOATVEC);
	//Loop over isolated leptons
        for (int i_lep=0; i_lep<n_Leptons; i_lep++) {
	  LCFloatVec *LeptonResiduals = new LCFloatVec;
	  float leptonResiduals[ 3 ]{ 0.0 };
	  //for each lepton find corresponding MC particle and calculate the normalised residual
	  ReconstructedParticle* lepton = dynamic_cast<ReconstructedParticleImpl*>(LeptonCol->getElementAt(i_lep));
	  getLeptonResiduals(pLCEvent, lepton, leptonResiduals);

	  LeptonResiduals->push_back(leptonResiduals[0]);                                                                                                                                                 
	  LeptonResiduals->push_back(leptonResiduals[1]);                                                                                                                                                 
	  LeptonResiduals->push_back(leptonResiduals[2]); 

	  OutLeptonResidualsCol->addElement(LeptonResiduals);
	} 
	pLCEvent->addCollection(OutLeptonResidualsCol, _OutLeptonResidualsCol.c_str() );
      } 
      
      m_nEvt++;
    }
  catch(DataNotAvailableException &e)
    {
      streamlog_out(MESSAGE) << "Check : Input collections not found in event " << m_nEvt << std::endl;
    }
}


void LeptonErrorAnalysis::getLeptonResiduals( EVENT::LCEvent *pLCEvent ,EVENT::ReconstructedParticle* lepton, float(&leptonResiduals)[ 3 ])
{
  LCRelationNavigator navTrackMCTruth(pLCEvent->getCollection(m_TrackMCTruthLinkCollection));
  LCRelationNavigator navMCTruthTrack(pLCEvent->getCollection(m_MCTruthTrackLinkCollection));
  
  TrackVec trackVec = lepton->getTracks();
  int nTrackslepton = trackVec.size();
  //Chose only those that have exactly ONE track                                                                                                                                             
  if ( nTrackslepton != 1 ) {
    streamlog_out(MESSAGE)  << "Number of tracks for lepton is not exactly ONE!!! (nTracks = " << nTrackslepton << " ) " << std::endl;
  }
  else {
    bool LEPTONlinkedtoMCP = false;

    const EVENT::LCObjectVec& mcpvec = navTrackMCTruth.getRelatedToObjects(trackVec[0]);
    const EVENT::FloatVec&  mcpweightvec = navTrackMCTruth.getRelatedToWeights(trackVec[0]);
    MCParticle *linkedMCP;
    TLorentzVector mcpFourMomentum;
    double maxweightLEPTONtoMCP = 0.0;
    int iLEPTONtoMCPmax = -1;
    int iMCPtoLEPTONmax = -1;
    int n_mcp = mcpvec.size();
    for ( int i_mcp = 0; i_mcp < n_mcp; i_mcp++ ) {
      double mcp_weight = mcpweightvec.at(i_mcp);
      MCParticle *testMCP = (MCParticle *) mcpvec.at(i_mcp);
      streamlog_out(DEBUG) << "checking linked MCP at " << i_mcp << " , MCP PDG = " << testMCP->getPDG() << " , link weight = " << mcp_weight << std::endl;
      if ( mcp_weight > maxweightLEPTONtoMCP && mcp_weight >= 0.9 )
	{
	  maxweightLEPTONtoMCP = mcp_weight;
	  iLEPTONtoMCPmax = i_mcp;
	  streamlog_out(DEBUG) << "linkedMCP: " << i_mcp << " has PDG: " << testMCP->getPDG() << " and LEPTON to MCP Link has weight = " << mcp_weight << std::endl;
	}
    }
    
    if ( iLEPTONtoMCPmax != -1 ) {
      linkedMCP = (MCParticle *) mcpvec.at(iLEPTONtoMCPmax);
      streamlog_out(DEBUG) << "Found linked MCP, MCP PDG: " << linkedMCP->getPDG() << " , link weight = " << maxweightLEPTONtoMCP << std::endl;
      Track *testTrack;
      const EVENT::LCObjectVec& trackvec = navMCTruthTrack.getRelatedToObjects(linkedMCP);
      const EVENT::FloatVec&  trackweightvec = navMCTruthTrack.getRelatedToWeights(linkedMCP);
      double maxweightMCPtoLEPTON = 0.;
      for ( unsigned int i_track = 0; i_track < trackvec.size(); i_track++ )
	{
	  double Track_weight = trackweightvec.at(i_track);
	  testTrack = (Track *) trackvec.at(i_track);
	  if ( Track_weight > maxweightMCPtoLEPTON && Track_weight >= 0.9 )
	    {
	      maxweightMCPtoLEPTON = Track_weight;
	      iMCPtoLEPTONmax = i_track;
	    }
	}
      if ( iMCPtoLEPTONmax != -1 && testTrack == trackVec[0] )
	{
	  LEPTONlinkedtoMCP = true;
	}
    }
    
    if ( LEPTONlinkedtoMCP ) {
      //calculate residuals                                                                                                                                                                    
      mcpFourMomentum = TLorentzVector( linkedMCP->getMomentum()[0] , linkedMCP->getMomentum()[1] , linkedMCP->getMomentum()[2] , linkedMCP->getEnergy() );
      calculateResiduals( lepton, mcpFourMomentum, leptonResiduals);
    }
  }
}


//std::vector<float> LeptonErrorAnalysis::getLeptonResiduals( EVENT::ReconstructedParticle* lepton, TLorentzVector mcpFourMomentum )
void LeptonErrorAnalysis::calculateResiduals( EVENT::ReconstructedParticle* lepton, TLorentzVector mcpFourMomentum, float(&leptonResiduals)[ 3 ] )
{
  TrackVec trackVec = lepton->getTracks();
  if ( trackVec.size() != 1 ) return;

  float Omega= trackVec[ 0 ]->getOmega();
  float tanLambda= trackVec[ 0 ]->getTanLambda();
  float Theta= 2.0 * atan( 1.0 ) - atan( tanLambda );//atan( 1.0 / tanLambda );
  float Phi= trackVec[ 0 ]->getPhi();

  float sigmaOmega= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 5 ] );
  float sigmaTanLambda= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 14 ] );
  float sigmaPhi= std::sqrt( trackVec[ 0 ]->getCovMatrix()[ 2 ] );

  float dTheta_dTanLambda= -1.0 / ( 1.0 + std::pow( tanLambda , 2 ) );
  float leptonInvPt= Omega / eB;
  float leptonTheta= Theta;
  float leptonPhi= Phi;
  float sigmaInvPt= sigmaOmega / eB;
  float sigmaTheta= std::fabs( dTheta_dTanLambda ) * sigmaTanLambda;
  //    sigmaPhi defined above 

  double mcpPx = mcpFourMomentum.Px();
  double mcpPy = mcpFourMomentum.Py();
  double mcpPt = std::sqrt( pow( mcpPx , 2 ) + pow( mcpPy , 2 ) );
  double mcpTheta = mcpFourMomentum.Theta();
  double mcpPhi = mcpFourMomentum.Phi();

  double InvPtResidual = leptonInvPt - (1./mcpPt);
  double ThetaResidual = leptonTheta - mcpTheta;
  //double ThetaResidual = ( ( leptonTheta - mcpTheta ) > 0 ? acos( truePunit.Dot(recoProtated) ) : -1 * acos( truePunit.Dot(recoProtated) ) );
  double PhiResidual = leptonPhi - mcpPhi;
  //double PhiResidual = ( ( leptonPhi - mcpPhi ) > 0 ? acos( truePtunit.Dot(recoPtunit) ) : -1 * acos( truePtunit.Dot(recoPtunit) ) );
  
  float NormResidualInvPt = InvPtResidual / sigmaInvPt;
  float NormResidualTheta = ThetaResidual / sigmaTheta;
  float NormResidualPhi = PhiResidual / sigmaPhi;
  
  leptonResiduals[0] = NormResidualInvPt;
  leptonResiduals[1] = NormResidualTheta;
  leptonResiduals[2] = NormResidualPhi;
}


void LeptonErrorAnalysis::check()
{

}

void LeptonErrorAnalysis::end()
{

}
