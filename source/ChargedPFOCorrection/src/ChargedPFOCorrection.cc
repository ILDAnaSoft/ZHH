#include "ChargedPFOCorrection.h"
#include <iostream>
#include <EVENT/LCCollection.h>
#include "EVENT/LCCollection.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/MCParticle.h"
#include "EVENT/ReconstructedParticle.h"
#include <IMPL/ReconstructedParticleImpl.h>
#include "IMPL/ParticleIDImpl.h"
#include "UTIL/PIDHandler.h"
#include "marlin/VerbosityLevels.h"
#include <GeometryUtil.h>
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH1I.h"
#include "TH2I.h"
#include "TF1.h"
#include "TTree.h"
#include "TPaveStats.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TRatioPlot.h"
#include "TAxis.h"
#include "TLine.h"

using namespace lcio ;
using namespace marlin ;

ChargedPFOCorrection aChargedPFOCorrection;

ChargedPFOCorrection::ChargedPFOCorrection() :

Processor("ChargedPFOCorrection"),
m_Bfield(0.f),
c(0.),
mm2m(0.),
eV2GeV(0.),
eB(0.),
m_MinWeightTrackMCTruthLink(0.9),
m_MinWeightMCTruthTrackLink(0.9),
m_pion_mass(0.13957018),
m_proton_mass(0.938272088),
m_kaon_mass(0.493677)
{
	_description = "ChargedPFOCorrection creates new RECONSTRUCTEDPARTICLE collection that PFOs are updated using tracks refitted with true mass for protons and kaons";

	registerInputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"PfoCollection",
					"Name of input pfo collection",
					m_inputPfoCollection,
					std::string("PandoraPFOs")
				);

	registerInputCollection(	LCIO::TRACK,
					"MarlinTrkTracksCollection" ,
					"Name of the MarlinTrkTracks collection"  ,
					m_MarlinTrkTracks,
					std::string("MarlinTrkTracks")
				);

	registerInputCollection(	LCIO::TRACK,
					"MarlinTrkTracksCollectionKaon" ,
					"Name of the MarlinTrkTracks collection"  ,
					m_MarlinTrkTracksKAON,
					std::string("MarlinTrkTracksKaon")
				);

	registerInputCollection(	LCIO::TRACK,
					"MarlinTrkTracksCollectionProton" ,
					"Name of the MarlinTrkTracks collection"  ,
					m_MarlinTrkTracksPROTON,
					std::string("MarlinTrkTracksProton")
				);

	registerInputCollection(	LCIO::LCRELATION,
					"TrackMCTruthLinkCollection",
					"Name of input TrackMCTruthLink Collection",
					m_TrackMCTruthLinkCollection,
					std::string("MarlinTrkTracksMCTruthLink")
				);

	registerInputCollection(	LCIO::LCRELATION,
					"MCTruthTrackLinkCollection",
					"Name of input MCTruthTrackLink Collection",
					m_MCTruthTrackLinkCollection,
					std::string("MCTruthMarlinTrkTracksLink")
				);

	registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"updatedPfoCollection",
					"Name of output pfo collection",
					m_outputPfoCollection,
					std::string("updatedChargedPFOs")
				);

	registerProcessorParameter(	"updatePFOs",
					"Update PFOs? true update PFO collection , false: make new PFO collection",
					m_updatePFOs,
					bool(true)
				);

	registerProcessorParameter(	"updatePFOwithOneTrack",
					"Update PFOs with one track.",
					m_updatePFOwithOneTrack,
					bool(true)
				);

	registerProcessorParameter(	"updatePFOwithTwoTrack",
					"Update PFOs with two track.",
					m_updatePFOwithTwoTrack,
					bool(true)
				);

	registerProcessorParameter(	"mainTrackSelectionScenario",
					"Scenario for chossing main track in Charged PFOs with two track: 1: track with lowes radius of inner most hit , 2: track with lowset 3D impaxt parameter , any: track with highest magnitude of momentum",
					m_mainTrackSelectionScenario,
					int(1)
				);

	registerProcessorParameter(	"updatePFOwithMoreTrack",
					"Update PFOs with More than Two track.",
					m_updatePFOwithMoreTrack,
					bool(true)
				);

	registerProcessorParameter(	"MinWeightTrackMCTruthLink" ,
					"Minimum acceptable weight for Track -> MCParticle Link"  ,
					m_MinWeightTrackMCTruthLink ,
					float(0.9f)
				);

	registerProcessorParameter(	"MinWeightMCTruthTrackLink" ,
					"Minimum acceptable weight for MCParticle -> Track Link"  ,
					m_MinWeightMCTruthTrackLink ,
					float(0.9f)
				);

	registerProcessorParameter(	"PionMass" ,
					"Pion mass for calculating (p,E) and CovMat from track parameters"  ,
					m_pion_mass ,
					float(0.13957018)
				);

	registerProcessorParameter(	"ProtonMass" ,
					"Proton mass for calculating (p,E) and CovMat from track parameters"  ,
					m_proton_mass ,
					float(0.938272088)
				);

	registerProcessorParameter(	"KaonMass" ,
					"Kaon mass for calculating (p,E) and CovMat from track parameters"  ,
					m_kaon_mass ,
					float(0.493677)
				);

}

