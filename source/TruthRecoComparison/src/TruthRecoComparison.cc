#include "TruthRecoComparison.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <ctime>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCIntVec.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;

TruthRecoComparison aTruthRecoComparison ;

TruthRecoComparison::TruthRecoComparison() :

  Processor("TruthRecoComparison"),
  m_n_run(0),
  m_n_evt(0),
  m_n_evt_sum(0),
  m_error_code(ERROR_CODES::UNINITIALIZED)
{

	_description = "TruthRecoComparison writes relevant observables to root-file " ;

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				"inputPfoCollection",
				"Name of input pfo collection",
				m_inputPfoCollection,
				std::string("PandoraPFOs")
				);

	registerInputCollection(LCIO::LCRELATION,
				 "RecoMCTruthLink",
				 "Name of the RecoMCTruthLink input collection"  ,
				 m_recoMCTruthLink,
				 std::string("RecoMCTruthLink")
				 );

	registerInputCollection(LCIO::LCRELATION,
				"MCTruthRecoLink",
				"Name of the MCTruthRecoLink input collection"  ,
				m_mcTruthRecoLink,
				std::string("MCTruthRecoLink")
				);

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);

  	registerProcessorParameter("outputRootFilename",
				"name of output root file",
				m_outputRootFile,
				std::string("TruthKinematics.root")
				);

	registerProcessorParameter("ECM" ,
				"Center-of-Mass Energy in GeV",
				m_ECM,
				float(500.f)
				);
}

void TruthRecoComparison::init()
{
	streamlog_out(DEBUG) << "   init called  " << std::endl;
	this->clear();

	m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree", "eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_n_run, "run/I");
	m_pTTree->Branch("event", &m_n_evt, "event/I");
	m_pTTree->Branch("error_code", &m_error_code);

	m_pTTree->Branch("true_p_t", &m_true_p_t);
	m_pTTree->Branch("reco_p_t", &m_reco_p_t);

	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void TruthRecoComparison::clear() 
{
	streamlog_out(DEBUG) << "   clear called  " << std::endl;

	m_error_code = ERROR_CODES::UNKNOWN_ERROR;

	m_mcp_mom_tot.SetXYZT(0., 0., 0., 0.);
	m_mcp_mom_detected.SetXYZT(0., 0., 0., 0.);
	m_mcp_mom_undetected.SetXYZT(0., 0., 0., 0.);
	m_pfo_mom_tot.SetXYZT(0., 0., 0., 0.);

	updateKinematics();
}

void TruthRecoComparison::updateKinematics()
{
	m_mcp_E_tot = m_mcp_mom_tot.E();
	m_mcp_px_tot = m_mcp_mom_tot.Px();
	m_mcp_py_tot = m_mcp_mom_tot.Py();
	m_mcp_pz_tot = m_mcp_mom_tot.Pz();

	m_mcp_E_detected = m_mcp_mom_detected.E();
	m_mcp_px_detected = m_mcp_mom_detected.Px();
	m_mcp_py_detected = m_mcp_mom_detected.Py();
	m_mcp_pz_detected = m_mcp_mom_detected.Pz();

	m_mcp_E_undetected = m_mcp_mom_undetected.E();
	m_mcp_px_undetected = m_mcp_mom_undetected.Px();
	m_mcp_py_undetected = m_mcp_mom_undetected.Py();
	m_mcp_pz_undetected = m_mcp_mom_undetected.Pz();

	// Transverse energy

	m_true_Et = m_mcp_mom_detected.Et();
	m_reco_Et = m_pfo_mom_tot.Et();

	// Missing pT, mass and energy

	float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
	double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);

	TLorentzVector ecms(target_p_due_crossing_angle, 0., 0., E_lab) ;

	TLorentzVector p_miss_true = ecms - m_mcp_mom_detected;
	TLorentzVector p_miss_reco = ecms - m_pfo_mom_tot;

	m_true_pt_miss = p_miss_true.Pt();
	m_reco_pt_miss = p_miss_reco.Pt();

	m_true_m_miss = p_miss_true.M();
	m_reco_m_miss = p_miss_reco.M();

	m_true_E_miss = p_miss_true.E();
	m_reco_E_miss = p_miss_reco.E();
}

void TruthRecoComparison::processRunHeader( LCRunHeader*  /*run*/) { 
	m_n_run++ ;
} 

