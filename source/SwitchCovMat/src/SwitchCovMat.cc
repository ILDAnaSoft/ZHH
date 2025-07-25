#include "SwitchCovMat.h"
#include <iostream>
#include <EVENT/LCCollection.h>
#include "EVENT/LCCollection.h"
#include "IMPL/LCCollectionVec.h"
#include <UTIL/LCRelationNavigator.h>
#include "EVENT/MCParticle.h"
#include "EVENT/Cluster.h"
#include "EVENT/ReconstructedParticle.h"
#include <IMPL/ReconstructedParticleImpl.h>
#include "IMPL/ParticleIDImpl.h"
#include "UTIL/PIDHandler.h"
#include "marlin/VerbosityLevels.h"
#include "TVector3.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH2I.h"
#include "TTree.h"

using namespace lcio ;
using namespace marlin ;
using namespace std ;

SwitchCovMat aSwitchCovMat;

SwitchCovMat::SwitchCovMat() :

Processor("SwitchCovMat")
{
	_description = "Set the convariance matrix in (P,E) for all pfos (charged particles, neutral hadrons and photons)";

	registerInputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"inputPfoCollection",
					"Name of input pfo collection",
					m_inputPfoCollection,
					std::string("PandoraPFOs")
				);

	registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"outputPfoCollection",
					"Name of output pfo collection",
					m_outputPfoCollection,
					std::string("updatedNeutralPFOs")
				);

	registerProcessorParameter(	"AssumeNeutralPFOMassive",
					"true: Neutral PFOs are taken massive, false: Neutral PFOs are taken massless",
					m_AssumeNeutralPFOMassive,
					bool(true)
				);
	registerProcessorParameter(	"isClusterEnergyKinEnergy",
					"true: the cluster energy is interpreted as kinetic energy of PFO, false: the cluster energy is interpreted as momentum magnitude of PFO",
					m_isClusterEnergyKinEnergy,
					bool(false)
				);

	registerProcessorParameter(	"updatePFO4Momentum",
					"true: Update 4-momentum of PFOs, false: set 4-momentum for PFOs same as input PFO",
					m_updatePFO4Momentum,
					bool(false)
				);

	registerProcessorParameter(	"useTrueJacobian",
					"true: Use (mathematically) true Jacobian for the option E_cluster = |p|, false: for the option E_cluster = |p|, Use the same jacobian as the option E_cluster = E_kinetic",
					m_useTrueJacobian,
					bool(false)
				);

}

void SwitchCovMat::init()
{

	streamlog_out(MESSAGE) << "   init called  " << std::endl;
	printParameters();

}

void SwitchCovMat::Clear()
{

}

void SwitchCovMat::processRunHeader()
{

}