void ChargedPFOCorrection::init()
{
	streamlog_out(DEBUG) << "	init called  " << std::endl;
//	m_Bfield = MarlinUtil::getBzAtOrigin();
	m_Bfield = 3.5;
	c = 2.99792458e8;
	mm2m = 1e-3;
	eV2GeV = 1e-9;
	eB = m_Bfield * c * mm2m * eV2GeV;
	printParameters();
	streamlog_out(DEBUG) << "	BField =  "<< m_Bfield << " Tesla" << std::endl ;

}

void ChargedPFOCorrection::Clear()
{

}

void ChargedPFOCorrection::processRunHeader()
{

}

void ChargedPFOCorrection::processEvent( EVENT::LCEvent *pLCEvent )
{

	LCCollection *inputPfoCollection{};
	LCCollection *MarlinTrkTracks{};
	LCCollection *MarlinTrkTracksKAON{};
	LCCollection *MarlinTrkTracksPROTON{};
	IMPL::LCCollectionVec* outputPfoCollection(NULL);
	outputPfoCollection = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
//	outputPfoCollection->setSubset( true );
//	if ( m_updatePFOs ) outputPfoCollection->setSubset( true );
	int n_PFO = -1;
	int n_TRK = -1;
	int n_TRKp = -1;
	int n_TRKk = -1;
	this->Clear();
	streamlog_out(MESSAGE) << "" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////	Processing event 	" << pLCEvent->getEventNumber() << "	////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;

        try
        {
		inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
		MarlinTrkTracks = pLCEvent->getCollection(m_MarlinTrkTracks);
		MarlinTrkTracksKAON = pLCEvent->getCollection(m_MarlinTrkTracksKAON);
		MarlinTrkTracksPROTON = pLCEvent->getCollection(m_MarlinTrkTracksPROTON);

		n_PFO = inputPfoCollection->getNumberOfElements();
		n_TRK = MarlinTrkTracks->getNumberOfElements();
		n_TRKk = MarlinTrkTracksKAON->getNumberOfElements();
		n_TRKp = MarlinTrkTracksPROTON->getNumberOfElements();
		if ( n_PFO == -1 ) streamlog_out(DEBUG4) << "	Input PFO collection (" << m_inputPfoCollection << ") has no element (PFO) " << std::endl;
		if ( n_TRK == -1 ) streamlog_out(DEBUG4) << "	Input TRACK collection (" << m_MarlinTrkTracks << ") has no element (Track) " << std::endl;
		if ( n_TRK != n_TRKk ) streamlog_out(DEBUG4) << "	Input TRACK collection (" << m_MarlinTrkTracksKAON << ") has in-equal number of elements with Main Track Collection (" << m_MarlinTrkTracks << ")!" << std::endl;
		if ( n_TRK != n_TRKp ) streamlog_out(DEBUG4) << "	Input TRACK collection (" << MarlinTrkTracksPROTON << ") has in-equal number of elements with Main Track Collection (" << m_MarlinTrkTracks << ")!" << std::endl;

		streamlog_out(DEBUG4) << "	Total Number of PFOs: " << n_PFO << std::endl;
		streamlog_out(DEBUG4) << "	Total Number of Tracks: " << n_TRK << std::endl;
		streamlog_out(DEBUG4) << "	Total Number of KaonTracks: " << n_TRKk << std::endl;
		streamlog_out(DEBUG4) << "	Total Number of ProtonTracks: " << n_TRKp << std::endl;

		for (int i_pfo = 0; i_pfo < n_PFO ; ++i_pfo)
		{
			ReconstructedParticleImpl* outputPFO = dynamic_cast<ReconstructedParticleImpl*>( inputPfoCollection->getElementAt( i_pfo ) );
			ReconstructedParticleImpl* newPFO = new ReconstructedParticleImpl;
			int pfoType = outputPFO->getType();
			double trackMass = 0.0;
			int TrackID = outputPFO->getType();
			int TrackIndex = -1;
			std::vector<float> newPFOCovMat( 10, 0.0 );
			std::vector<float> trackFourMomentumCovMat( 10, 0.0 );
			bool m_updatePFO = false;
			TLorentzVector pfoFourMomentum( 0.0 , 0.0 , 0.0 , 0.0 );
			const EVENT::TrackVec& inputPFOtrkvec = outputPFO->getTracks();
			Track *refittedTrack = NULL;
			std::vector<Track*> outputPFOtrkvec;
			int nTRKsofPFO = inputPFOtrkvec.size();

			if ( nTRKsofPFO == 0 )
			{
				m_updatePFO = false;
			}
			else if ( nTRKsofPFO == 1 )
			{
				Track *defaultTrk = (Track*)inputPFOtrkvec.at(0);
				TrackID = getTruthTrkID( pLCEvent, defaultTrk );
				TrackIndex = getTrackIndex( MarlinTrkTracks , defaultTrk );
				if ( TrackID == 0 ) m_updatePFO = false;
				streamlog_out(DEBUG2) << "	TrackID: 	" << TrackID << std::endl;
				streamlog_out(DEBUG2) << "	TrackIndex: 	" << TrackIndex << std::endl;

				if ( abs( TrackID ) == 2212 )
				{
					refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksPROTON->getElementAt( TrackIndex ) );
					outputPFOtrkvec.push_back( refittedTrack );
					trackMass = m_proton_mass;
					m_updatePFO = m_updatePFOwithOneTrack;
					streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with proton mass" << std::endl;
				}
				else if ( abs( TrackID ) == 321 )
				{
					refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksKAON->getElementAt( TrackIndex ) );
					outputPFOtrkvec.push_back( refittedTrack );
					trackMass = m_kaon_mass;
					m_updatePFO = m_updatePFOwithOneTrack;
					streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with kaon mass" << std::endl;
				}
				else
				{
					refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracks->getElementAt( TrackIndex ) );
					outputPFOtrkvec.push_back( refittedTrack );
					trackMass = m_pion_mass;
					m_updatePFO = m_updatePFOwithPionTrack;
					streamlog_out(DEBUG2) << "	Standard track is used for updating PFO" << std::endl;
				}

				pfoFourMomentum = getTrackFourMomentum( refittedTrack , trackMass );
				newPFOCovMat = getChargedPFOCovMat( refittedTrack , trackMass );
			}
			else if ( nTRKsofPFO == 2 && m_updatePFOwithTwoTrack )
			{
				if ( outputPFO->getCharge() == 0.0 )
				{
					for ( int i_trk = 0 ; i_trk < nTRKsofPFO ; ++i_trk )
					{
						streamlog_out(DEBUG2) << "	---------------------" << std::endl;
						streamlog_out(DEBUG2) << "	Adding Track " << i_trk << " to PFO " << std::endl;
						streamlog_out(DEBUG2) << "	---------------------" << std::endl;
						Track *inputTrk = (Track*)inputPFOtrkvec.at( i_trk );
						TrackID = getTruthTrkID( pLCEvent, inputTrk );
						TrackIndex = this->getTrackIndex( MarlinTrkTracks , inputTrk );
						streamlog_out(DEBUG2) << "	TrackID: 	" << TrackID << std::endl;
						streamlog_out(DEBUG2) << "	TrackIndex: 	" << TrackIndex << std::endl;

						if ( abs( TrackID ) == 2212 )
						{
							refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksPROTON->getElementAt( TrackIndex ) );
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_proton_mass;
							m_updatePFO = true;
							streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with proton mass" << std::endl;
						}
						else if ( abs( TrackID ) == 321 )
						{
							refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksKAON->getElementAt( TrackIndex ) );
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_kaon_mass;
							m_updatePFO = true;
							streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with kaon mass" << std::endl;
						}
						else
						{
							refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracks->getElementAt( TrackIndex ) );
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_pion_mass;
							if ( !m_updatePFO ) m_updatePFO = false;
							streamlog_out(DEBUG2) << "	Standard track is used for updating PFO" << std::endl;
						}
						pfoFourMomentum += getTrackFourMomentum( refittedTrack , trackMass );
						trackFourMomentumCovMat = getChargedPFOCovMat( refittedTrack , trackMass );
						for ( int i_element = 0 ; i_element < 10 ; ++i_element ) newPFOCovMat[ i_element ] += trackFourMomentumCovMat[ i_element ];
					}
				}
				else
				{
					float radiusInnerMostHit = 10000.0;
					float firstHitRef = 1000000.0;
					float ImpactParameter3D = 1000000.0;
					Track *mainTrk = NULL;
					double mainTrkMass = 0.0;
					int mainTrackID = 0;
					for ( int i_trk = 0 ; i_trk < nTRKsofPFO ; ++i_trk )
					{
						Track *inputTrk = (Track*)inputPFOtrkvec.at( i_trk );
						TrackID = getTruthTrkID( pLCEvent, inputTrk );
						TrackIndex = this->getTrackIndex( MarlinTrkTracks , inputTrk );
						streamlog_out(DEBUG5) << "	Investigating track " << i_trk << " of PFO" << std::endl;
						streamlog_out(DEBUG5) << *inputTrk << std::endl;
						if ( abs( TrackID ) == 2212 )
						{
							refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksPROTON->getElementAt( TrackIndex ) );
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_proton_mass;
							m_updatePFO = true;
							streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with proton mass" << std::endl;
						}
						else if ( abs( TrackID ) == 321 )
						{
							refittedTrack = dynamic_cast<EVENT::Track*>( MarlinTrkTracksKAON->getElementAt( TrackIndex ) );
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_kaon_mass;
							m_updatePFO = true;
							streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with kaon mass" << std::endl;
						}
						else
						{
							refittedTrack = inputTrk;
							outputPFOtrkvec.push_back( refittedTrack );
							trackMass = m_pion_mass;
							if ( !m_updatePFO ) m_updatePFO = false;
							streamlog_out(DEBUG2) << "	Standard track is used for updating PFO" << std::endl;
						}
						if ( m_mainTrackSelectionScenario == 1 )
						{
							if ( refittedTrack->getRadiusOfInnermostHit() <= radiusInnerMostHit )
							{
								mainTrk = refittedTrack;
								mainTrkMass = trackMass;
								mainTrackID = TrackID;
								radiusInnerMostHit = inputTrk->getRadiusOfInnermostHit();
							}
						}
						else if ( m_mainTrackSelectionScenario == 2 )
						{
							if ( sqrt( pow( refittedTrack->getD0() , 2 ) + pow( refittedTrack->getZ0() , 2 ) ) <= ImpactParameter3D )
							{
								mainTrk = refittedTrack;
								mainTrkMass = trackMass;
								mainTrackID = TrackID;
								ImpactParameter3D = sqrt( pow( refittedTrack->getD0() , 2 ) + pow( refittedTrack->getZ0() , 2 ) );
							}
						}
						else
						{
							streamlog_out(DEBUG5) << "	Investigating track state at first hit" << std::endl;
							TrackState *trackStateAtFirstHit = inputTrk->getTrackStates()[ 1 ];
							streamlog_out(DEBUG5) << *trackStateAtFirstHit << std::endl;
							if ( sqrt( pow( trackStateAtFirstHit->getReferencePoint()[ 0 ] , 2 ) + pow( trackStateAtFirstHit->getReferencePoint()[ 1 ] , 2 ) + pow( trackStateAtFirstHit->getReferencePoint()[ 2 ] , 2 ) ) <= firstHitRef )
							{
								mainTrk = refittedTrack;
								mainTrkMass = trackMass;
								mainTrackID = TrackID;
								firstHitRef = sqrt( pow( trackStateAtFirstHit->getReferencePoint()[ 0 ] , 2 ) + pow( trackStateAtFirstHit->getReferencePoint()[ 1 ] , 2 ) + pow( trackStateAtFirstHit->getReferencePoint()[ 2 ] , 2 ) );
							}
						}
					}
					streamlog_out(DEBUG5) << "	Main track of PFO" << std::endl;
					streamlog_out(DEBUG5) << *mainTrk << std::endl;
					pfoFourMomentum = getTrackFourMomentum( mainTrk , mainTrkMass );
					newPFOCovMat = getChargedPFOCovMat( mainTrk , mainTrkMass );
					TrackID = mainTrackID;