void TruthRecoComparison::processEvent( EVENT::LCEvent *pLCEvent )
{
	this->clear();
	
	streamlog_out(DEBUG)  << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << std::endl;
	
	m_n_run = pLCEvent->getRunNumber();
	m_n_evt = pLCEvent->getEventNumber();
	m_n_evt_sum++;

	try {
		LCCollection *inputPFOCollection;
		LCCollection *inputMCParticleCollection;

		streamlog_out(DEBUG) << "        getting PFO collection: " << m_inputPfoCollection << std::endl ;
		inputPFOCollection = pLCEvent->getCollection( m_inputPfoCollection );

		streamlog_out(DEBUG) << "        getting MCParticle collection: " << m_mcParticleCollection << std::endl ;
		inputMCParticleCollection = pLCEvent->getCollection( m_mcParticleCollection );

		streamlog_out(DEBUG) << "        getting RecoMCTruthLink relation: " << m_recoMCTruthLink << std::endl ;
		LCRelationNavigator RecoMCParticleNav = pLCEvent->getCollection(m_recoMCTruthLink);

		streamlog_out(DEBUG) << "        getting TruthRecoLink relation: " << m_mcTruthRecoLink << std::endl ;
      	LCRelationNavigator MCParticleRecoNav = pLCEvent->getCollection(m_mcTruthRecoLink);

		for (size_t i=0; i < inputMCParticleCollection->getNumberOfElements(); i++) {
			MCParticle* mcp = (MCParticle*) inputMCParticleCollection->getElementAt(i);

			if (mcp->getDaughters().size() == 0) {
				TLorentzVector mcp_mom (mcp->getMomentum() , mcp->getEnergy());
				m_mcp_mom_tot += mcp_mom;

				float weightPFOtoMCP = 0.0;
	  			float weightMCPtoPFO = 0.0;

				ReconstructedParticle* linkedPFO = getLinkedPFO( mcp , RecoMCParticleNav , MCParticleRecoNav , false , false , weightPFOtoMCP , weightMCPtoPFO );
				if ( linkedPFO == NULL ) {
					m_mcp_mom_detected += mcp_mom;
				} else {
					m_mcp_mom_undetected += mcp_mom;
				}
			}
		}

		for (size_t i=0; i < inputPFOCollection->getNumberOfElements(); i++) {
			ReconstructedParticle* pfo = (ReconstructedParticle*) inputPFOCollection->getElementAt(i);
			m_pfo_mom_tot += TLorentzVector( pfo->getMomentum() , pfo->getEnergy() );
		}

		updateKinematics();

	} catch(DataNotAvailableException &e) {
		m_error_code = ERROR_CODES::COLLECTION_NOT_FOUND;
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_n_evt << std::endl;
	}

	m_pTTree->Fill();
}

void TruthRecoComparison::check()
{
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}


void TruthRecoComparison::end()
{
	// Write ROOT file
	m_pTFile->cd();
	m_pTTree->Write();
	m_pTFile->Close();

	delete m_pTFile;
}