void SwitchCovMat::processEvent( EVENT::LCEvent *pLCEvent )
{

	LCCollection *inputPfoCollection{};
	IMPL::LCCollectionVec* outputPfoCollection(NULL);
	outputPfoCollection = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
	outputPfoCollection->setSubset( true );
	int n_PFO = -1;
	this->Clear();
	streamlog_out(MESSAGE) << "" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////	Processing event 	" << pLCEvent->getEventNumber() << "	////////////////////" << std::endl;
	streamlog_out(MESSAGE) << "	////////////////////////////////////////////////////////////////////////////" << std::endl;

	try {
	  inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
	} catch(DataNotAvailableException &e) {
	  streamlog_out(MESSAGE) << "Input collection not found in event " << pLCEvent->getEventNumber() << std::endl;
	}
	n_PFO = inputPfoCollection->getNumberOfElements();
	if ( n_PFO == -1 ) streamlog_out(MESSAGE7) << "	Input PFO collection (" << m_inputPfoCollection << ") has no element (PFO) " << std::endl;
	//streamlog_out(MESSAGE7) << "	Total Number of PFOs: " << n_PFO << std::endl;
	for (int i_pfo = 0; i_pfo < n_PFO ; ++i_pfo) {
	  ReconstructedParticleImpl* outputPFO = dynamic_cast<ReconstructedParticleImpl*>( inputPfoCollection->getElementAt( i_pfo ) );
	  //if (outputPFO->getCovMatrix()[9] < 10e10) continue; 
	  if (outputPFO->getCovMatrix()[0] < 50 || outputPFO->getCovMatrix()[2] < 50 || outputPFO->getCovMatrix()[5] < 50) continue;
	  if (( outputPFO->getClusters()).size() == 0 ) continue;
	  streamlog_out(MESSAGE6) << "" << std::endl;
	  streamlog_out(MESSAGE6) << "	-------------------------------------------------------" << std::endl;
	  streamlog_out(MESSAGE6) << "	Processing PFO at index " << i_pfo << std::endl;
	  streamlog_out(MESSAGE5) << *outputPFO << std::endl;
	  float pfoMass = outputPFO->getMass();
	  TVector3 clusterPosition( 0.0 , 0.0 , 0.0 );
	  ROOT::Math::PxPyPzEVector pfoFourMomentum( 0.0 , 0.0 , 0.0 , 0.0 );
	  double outputPFOMomentum[3]{0., 0., 0.};
	  std::vector<float> outputCovMatrix( 10 , 0.0 );
	  if ( !m_AssumeNeutralPFOMassive ) pfoMass = 0.0;
	  float clusterX = ( outputPFO->getClusters()[0] )->getPosition()[0];
	  float clusterY = ( outputPFO->getClusters()[0] )->getPosition()[1];
	  float clusterZ = ( outputPFO->getClusters()[0] )->getPosition()[2];
	  clusterPosition	= TVector3( clusterX , clusterY , clusterZ );
	  float clusterDistance	= sqrt( pow( clusterX , 2 ) + pow( clusterY , 2 ) + pow( clusterZ , 2 ) );
	  float pfoMomentumMag	= 0;
	  float pfoEnergy	= outputPFO->getEnergy();
	  float pfoE;
	  pfoMomentumMag = ( m_isClusterEnergyKinEnergy ? sqrt( pow( pfoEnergy , 2 ) + 2 * pfoMass * pfoEnergy ) : pfoEnergy );
	  float pfoPx;
	  float pfoPy;
	  float pfoPz;
	  if ( m_updatePFO4Momentum ) {
	      pfoPx	= pfoMomentumMag * clusterX / clusterDistance;
	      pfoPy	= pfoMomentumMag * clusterY / clusterDistance;
	      pfoPz	= pfoMomentumMag * clusterZ / clusterDistance;
	      pfoE = ( m_isClusterEnergyKinEnergy ? pfoEnergy + pfoMass : sqrt( pow( pfoMomentumMag , 2 ) + pow( pfoMass , 2 ) ) );
	  } else {
	    pfoPx	= outputPFO->getMomentum()[ 0 ];
	    pfoPy	= outputPFO->getMomentum()[ 1 ];
	    pfoPz	= outputPFO->getMomentum()[ 2 ];
	    pfoE	= outputPFO->getEnergy();
	  }
	  std::vector<float> clusterPositionError = ( outputPFO->getClusters()[0] )->getPositionError();
	  float clusterEnergyError = ( outputPFO->getClusters()[0] )->getEnergyError();
	  pfoFourMomentum	= ROOT::Math::PxPyPzEVector(  pfoPx , pfoPy , pfoPz , pfoE );
	  streamlog_out(WARNING) << "Type = " << outputPFO->getType() << std::endl;
	  outputCovMatrix	= getNeutralCovMat( clusterPosition , pfoEnergy , pfoMass , clusterPositionError , clusterEnergyError );
	  
	  outputPFOMomentum[ 0 ] = pfoFourMomentum.Px();
	  outputPFOMomentum[ 1 ] = pfoFourMomentum.Py();
	  outputPFOMomentum[ 2 ] = pfoFourMomentum.Pz();
	  pfoE = pfoFourMomentum.E();
	  
	  //Trace of cov mat from track
	  float tracefromtrack = outputPFO->getCovMatrix()[0]+outputPFO->getCovMatrix()[2]+outputPFO->getCovMatrix()[5]+outputPFO->getCovMatrix()[9];
	  //Trace of cov mat from cluster
	  float tracefromcluster = outputCovMatrix[0]+outputCovMatrix[2]+outputCovMatrix[5]+outputCovMatrix[9];
	  //compare traces and set cov mat with the better trace
	  streamlog_out(MESSAGE) << "Trace from track   = " << tracefromtrack << std::endl;
	  streamlog_out(MESSAGE) << "Trace from cluster = " << tracefromcluster << std::endl;
	  //Also change pfo 4-mom based on cluster?
	  
	  // outputPFO->setType(outputPFO->getType());
	  // outputPFO->setMomentum( outputPFOMomentum );
	  // outputPFO->setEnergy( pfoE );
	  // outputPFO->setMass( outputPFO->getMass() );
	  // outputPFO->setCharge(outputPFO->getCharge());
	  if (tracefromcluster < tracefromtrack) outputPFO->setCovMatrix( outputCovMatrix );
	  /* outputPFO->setReferencePoint(outputPFO->getReferencePoint());
	     for (unsigned int j=0; j<outputPFO->getParticleIDs().size(); ++j) {
	     ParticleIDImpl* inPID = dynamic_cast<ParticleIDImpl*>(outputPFO->getParticleIDs()[j]);
	     ParticleIDImpl* outPID = new ParticleIDImpl;
	     outPID->setType(inPID->getType());
	     outPID->setPDG(inPID->getPDG());
	     outPID->setLikelihood(inPID->getLikelihood());
	     outPID->setAlgorithmType(inPID->getAlgorithmType()) ;
	     for (unsigned int k=0; k<inPID->getParameters().size()  ; ++k) outPID->addParameter(inPID->getParameters()[k]) ;
	     outputPFO->addParticleID(outPID);
	     }
	     outputPFO->setParticleIDUsed(outputPFO->getParticleIDUsed());
	     outputPFO->setGoodnessOfPID(outputPFO->getGoodnessOfPID());
	     for (unsigned int j=0; j< outputPFO->getParticles().size(); ++j) {
	     outputPFO->addParticle(outputPFO->getParticles()[j]);
	     }
	     for (unsigned int j=0; j<outputPFO->getClusters().size(); ++j) {
	     outputPFO->addCluster(outputPFO->getClusters()[j]);
	     }
	     for (unsigned int j=0; j<outputPFO->getTracks().size(); ++j) {
	     outputPFO->addTrack(outputPFO->getTracks()[j]);
	     }
	     outputPFO->setStartVertex(outputPFO->getStartVertex());
	  */
	  streamlog_out(MESSAGE6) << "	Updated PFO:" << std::endl;
	  streamlog_out(MESSAGE5) << *outputPFO << std::endl;			
	  outputPfoCollection->addElement( outputPFO );
	}
	pLCEvent->addCollection( outputPfoCollection , m_outputPfoCollection );

}