/*
					TrackID = getTruthTrkID( pLCEvent, mainTrk );
					TrackIndex = this->getTrackIndex( MarlinTrkTracks , mainTrk );
					streamlog_out(DEBUG6) << "	TrackID: 	" << TrackID << std::endl;
					streamlog_out(DEBUG6) << "	TrackIndex: 	" << TrackIndex << std::endl;
					if ( abs( TrackID ) == 2212 )
					{
						mainTrk = dynamic_cast<EVENT::Track*>( MarlinTrkTracksPROTON->getElementAt( TrackIndex ) );
						trackMass = m_proton_mass;
						m_updatePFO = true;
						streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with proton mass" << std::endl;
					}
					else if ( abs( TrackID ) == 321 )
					{
						mainTrk = dynamic_cast<EVENT::Track*>( MarlinTrkTracksKAON->getElementAt( TrackIndex ) );
						trackMass = m_kaon_mass;
						m_updatePFO = true;
						streamlog_out(DEBUG2) << "	Standard track is replaced with track refitted with kaon mass" << std::endl;
					}
					else
					{
						trackMass = m_pion_mass;
						m_updatePFO = false;
						streamlog_out(DEBUG2) << "	Standard track is used for updating PFO" << std::endl;
					}
//					m_updatePFO = false;
*/				}
			}
			else
			{
				m_updatePFO = false;
			}
			if ( nTRKsofPFO == 1 || ( nTRKsofPFO == 2 && fabs( outputPFO->getCharge() ) >=0.5 ) )
			{
				pfoType = TrackID;
			}

			double Momentum[3]{ pfoFourMomentum.Px() , pfoFourMomentum.Py() , pfoFourMomentum.Pz() };
			double Energy = pfoFourMomentum.E();
			double Mass = pfoFourMomentum.M();
			if ( nTRKsofPFO == 1 && m_updatePFO && Energy > 0.1 )
			{
				outputPFO->setType( pfoType );
				outputPFO->setMomentum( Momentum );
				outputPFO->setEnergy( Energy );
				outputPFO->setCovMatrix( newPFOCovMat );
				outputPFO->setMass( Mass );
			}
			else if ( nTRKsofPFO == 2 && Energy > 0.1 )
			{
				if ( m_updatePFO )
				{
					outputPFO->setType( pfoType );
					outputPFO->setMomentum( Momentum );
					outputPFO->setEnergy( Energy );
					outputPFO->setCovMatrix( newPFOCovMat );
					outputPFO->setMass( Mass );
				}
				else
				{
					outputPFO->setCovMatrix( newPFOCovMat );
				}
			}
			streamlog_out(DEBUG8) << "	UPDATED PFO:" << std::endl;
			streamlog_out(DEBUG8) << *outputPFO << std::endl;