EVENT::ReconstructedParticle* TruthRecoComparison::getLinkedPFO( EVENT::MCParticle *mcParticle , LCRelationNavigator RecoMCParticleNav , LCRelationNavigator MCParticleRecoNav , bool getChargedPFO , bool getNeutralPFO , float &weightPFOtoMCP , float &weightMCPtoPFO )
{
  streamlog_out(DEBUG1) << "" << std::endl;
  streamlog_out(DEBUG1) << "Look for PFO linked to visible MCParticle:" << std::endl;

  ReconstructedParticle* linkedPFO{};
  bool foundlinkedPFO = false;
  const EVENT::LCObjectVec& PFOvec = MCParticleRecoNav.getRelatedToObjects( mcParticle );
  const EVENT::FloatVec&  PFOweightvec = MCParticleRecoNav.getRelatedToWeights( mcParticle );
  streamlog_out(DEBUG0) << "Visible MCParticle is linked to " << PFOvec.size() << " PFO(s)" << std::endl;
  weightPFOtoMCP = 0.0;
  weightMCPtoPFO = 0.0;
  double maxweightPFOtoMCP = 0.;
  double maxweightMCPtoPFO = 0.;
  int iPFOtoMCPmax = -1;
  int iMCPtoPFOmax = -1;
  for ( unsigned int i_pfo = 0; i_pfo < PFOvec.size(); i_pfo++ )
    {
      double pfo_weight = 0.0;
      double trackWeight = ( int( PFOweightvec.at( i_pfo ) ) % 10000 ) / 1000.0;
      double clusterWeight = ( int( PFOweightvec.at( i_pfo ) ) / 10000 ) / 1000.0;
      if ( getChargedPFO && !getNeutralPFO )
	{
	  pfo_weight = trackWeight;
	}
      else if ( getNeutralPFO && !getChargedPFO )
	{
	  pfo_weight = clusterWeight;
	}
      else
	{
	  pfo_weight = ( trackWeight > clusterWeight ? trackWeight : clusterWeight );
	}
      streamlog_out(DEBUG0) << "Visible MCParticle linkWeight to PFO: " << PFOweightvec.at( i_pfo ) << " (Track: " << trackWeight << " , Cluster: " << clusterWeight << ")" << std::endl;
      ReconstructedParticle *testPFO = (ReconstructedParticle *) PFOvec.at( i_pfo );
      if ( pfo_weight > maxweightMCPtoPFO )//&& track_weight >= m_MinWeightTrackMCTruthLink )
	{
	  maxweightMCPtoPFO = pfo_weight;
	  iMCPtoPFOmax = i_pfo;
	  streamlog_out(DEBUG0) << "PFO at index: " << testPFO->id() << " has TYPE: " << testPFO->getType() << " and MCParticle to PFO link weight is " << pfo_weight << std::endl;
	}
    }
  if ( getChargedPFO && maxweightMCPtoPFO < 0.8 )
    {
      streamlog_out(DEBUG1) << "MCParticle has link weight lower than 0.8 ( " << maxweightMCPtoPFO << " ), looking for linked PFO in clusters" << std::endl;
      for ( unsigned int i_pfo = 0; i_pfo < PFOvec.size(); i_pfo++ )
	{
	  double pfo_weight = ( int( PFOweightvec.at( i_pfo ) ) / 10000 ) / 1000.0;
	  streamlog_out(DEBUG0) << "Visible MCParticle linkWeight to PFO: " << PFOweightvec.at( i_pfo ) << " (Track: " << ( int( PFOweightvec.at( i_pfo ) ) % 10000 ) / 1000.0 << " , Cluster: " << ( int( PFOweightvec.at( i_pfo ) ) / 10000 ) / 1000.0 << ")" << std::endl;
	  ReconstructedParticle *testPFO = (ReconstructedParticle *) PFOvec.at( i_pfo );
	  if ( pfo_weight > maxweightMCPtoPFO )//&& track_weight >= m_MinWeightTrackMCTruthLink )
	    {
	      maxweightMCPtoPFO = pfo_weight;
	      iMCPtoPFOmax = i_pfo;
	      streamlog_out(DEBUG0) << "PFO at index: " << testPFO->id() << " has TYPE: " << testPFO->getType() << " and MCParticle to PFO link weight is " << pfo_weight << std::endl;
	    }
	}
    }
  if ( iMCPtoPFOmax != -1 )
    {
      ReconstructedParticle *testPFO = (ReconstructedParticle *) PFOvec.at( iMCPtoPFOmax );
      const EVENT::LCObjectVec& MCPvec = RecoMCParticleNav.getRelatedToObjects( testPFO );
      const EVENT::FloatVec&  MCPweightvec = RecoMCParticleNav.getRelatedToWeights( testPFO );
      for ( unsigned int i_mcp = 0; i_mcp < MCPvec.size(); i_mcp++ )
	{
	  double mcp_weight = 0.0;
	  double trackWeight = ( int( MCPweightvec.at( i_mcp ) ) % 10000 ) / 1000.0;
	  double clusterWeight = ( int( MCPweightvec.at( i_mcp ) ) / 10000 ) / 1000.0;
	  if ( getChargedPFO && !getNeutralPFO )
	    {
	      mcp_weight = trackWeight;
	    }
	  else if ( getNeutralPFO && !getChargedPFO )
	    {
	      mcp_weight = clusterWeight;
	    }
	  else
	    {
	      mcp_weight = ( trackWeight > clusterWeight ? trackWeight : clusterWeight );
	    }
	  MCParticle *testMCP = (MCParticle *) MCPvec.at( i_mcp );
	  if ( mcp_weight > maxweightPFOtoMCP )//&& mcp_weight >= m_MinWeightTrackMCTruthLink )
	    {
	      maxweightPFOtoMCP = mcp_weight;
	      iPFOtoMCPmax = i_mcp;
	      streamlog_out(DEBUG0) << "MCParticle at index: " << testMCP->id() << " has PDG: " << testMCP->getPDG() << " and PFO to MCParticle link weight is " << mcp_weight << std::endl;
	    }
	}
      if ( iPFOtoMCPmax != -1 )
	{
	  if ( MCPvec.at( iPFOtoMCPmax ) == mcParticle )
	    {
	      linkedPFO = testPFO;
	      foundlinkedPFO = true;
	    }
	}
    }

  if( foundlinkedPFO )
    {
      streamlog_out(DEBUG1) << "Linked PFO to MCParticle found successfully " << std::endl;
      weightPFOtoMCP = maxweightPFOtoMCP;
      weightMCPtoPFO = maxweightMCPtoPFO;
      return linkedPFO;
    }
  else
    {
      streamlog_out(DEBUG1) << "Couldn't Find a PFO linked to MCParticle" << std::endl;
      return NULL;
    }
}