std::vector<float> SwitchCovMat::getNeutralCovMat( TVector3 clusterPosition , float pfoEc , float pfoMass , std::vector<float> clusterPositionError , float clusterEnergyError )
{

//	Obtain covariance matrix on (px,py,pz,E) from the
//	covariance matrix on cluster parameters (px,py,pz,|p|=Ec).
//	=> E^2 = Ec^2 + m^2	;	|p| = Ec
//	define the jacobian as the 4x4 matrix:
//
//
//
//			Dpx/Dx			Dpy/Dx			Dpz/Dx			DE/Dx
//
//			Dpx/Dy			Dpy/Dy			Dpz/Dy			DE/Dy
//	J =
//			Dpx/Dz			Dpy/Dz			Dpz/Dz			DE/Dz
//
//			Dpx/DEc			Dpy/DEc			Dpz/DEc			DE/DEc
//
//
//
//
//
//			 |P|.(r2-x2)/r3		-|P|.x.y/r3		-|P|.x.z/r3		0
//
//			-|P|.y.x/r3		 |P|.(r2-y2)/r3		-|P|.y.z/r3		0
//	J =
//			-|P|.z.x/r3		-|P|.z.y/r3		 |P|.(r2-z2)/r3		0
//
//			 (E/|p|).(x/r)		 (E/|p|).(y/r)		 (E/|p|).(z/r)		1
//
//
//
//
//	CovMatrix elements in terms of cluster position error and cluster energy error:
//
//			x.x			x.y			x.z			x.Ec
//
//			y.x			y.y			y.z			y.Ec
//	Cov =
//			z.x			z.y			z.z			z.Ec
//
//			Ec.x			Ec.y			Ec.z			Ec.Ec
//
//
//

	const int rows			= 4; // n rows jacobian
	const int columns		= 4; // n columns jacobian
	const int kspace_time_dim	= 4;

	TMatrixD covMatrixMomenta(kspace_time_dim,kspace_time_dim);
	std::vector<float> covP; covP.clear();

//	pfoMass			= 0.0;

	float pfoX		=	clusterPosition.X();
	float pfoY		=	clusterPosition.Y();
	float pfoZ		=	clusterPosition.Z();
	float pfoR		=	std::sqrt( pow( pfoX , 2 ) + pow( pfoY , 2 ) + pow( pfoZ , 2 ) );
	float pfoX2		=	pow( pfoX , 2 );
	float pfoY2		=	pow( pfoY , 2 );
	float pfoZ2		=	pow( pfoZ , 2 );
	float pfoR2		=	pow( pfoR , 2 );
	float pfoR3		=	pow( pfoR , 3 );
	float SigmaX2		=	clusterPositionError[ 0 ];
	float SigmaXY		=	clusterPositionError[ 1 ];
	float SigmaY2		=	clusterPositionError[ 2 ];
	float SigmaXZ		=	clusterPositionError[ 3 ];
	float SigmaYZ		=	clusterPositionError[ 4 ];
	float SigmaZ2		=	clusterPositionError[ 5 ];
	float SigmaE2		=	pow( clusterEnergyError , 2 );

	float pfoP = ( m_isClusterEnergyKinEnergy ? sqrt( pow( pfoEc , 2 ) + 2 * pfoMass * pfoEc ) : pfoEc );
	float pfoE = ( m_isClusterEnergyKinEnergy ? sqrt( pow( pfoP , 2 ) + pow( pfoMass , 2 ) ) : sqrt( pow( pfoP , 2 ) + pow( pfoMass , 2 ) ) );
	float derivative_coeff	= ( ( m_useTrueJacobian && !m_isClusterEnergyKinEnergy ) ? pfoP / pfoE : 1.0 );

	streamlog_out(MESSAGE0) << "	Cluster information obtained:" << std::endl;
	streamlog_out(WARNING0) << "		Cluster Information:" << std::endl;
	streamlog_out(WARNING0) << "		X = " << pfoX << "	, Y = " << pfoY << "	, Z = " << pfoZ << "	, Energy = " << pfoEc << "	, Mass = " << pfoMass << std::endl;
//	cluster covariance matrix by rows
	double cluster_cov_matrix_by_rows[rows*rows] =
	{
		SigmaX2		,	SigmaXY		,	SigmaXZ		,	0	,
		SigmaXY		,	SigmaY2		,	SigmaYZ		,	0	,
		SigmaXZ		,	SigmaYZ		,	SigmaZ2		,	0	,
		0		,	0		,	0		,	SigmaE2
	};
	TMatrixD covMatrix_cluster(rows,rows, cluster_cov_matrix_by_rows, "C");
	streamlog_out(MESSAGE0) << "	Cluster covariance matrix array converted to cluster covariance matrix" << std::endl;

	streamlog_out(MESSAGE0) << "		Cluster Position Error:" << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrix_cluster( 0 , 0 ) << "	, " << covMatrix_cluster( 0 , 1 ) << "	, " << covMatrix_cluster( 0 , 2 ) << "	, " << covMatrix_cluster( 0 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrix_cluster( 1 , 0 ) << "	, " << covMatrix_cluster( 1 , 1 ) << "	, " << covMatrix_cluster( 1 , 2 ) << "	, " << covMatrix_cluster( 1 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrix_cluster( 2 , 0 ) << "	, " << covMatrix_cluster( 2 , 1 ) << "	, " << covMatrix_cluster( 2 , 2 ) << "	, " << covMatrix_cluster( 2 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrix_cluster( 3 , 0 ) << "	, " << covMatrix_cluster( 3 , 1 ) << "	, " << covMatrix_cluster( 3 , 2 ) << "	, " << covMatrix_cluster( 3 , 3 ) << std::endl;


//	Define array with jacobian matrix elements by rows
	double jacobian_by_rows[rows*columns] =
	{
		pfoP * ( pfoR2 - pfoX2 ) / pfoR3			,	-pfoP * pfoX * pfoY / pfoR3				,	-pfoP * pfoX * pfoZ / pfoR3				,	0			,
		-pfoP * pfoY * pfoX / pfoR3				,	pfoP * ( pfoR2 - pfoY2 ) / pfoR3			,	-pfoP * pfoY * pfoZ / pfoR3				,	0			,
		-pfoP * pfoZ * pfoX / pfoR3				,	-pfoP * pfoZ * pfoY / pfoR3				,	pfoP * ( pfoR2 - pfoZ2 ) / pfoR3			,	0			,
		derivative_coeff * pfoE * pfoX / ( pfoP * pfoR )	,	derivative_coeff * pfoE * pfoY / ( pfoP * pfoR )	,	derivative_coeff * pfoE * pfoZ / ( pfoP * pfoR )	,	derivative_coeff
	};

//	construct the Jacobian using previous array ("F" if filling by columns, "C" if filling by rows, $ROOTSYS/math/matrix/src/TMatrixT.cxx)
	TMatrixD jacobian(rows,columns, jacobian_by_rows, "C");
	streamlog_out(MESSAGE0) << "	Jacobian array converted to Jacobian matrix" << std::endl;
	streamlog_out(MESSAGE0) << "		Jacobian:" << std::endl;
	streamlog_out(MESSAGE0) << "			" << jacobian( 0 , 0 ) << "	, " << jacobian( 0 , 1 ) << "	, " << jacobian( 0 , 2 ) << "	, " << jacobian( 0 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << jacobian( 1 , 0 ) << "	, " << jacobian( 1 , 1 ) << "	, " << jacobian( 1 , 2 ) << "	, " << jacobian( 1 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << jacobian( 2 , 0 ) << "	, " << jacobian( 2 , 1 ) << "	, " << jacobian( 2 , 2 ) << "	, " << jacobian( 2 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << jacobian( 3 , 0 ) << "	, " << jacobian( 3 , 1 ) << "	, " << jacobian( 3 , 2 ) << "	, " << jacobian( 3 , 3 ) << std::endl;


	covMatrixMomenta.Mult( TMatrixD( jacobian ,
					TMatrixD::kTransposeMult ,
					covMatrix_cluster) ,
					jacobian
					);

	streamlog_out(MESSAGE0) << "		PFO Covariance Matrix:" << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrixMomenta( 0 , 0 ) << "	, " << covMatrixMomenta( 0 , 1 ) << "	, " << covMatrixMomenta( 0 , 2 ) << "	, " << covMatrixMomenta( 0 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrixMomenta( 1 , 0 ) << "	, " << covMatrixMomenta( 1 , 1 ) << "	, " << covMatrixMomenta( 1 , 2 ) << "	, " << covMatrixMomenta( 1 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrixMomenta( 2 , 0 ) << "	, " << covMatrixMomenta( 2 , 1 ) << "	, " << covMatrixMomenta( 2 , 2 ) << "	, " << covMatrixMomenta( 2 , 3 ) << std::endl;
	streamlog_out(MESSAGE0) << "			" << covMatrixMomenta( 3 , 0 ) << "	, " << covMatrixMomenta( 3 , 1 ) << "	, " << covMatrixMomenta( 3 , 2 ) << "	, " << covMatrixMomenta( 3 , 3 ) << std::endl;

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
	if ( covP.size() == 10 ) streamlog_out(MESSAGE0) << "	FourMomentumCovarianceMatrix Filled succesfully" << std::endl;

	return covP;

}

void SwitchCovMat::check(EVENT::LCEvent *pLCEvent)
{

	LCCollection *inputPfoCollection{};
	LCCollection *outputPfoCollection{};
	try
	{
		inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
		outputPfoCollection = pLCEvent->getCollection(m_outputPfoCollection);
		//int n_inputPFOs = inputPfoCollection->getNumberOfElements();
		//int n_outputPFOs = outputPfoCollection->getNumberOfElements();
		//streamlog_out(MESSAGE) << " CHECK : processed event: " << pLCEvent->getEventNumber() << " (Number of inputPFOS: " << n_inputPFOs << " , Number of outputPFOs: " << n_outputPFOs <<")" << std::endl;
	}
	catch(DataNotAvailableException &e)
        {
          streamlog_out(MESSAGE) << "Input/Output collection not found in event " << pLCEvent->getEventNumber() << std::endl;
        }

}

void SwitchCovMat::end()
{

//	std::cout << " END : processed events: " << m_nEvtSum << std::endl;

}