//			outputPfoCollection->addElement( outputPFO );

			newPFO->setType( outputPFO->getType() );
			newPFO->setMomentum( Momentum );
			newPFO->setEnergy( Energy );
			newPFO->setCovMatrix( newPFOCovMat );
			newPFO->setMass( Mass );
			newPFO->setCharge( outputPFO->getCharge() );
			newPFO->setReferencePoint( outputPFO->getReferencePoint() );
			for ( unsigned int j = 0 ; j < outputPFO->getParticleIDs().size() ; ++j )
			{
				ParticleIDImpl* inPID = dynamic_cast<ParticleIDImpl*>( outputPFO->getParticleIDs()[ j ] );
				ParticleIDImpl* outPID = new ParticleIDImpl;
				outPID->setType( inPID->getType() );
				outPID->setPDG( inPID->getPDG() );
				outPID->setLikelihood( inPID->getLikelihood() );
				outPID->setAlgorithmType( inPID->getAlgorithmType() ) ;
				for ( unsigned int k = 0 ; k < inPID->getParameters().size() ; ++k ) outPID->addParameter( inPID->getParameters()[ k ] );
				newPFO->addParticleID( outPID );
			}
			newPFO->setParticleIDUsed( outputPFO->getParticleIDUsed() );
			newPFO->setGoodnessOfPID( outputPFO->getGoodnessOfPID() );
			for ( unsigned int j = 0 ; j < outputPFO->getParticles().size() ; ++j )
			{
				newPFO->addParticle( outputPFO->getParticles()[ j ] );
			}
			for ( unsigned int j = 0 ; j < outputPFO->getClusters().size() ; ++j )
			{
				newPFO->addCluster( outputPFO->getClusters()[ j ] );
			}
			for ( unsigned int j = 0 ; j < outputPFOtrkvec.size() ; ++j)
			{
				newPFO->addTrack( outputPFOtrkvec[ j ] );
			}
			newPFO->setStartVertex( outputPFO->getStartVertex() );
//			if ( m_updatePFOs )
//			{
//				outputPfoCollection->addElement( outputPFO );
//			}
//			else
//			{
//				outputPfoCollection->addElement( newPFO );
//			}
			outputPfoCollection->addElement( newPFO );
		}
		pLCEvent->addCollection( outputPfoCollection , m_outputPfoCollection );
	}
	catch(DataNotAvailableException &e)
        {
        	streamlog_out(MESSAGE) << "	Input collection not found in event " << pLCEvent->getEventNumber() << std::endl;
        }

}

int ChargedPFOCorrection::getTruthTrkID( EVENT::LCEvent *pLCEvent , EVENT::Track* inputTrk )
{
	int TrackID = 0;
	MCParticle *linkedMCP{};
	try
	{
		LCRelationNavigator TrackMCParticleNav( pLCEvent->getCollection( m_TrackMCTruthLinkCollection ) );
		LCRelationNavigator MCParticleTrackNav( pLCEvent->getCollection( m_MCTruthTrackLinkCollection ) );
	}
	catch (DataNotAvailableException &e)
	{
		streamlog_out(WARNING) << "Could not find the LCRelationNavigator for Track <-> MCParticle" << std::endl;
		return TrackID;
	}
	LCRelationNavigator TrackMCParticleNav( pLCEvent->getCollection( m_TrackMCTruthLinkCollection ) );
	const EVENT::LCObjectVec& mcpvec = TrackMCParticleNav.getRelatedToObjects(inputTrk);
	const EVENT::FloatVec&  mcpweightvec = TrackMCParticleNav.getRelatedToWeights(inputTrk);
	double maxweightTRKtoMCP = 0.;
	double maxweightMCPtoTRK = 0.;
	int iTRKtoMCPmax = -1;
	for ( unsigned int i_mcp = 0; i_mcp < mcpvec.size(); i_mcp++ )
	{
		double mcp_weight = mcpweightvec.at(i_mcp);
		MCParticle *testMCP = (MCParticle *) mcpvec.at(i_mcp);
		if ( mcp_weight > maxweightTRKtoMCP )//&& mcp_weight >= m_MinWeightTrackMCTruthLink )
		{
			maxweightTRKtoMCP = mcp_weight;
			iTRKtoMCPmax = i_mcp;
			streamlog_out(DEBUG0) << "	MCParticle at index: " << i_mcp << " has PDG: " << testMCP->getPDG() << " and MCParticle to Track link weight is " << mcp_weight << std::endl;
		}
	}
	if ( iTRKtoMCPmax != -1 )
	{
		MCParticle *testMCP = (MCParticle *) mcpvec.at(iTRKtoMCPmax);
		streamlog_out(DEBUG0) << "	MCParticle at index: " << iTRKtoMCPmax << " with PDG code: " << testMCP->getPDG() << " has highest link weight to input track" << std::endl;
		LCRelationNavigator MCParticleTrackNav( pLCEvent->getCollection( m_MCTruthTrackLinkCollection ) );
		const EVENT::LCObjectVec& trkvec = MCParticleTrackNav.getRelatedToObjects(testMCP);
		const EVENT::FloatVec&  trkweightvec = MCParticleTrackNav.getRelatedToWeights(testMCP);
		int iMCPtoTRKmax = -1;
		for ( unsigned int i_trk = 0; i_trk < trkvec.size(); i_trk++ )
		{
			double track_weight = trkweightvec.at( i_trk );
			if ( track_weight > maxweightMCPtoTRK )//&& track_weight >= m_MinWeightTrackMCTruthLink )
			{
				maxweightMCPtoTRK = track_weight;
				iMCPtoTRKmax = i_trk;
				streamlog_out(DEBUG0) << "	Track at index: " << i_trk << " has highest link weight to MCParticle = " << track_weight << std::endl;
			}
		}
		if ( iMCPtoTRKmax != -1 )
		{
			Track *linkedTrack = (Track *) trkvec.at( iMCPtoTRKmax );
			if ( linkedTrack == inputTrk )
			{
				streamlog_out(DEBUG0) << "	Track linked to MCParticle with highest weight" << std::endl;
				linkedMCP = (MCParticle *) mcpvec.at(iTRKtoMCPmax);
				if ( maxweightTRKtoMCP >= m_MinWeightTrackMCTruthLink && maxweightMCPtoTRK >= m_MinWeightMCTruthTrackLink ) TrackID = linkedMCP->getPDG();
			}
		}
	}
	return TrackID;
}

int ChargedPFOCorrection::getTrackIndex( EVENT::LCCollection *TrackCollection , EVENT::Track* inputTrk )
{
	LCCollection *MarlinTrkTracks{};
	try
	{
		MarlinTrkTracks = TrackCollection;//pLCEvent->getCollection( TrackCollection );
	}
	catch (DataNotAvailableException &e)
	{
		streamlog_out(WARNING) << "	Could not find the " << TrackCollection << " Collection" << std::endl;
		return -1;
	}
	unsigned int nTRKs = MarlinTrkTracks->getNumberOfElements();
	int track_index = -1;
	for (unsigned int i_trk = 0; i_trk < nTRKs;  ++i_trk )
	{
		Track* track = dynamic_cast<EVENT::Track*>( MarlinTrkTracks->getElementAt( i_trk ) );
		if ( track == inputTrk )
		{
			track_index = i_trk;
		}
	}
	if ( track_index == -1)
	{
		streamlog_out(DEBUG1) << "	Coudln't find track_index!!!  " << std::endl;
	}
	else
	{
		streamlog_out(DEBUG1) << "	Track index in " << m_MarlinTrkTracks << " collection is " << track_index << std::endl;
	}
	return track_index;
}

TLorentzVector ChargedPFOCorrection::getTrackFourMomentum( EVENT::Track* inputTrk , double trackMass )
{
	streamlog_out(DEBUG1) << "	------------------------------------------------" << std::endl;
	streamlog_out(DEBUG1) << "	Calculating PFO 4-momentum from track parameters" << std::endl;
	streamlog_out(DEBUG1) << "	------------------------------------------------" << std::endl;
	double Phi = inputTrk->getPhi();
	double Omega = inputTrk->getOmega();
	double tanLambda = inputTrk->getTanLambda();
	streamlog_out(DEBUG0) << "	Track parameters obtained" << std::endl;
	double pT = eB / fabs( Omega );
	double px = pT * TMath::Cos( Phi );
	double py = pT * TMath::Sin( Phi );
	double pz = pT * tanLambda;
	double E = sqrt( pow( trackMass , 2 ) + px * px + py * py + pz * pz);
	streamlog_out(DEBUG0) << "	Track parameters is converted to (p,E)" << std::endl;
	TLorentzVector trackFourMomentum( px , py , pz , E );
	return trackFourMomentum;
}

std::vector<float> ChargedPFOCorrection::getChargedPFOCovMat( EVENT::Track* inputTrk , float trackMass )
{

//	Obtain covariance matrix on (px,py,pz,E) from the
//	covariance matrix on track parameters.
//
//	define the jacobian as the 3x4 matrix:
//
//
//
//			Dpx/DTanLambda		Dpy/DTanLambda		Dpz/DTanLambda		DE/DTanLambda
//
//	 J =		Dpx/DPhi		Dpy/DPhi		Dpz/DPhi		DE/DPhi
//
//			Dpx/DOmega		Dpy/DOmega		Dpz/DOmega		DE/DOmega
//
//
//
//			0			0			Pt			Pz.Pt/E
//
//	J =		-Py			Px			0			0
//
//			-Px/Omega		-Py/Omega		-Pz/Omega		-P2/E.Omega
//
//
//
//	Order in the covariance matrix on helix parameters:
//
//			TanLambda.TanLambda	TanLambda.phi		TanLambda.Omega
//
//	Cov =		phi.TanLambda		phi.phi			phi.Omega
//
//			Omega.TanLambda		Omega.phi		Omega.Omega
//
//
//

	streamlog_out(DEBUG1) << "	-------------------------------------------------------" << std::endl;
	streamlog_out(DEBUG1) << "	Calculating PFO CovMat from track parameters and CovMat" << std::endl;
	streamlog_out(DEBUG1) << "	-------------------------------------------------------" << std::endl;
	const int rows			= 3; // n rows jacobian
	const int columns		= 4; // n columns jacobian
	const int kspace_time_dim	= 4;

	TMatrixD covMatrixMomenta(kspace_time_dim,kspace_time_dim);
	std::vector<float> covP;

	double trackTanLambda		=	inputTrk->getTanLambda();
	double trackPhi			=	inputTrk->getPhi();
	double trackOmega		=	inputTrk->getOmega();
	std::vector<float> trackCovMat 	=	inputTrk->getCovMatrix();
	double trackPt			=	eB / fabs( trackOmega );
	double trackPx			= 	trackPt * TMath::Cos( trackPhi );
	double trackPy			= 	trackPt * TMath::Sin( trackPhi );
	double trackPz			= 	trackPt * trackTanLambda;
	double trackP			= 	std::sqrt( pow( trackPt , 2 ) + pow( trackPz , 2 ) );
	double trackE			= 	std::sqrt( pow( trackP , 2 ) + pow( trackMass , 2 ) );

	float SigmaPhi2			=	trackCovMat[  2 ];
	float SigmaPhiSigmaOmega	=	trackCovMat[  4 ];
	float SigmaOmega2		=	trackCovMat[  5 ];
	float SigmaTanLambdaSigmaPhi	=	trackCovMat[ 11 ];
	float SigmaTanLambdaSigmaOmega 	=	trackCovMat[ 12 ];
	float SigmaTanLambda2		=	trackCovMat[ 14 ];

	streamlog_out(DEBUG0) << "	Track parametrs and CovMat obtained" << std::endl;

//	Define array with jacobian matrix elements by rows
	double jacobian_by_rows[rows*columns] =
	{
		0			,	0			,	trackPt			,	trackPz * trackPt / trackE			,
		-trackPy		,	trackPx		,	0				,	0						,
		-trackPx / trackOmega	,	-trackPy / trackOmega	,	-trackPz / trackOmega		,	-trackP * trackP / ( trackE * trackOmega )
	};

	streamlog_out(DEBUG0) << "	Jacobian array formed by rows" << std::endl;

//	construct the Jacobian using previous array ("F" if filling by columns, "C", if filling by rows, $ROOTSYS/math/matrix/src/TMatrixT.cxx)
	TMatrixD jacobian(rows,columns, jacobian_by_rows, "C");
	streamlog_out(DEBUG0) << "	Jacobian array converted to Jacobian matrix" << std::endl;

//	track covariance matrix by rows
	double track_cov_matrix_by_rows[rows*rows] =
	{
		SigmaTanLambda2			,	SigmaTanLambdaSigmaPhi		,	SigmaTanLambdaSigmaOmega	,
		SigmaTanLambdaSigmaPhi		,	SigmaPhi2			,	SigmaPhiSigmaOmega	,
		SigmaTanLambdaSigmaOmega	,	SigmaPhiSigmaOmega		,	SigmaOmega2
	};
	streamlog_out(DEBUG0) << "	Track covariance matrix array formed by rows" << std::endl;

	TMatrixD covMatrix_track(rows,rows, track_cov_matrix_by_rows, "C");
	streamlog_out(DEBUG0) << "	Track CovMat array converted to track covariance matrix" << std::endl;

	covMatrixMomenta.Mult( TMatrixD( jacobian ,
					TMatrixD::kTransposeMult ,
					covMatrix_track) ,
					jacobian
					);
	streamlog_out(DEBUG0) << "	Track covariance matrix converted to FourMomentum Covariance matrix" << std::endl;

	covP.push_back( covMatrixMomenta(0,0) ); // x-x
	covP.push_back( covMatrixMomenta(1,0) ); // y-x
	covP.push_back( covMatrixMomenta(1,1) ); // y-y
	covP.push_back( covMatrixMomenta(2,0) ); // z-x
	covP.push_back( covMatrixMomenta(2,1) ); // z-y
	covP.push_back( covMatrixMomenta(2,2) ); // z-z
	covP.push_back( covMatrixMomenta(3,0) ); // e-x
	covP.push_back( covMatrixMomenta(3,1) ); // e-y
	covP.push_back( covMatrixMomenta(3,2) ); // e-z
	covP.push_back( covMatrixMomenta(3,3) ); // e-e
	streamlog_out(DEBUG0) << "	FourMomentum CovarianceMatrix Filled succesfully" << std::endl;

	return covP;

}

void ChargedPFOCorrection::check( EVENT::LCEvent *pLCEvent )
{
	LCCollection *inputPfoCollection{};
	LCCollection *outputPfoCollection{};
	try
	{
		inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
		outputPfoCollection = pLCEvent->getCollection(m_outputPfoCollection);
		int n_inputPFOs = inputPfoCollection->getNumberOfElements();
		int n_outputPFOs = outputPfoCollection->getNumberOfElements();
		streamlog_out(DEBUG4) << "	CHECK : processed event: " << pLCEvent->getEventNumber() << " (Number of inputPFOS: " << n_inputPFOs << " , Number of outputPFOs: " << n_outputPFOs <<")" << std::endl;
	}
	catch(DataNotAvailableException &e)
        {
          streamlog_out(MESSAGE) << "	Input/Output collection not found in event " << pLCEvent->getEventNumber() << std::endl;
        }
}
void ChargedPFOCorrection::end()
{

	streamlog_out(MESSAGE) << " " << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	///////////////////////////	processed all events 	////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << " " << std::endl;

}
