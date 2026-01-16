#include "PFOSelection.h"
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

PFOSelection aPFOSelection;

PFOSelection::PFOSelection() :

  Processor("PFOSelection") {
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
}

void PFOSelection::init() {
  streamlog_out(MESSAGE) << "   init called  " << std::endl;
  printParameters();
}

void PFOSelection::Clear() {
}

void PFOSelection::processRunHeader() {
}

void PFOSelection::processEvent( EVENT::LCEvent *pLCEvent ) {
  
  LCCollection *inputPfoCollection{};
  IMPL::LCCollectionVec* outputPfoCollection(NULL);
  outputPfoCollection = new IMPL::LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
  outputPfoCollection->setSubset( true );
  int npfos = -1;
  this->Clear();
  
  try {
    inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "Input collection not found in event " << pLCEvent->getEventNumber() << std::endl;
  }
  npfos = inputPfoCollection->getNumberOfElements();
  if ( npfos == -1 ) streamlog_out(MESSAGE7) << "	Input PFO collection (" << m_inputPfoCollection << ") has no element (PFO) " << std::endl;
  double Etot = 0.0;
  for (int i_pfo = 0; i_pfo < npfos ; ++i_pfo) {
    ReconstructedParticleImpl* pfo = dynamic_cast<ReconstructedParticleImpl*>( inputPfoCollection->getElementAt( i_pfo ) );
    Etot += pfo->getEnergy();
  }
    streamlog_out(MESSAGE) << "       ////////////////////////////////////////////////////////////////////////////" << std::endl;
    streamlog_out(MESSAGE) << "       ////////////////////    fucking fuck " << Etot << "    ////////////////////" << std::endl;
    streamlog_out(MESSAGE) << "	////////////////////	Processing event 	" << pLCEvent->getEventNumber() << "	////////////////////" << std::endl;
    streamlog_out(MESSAGE) << "       ////////////////////////////////////////////////////////////////////////////" << std::endl;
  for (int i_pfo = 0; i_pfo < npfos ; ++i_pfo) {
    ReconstructedParticleImpl* pfo = dynamic_cast<ReconstructedParticleImpl*>( inputPfoCollection->getElementAt( i_pfo ) );
    //look for pfos with large relative energy error
    if (sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) > 1) {
      int ntracks = (pfo->getTracks()).size();
      int nclusters = (pfo->getClusters()).size();
      //streamlog_out(MESSAGE6) << "" << std::endl;
      //streamlog_out(MESSAGE6) << "	-------------------------------------------------------" << std::endl;
      //streamlog_out(MESSAGE6) << "	Processing PFO at index " << i_pfo << std::endl;
      //streamlog_out(MESSAGE5) << *pfo << std::endl;

      //streamlog_out(MESSAGE6) << "pfo with sigma/E = " << sqrt(pfo->getCovMatrix()[9]) <<"/"<< pfo->getEnergy() <<"="<<sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) << endl;
      //streamlog_out(MESSAGE6) << ntracks << " tracks, " << nclusters << " clusters" << endl;
      //if suspicious track has good cluster(s) -> use clusters instead to reconstruct pfo
      streamlog_out(MESSAGE6) << "pfo with sigma/E = " << sqrt(pfo->getCovMatrix()[9]) <<"/"<< pfo->getEnergy() <<"="<<sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) << endl;
      streamlog_out(MESSAGE6) << ntracks << " tracks, " << nclusters << " clusters" << endl;
      if (ntracks > 0 && nclusters > 0 ) {
	streamlog_out(MESSAGE6) << "pfo energy before: " << pfo->getEnergy() << endl;
	pfofromcluster(pfo);
	streamlog_out(MESSAGE6) << "pfo energy after:  " << pfo->getEnergy() << endl;
	streamlog_out(WARNING) << "Type = " << pfo->getType() << std::endl;
	streamlog_out(MESSAGE6) << "pfo with sigma/E = " << sqrt(pfo->getCovMatrix()[9]) <<"/"<< pfo->getEnergy() <<"="<<sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) << endl;
      }
      //if suspicious track has no clusters, check track
      else if (ntracks > 0 && nclusters == 0 ) {
	Track* track = (Track*) pfo->getTracks()[0];
	IntVec subdetectorhitnumbers = track->getSubdetectorHitNumbers();
	//float eB = MarlinUtil::getBzAtOrigin() * 2.99792458e8 * 1e-3 * 1e-9; //Bfield*c*mm2m*eV2GeV 
	float eB = 3.5 * 2.99792458e8 * 1e-3 * 1e-9; //Bfield*c*mm2m*eV2GeV 
	float pt = eB / track->getOmega();
	float theta= 2.0 * atan( 1.0 ) - atan( track->getTanLambda() );
	float phi = track->getPhi();
	//streamlog_out(MESSAGE6) << "pt = " << pt << ", theta = " << theta << ", phi = " << phi << endl;
	//if pfo is energetic enough to reach TPC but leaves no TPC hits
	if (abs(pt)>1 && (theta>0.25 && theta<2.9) && subdetectorhitnumbers[6]==0) {
	  streamlog_out(MESSAGE6) << "skips pfo with energy: " << pfo->getEnergy() << endl;
	  continue;
	}
	//TO DO: understand any left over pfos with large relative error
      }
    } //end if large relative energy error

    //if energy larger than ECM, skip pfo completely
    if (pfo->getEnergy()>550) {
      streamlog_out(MESSAGE6) << "skips pfo with very large energy: " << pfo->getEnergy() << endl;
      continue;
    }
    //streamlog_out(MESSAGE6) << "adding pfo with energy: " << pfo->getEnergy() << endl;
    //streamlog_out(MESSAGE5) << *pfo << std::endl;			
    outputPfoCollection->addElement( pfo );
  }
  pLCEvent->addCollection( outputPfoCollection , m_outputPfoCollection );

}

void PFOSelection::pfofromcluster(ReconstructedParticleImpl* pfo) {
  int nclusters = (pfo->getClusters()).size();
  float pfoMass = pfo->getMass();
  //if ( !m_AssumeNeutralPFOMassive ) pfoMass = 0.0;
  //Q: which mass for "fake muons"?
  ROOT::Math::PxPyPzEVector pfoFourMomentum( 0.0 , 0.0 , 0.0 , 0.0 );
  double pfoMomentum[3]{0., 0., 0.};
  float varEcluster = 0.;
  std::vector<float> outputCovMatrix( 10 , 0.0 );
  
  for (int iclu=0; iclu<nclusters; iclu++) {
    float clusterX = ( pfo->getClusters()[iclu] )->getPosition()[0];
    float clusterY = ( pfo->getClusters()[iclu] )->getPosition()[1];
    float clusterZ = ( pfo->getClusters()[iclu] )->getPosition()[2];
    streamlog_out(MESSAGE6) << clusterX << ", " << clusterY << ", " << clusterZ << endl;
    TVector3 clusterPosition = TVector3( clusterX , clusterY , clusterZ );
    float clusterDistance	= sqrt( pow( clusterX , 2 ) + pow( clusterY , 2 ) + pow( clusterZ , 2 ) );
    float pfoMomentumMag	= pfo->getClusters()[iclu]->getEnergy();
    float pfoPx	= pfoMomentumMag * clusterX / clusterDistance;
    float pfoPy	= pfoMomentumMag * clusterY / clusterDistance;
    float pfoPz	= pfoMomentumMag * clusterZ / clusterDistance;
    float pfoE = sqrt( pow( pfoMomentumMag , 2 ) + pow( pfoMass , 2 ) );
    std::vector<float> clusterPositionError = ( pfo->getClusters()[iclu] )->getPositionError();
    float clusterEnergyError = ( pfo->getClusters()[iclu] )->getEnergyError();
    pfoFourMomentum       += ROOT::Math::PxPyPzEVector(  pfoPx , pfoPy , pfoPz , pfoE );
    varEcluster += pow((pfo->getClusters()[iclu])->getEnergyError(),2);
    streamlog_out(WARNING) << "Type = " << pfo->getType() << std::endl;
    std::vector<float> tempCovMatrix( 10 , 0.0 );
    tempCovMatrix = getNeutralCovMat( clusterPosition , pfoE , pfoMass , clusterPositionError , clusterEnergyError );
    for (int i=0; i<10; i++) outputCovMatrix[i] += tempCovMatrix[i];
  }
  //check if track reconstruction is better than cluster reconstruction:
  if ( sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) < sqrt(varEcluster)/pfoFourMomentum.E() )  {
    streamlog_out(MESSAGE6) << "From track: " << sqrt(pfo->getCovMatrix()[9]) << "/" << (pfo->getEnergy()) << " = " << sqrt(pfo->getCovMatrix()[9])/(pfo->getEnergy()) << endl;
    streamlog_out(MESSAGE6) << "From cluster: " << sqrt(varEcluster) << "/" << pfoFourMomentum.E() << " = " << sqrt(varEcluster)/pfoFourMomentum.E() << endl;
    return;
  }

  streamlog_out(MESSAGE6) << "pfo (e,px,py,pz): "<<  pfoFourMomentum.E() << ", " << pfoFourMomentum.Px() << ", " << pfoFourMomentum.Py() << ", " << pfoFourMomentum.Pz() << endl;
  
  pfoMomentum[ 0 ] = pfoFourMomentum.Px();
  pfoMomentum[ 1 ] = pfoFourMomentum.Py();
  pfoMomentum[ 2 ] = pfoFourMomentum.Pz();

  pfo->setType(pfo->getType()>0 ? 211 : -211); //if track + cluster --> assume charged particle i.e. kaon?
  pfo->setMomentum( pfoMomentum );
  pfo->setEnergy( pfoFourMomentum.E() );
  pfo->setMass( pfo->getMass() );
  pfo->setCharge(pfo->getCharge());
  pfo->setCovMatrix( outputCovMatrix );
  /*pfo->setReferencePoint(pfo->getReferencePoint());
  streamlog_out(MESSAGE)  << pfo->getParticleIDs().size() << endl;
  for (unsigned int j=0; j<pfo->getParticleIDs().size(); j++) {
    ParticleIDImpl* inPID = dynamic_cast<ParticleIDImpl*>(pfo->getParticleIDs()[j]);
    ParticleIDImpl* outPID = new ParticleIDImpl;
    outPID->setType(inPID->getType());
    outPID->setPDG(inPID->getPDG());
    outPID->setLikelihood(inPID->getLikelihood());
    outPID->setAlgorithmType(inPID->getAlgorithmType()) ;
    for (unsigned int k=0; k<inPID->getParameters().size()  ; ++k) outPID->addParameter(inPID->getParameters()[k]) ;
    pfo->addParticleID(outPID);
  }
  pfo->setParticleIDUsed(pfo->getParticleIDUsed());
  pfo->setGoodnessOfPID(pfo->getGoodnessOfPID());
  for (unsigned int j=0; j< pfo->getParticles().size(); ++j) {
    pfo->addParticle(pfo->getParticles()[j]);
  }
  for (unsigned int j=0; j<pfo->getClusters().size(); ++j) {
    pfo->addCluster(pfo->getClusters()[j]);
  }
  for (unsigned int j=0; j<pfo->getTracks().size(); ++j) {
    pfo->addTrack(pfo->getTracks()[j]);
  }
  pfo->setStartVertex(pfo->getStartVertex());
  */
  return;
}

std::vector<float> PFOSelection::getNeutralCovMat( TVector3 clusterPosition , float pfoEc , float pfoMass , std::vector<float> clusterPositionError , float clusterEnergyError ) {

//      Obtain covariance matrix on (px,py,pz,E) from the
//      covariance matrix on cluster parameters (px,py,pz,|p|=Ec).
//      => E^2 = Ec^2 + m^2     ;       |p| = Ec
//      define the jacobian as the 4x4 matrix:
//
//      J =
//                      Dpx/Dx                  Dpy/Dx                  Dpz/Dx                  DE/Dx
//                      Dpx/Dy                  Dpy/Dy                  Dpz/Dy                  DE/Dy
//                      Dpx/Dz                  Dpy/Dz                  Dpz/Dz                  DE/Dz
//                      Dpx/DEc                 Dpy/DEc                 Dpz/DEc                 DE/DEc
//
//      J =
//                       |P|.(r2-x2)/r3         -|P|.x.y/r3             -|P|.x.z/r3             0
//                      -|P|.y.x/r3              |P|.(r2-y2)/r3         -|P|.y.z/r3             0
//                      -|P|.z.x/r3             -|P|.z.y/r3              |P|.(r2-z2)/r3         0
//                       (E/|p|).(x/r)           (E/|p|).(y/r)           (E/|p|).(z/r)          1
//
//      CovMatrix elements in terms of cluster position error and cluster energy error:
//
//      Cov =
//                      x.x                     x.y                     x.z                     x.Ec
//                      y.x                     y.y                     y.z                     y.Ec
//                      z.x                     z.y                     z.z                     z.Ec
//                      Ec.x                    Ec.y                    Ec.z                    Ec.Ec

        const int rows                  = 4; // n rows jacobian
        const int columns               = 4; // n columns jacobian
        const int kspace_time_dim       = 4;

        TMatrixD covMatrixMomenta(kspace_time_dim,kspace_time_dim);
        std::vector<float> covP; covP.clear();

//      pfoMass                 = 0.0;

        float pfoX              =       clusterPosition.X();
        float pfoY              =       clusterPosition.Y();
        float pfoZ              =       clusterPosition.Z();
        float pfoR              =       std::sqrt( pow( pfoX , 2 ) + pow( pfoY , 2 ) + pow( pfoZ , 2 ) );
        float pfoX2             =       pow( pfoX , 2 );
        float pfoY2             =       pow( pfoY , 2 );
        float pfoZ2             =       pow( pfoZ , 2 );
        float pfoR2             =       pow( pfoR , 2 );
        float pfoR3             =       pow( pfoR , 3 );
        float SigmaX2           =       clusterPositionError[ 0 ];
        float SigmaXY           =       clusterPositionError[ 1 ];
        float SigmaY2           =       clusterPositionError[ 2 ];
        float SigmaXZ           =       clusterPositionError[ 3 ];
        float SigmaYZ           =       clusterPositionError[ 4 ];
        float SigmaZ2           =       clusterPositionError[ 5 ];
        float SigmaE2           =       pow( clusterEnergyError , 2 );

        float pfoP = pfoEc ;
        float pfoE = sqrt( pow( pfoP , 2 ) + pow( pfoMass , 2 ) );
        float derivative_coeff  = pfoP / pfoE;

        streamlog_out(MESSAGE0) << "    Cluster information obtained:" << std::endl;
        streamlog_out(WARNING0) << "            Cluster Information:" << std::endl;
        streamlog_out(WARNING0) << "            X = " << pfoX << "      , Y = " << pfoY << "    , Z = " << pfoZ << "    , Energy = " << pfoEc << "      , Mass = " << pfoMass << std::endl;
//      cluster covariance matrix by rows
        double cluster_cov_matrix_by_rows[rows*rows] =
        {
                SigmaX2         ,       SigmaXY         ,       SigmaXZ         ,       0       ,
                SigmaXY         ,       SigmaY2         ,       SigmaYZ         ,       0       ,
                SigmaXZ         ,       SigmaYZ         ,       SigmaZ2         ,       0       ,
                0               ,       0               ,       0               ,       SigmaE2
        };
        TMatrixD covMatrix_cluster(rows,rows, cluster_cov_matrix_by_rows, "C");
        streamlog_out(MESSAGE0) << "    Cluster covariance matrix array converted to cluster covariance matrix" << std::endl;

        streamlog_out(MESSAGE0) << "            Cluster Position Error:" << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrix_cluster( 0 , 0 ) << "    , " << covMatrix_cluster( 0 , 1 ) << "  , " << covMatrix_cluster( 0 , 2 ) << "  , " << covMatrix_cluster( 0 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrix_cluster( 1 , 0 ) << "    , " << covMatrix_cluster( 1 , 1 ) << "  , " << covMatrix_cluster( 1 , 2 ) << "  , " << covMatrix_cluster( 1 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrix_cluster( 2 , 0 ) << "    , " << covMatrix_cluster( 2 , 1 ) << "  , " << covMatrix_cluster( 2 , 2 ) << "  , " << covMatrix_cluster( 2 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrix_cluster( 3 , 0 ) << "    , " << covMatrix_cluster( 3 , 1 ) << "  , " << covMatrix_cluster( 3 , 2 ) << "  , " << covMatrix_cluster( 3 , 3 ) << std::endl;


//      Define array with jacobian matrix elements by rows
        double jacobian_by_rows[rows*columns] =
        {
                pfoP * ( pfoR2 - pfoX2 ) / pfoR3                        ,       -pfoP * pfoX * pfoY / pfoR3                             ,       -pfoP * pfoX * pfoZ / pfoR3                             ,       0                       ,
                -pfoP * pfoY * pfoX / pfoR3                             ,       pfoP * ( pfoR2 - pfoY2 ) / pfoR3                        ,       -pfoP * pfoY * pfoZ / pfoR3                             ,       0                       ,
                -pfoP * pfoZ * pfoX / pfoR3                             ,       -pfoP * pfoZ * pfoY / pfoR3                             ,       pfoP * ( pfoR2 - pfoZ2 ) / pfoR3                        ,       0                       ,
                derivative_coeff * pfoE * pfoX / ( pfoP * pfoR )        ,       derivative_coeff * pfoE * pfoY / ( pfoP * pfoR )        ,       derivative_coeff * pfoE * pfoZ / ( pfoP * pfoR )        ,       derivative_coeff
        };

//      construct the Jacobian using previous array ("F" if filling by columns, "C" if filling by rows, $ROOTSYS/math/matrix/src/TMatrixT.cxx)
        TMatrixD jacobian(rows,columns, jacobian_by_rows, "C");
        streamlog_out(MESSAGE0) << "    Jacobian array converted to Jacobian matrix" << std::endl;
        streamlog_out(MESSAGE0) << "            Jacobian:" << std::endl;
        streamlog_out(MESSAGE0) << "                    " << jacobian( 0 , 0 ) << "     , " << jacobian( 0 , 1 ) << "   , " << jacobian( 0 , 2 ) << "   , " << jacobian( 0 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << jacobian( 1 , 0 ) << "     , " << jacobian( 1 , 1 ) << "   , " << jacobian( 1 , 2 ) << "   , " << jacobian( 1 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << jacobian( 2 , 0 ) << "     , " << jacobian( 2 , 1 ) << "   , " << jacobian( 2 , 2 ) << "   , " << jacobian( 2 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << jacobian( 3 , 0 ) << "     , " << jacobian( 3 , 1 ) << "   , " << jacobian( 3 , 2 ) << "   , " << jacobian( 3 , 3 ) << std::endl;

	covMatrixMomenta.Mult( TMatrixD( jacobian ,
                                        TMatrixD::kTransposeMult ,
                                        covMatrix_cluster) ,
                                        jacobian
                                        );

        streamlog_out(MESSAGE0) << "            PFO Covariance Matrix:" << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrixMomenta( 0 , 0 ) << "     , " << covMatrixMomenta( 0 , 1 ) << "   , " << covMatrixMomenta( 0 , 2 ) << "   , " << covMatrixMomenta( 0 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrixMomenta( 1 , 0 ) << "     , " << covMatrixMomenta( 1 , 1 ) << "   , " << covMatrixMomenta( 1 , 2 ) << "   , " << covMatrixMomenta( 1 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrixMomenta( 2 , 0 ) << "     , " << covMatrixMomenta( 2 , 1 ) << "   , " << covMatrixMomenta( 2 , 2 ) << "   , " << covMatrixMomenta( 2 , 3 ) << std::endl;
        streamlog_out(MESSAGE0) << "                    " << covMatrixMomenta( 3 , 0 ) << "     , " << covMatrixMomenta( 3 , 1 ) << "   , " << covMatrixMomenta( 3 , 2 ) << "   , " << covMatrixMomenta( 3 , 3 ) << std::endl;

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
        if ( covP.size() == 10 ) streamlog_out(MESSAGE0) << "   FourMomentumCovarianceMatrix Filled succesfully" << std::endl;

        return covP;
}

void PFOSelection::check(EVENT::LCEvent *pLCEvent) {

  LCCollection *inputPfoCollection{};
  LCCollection *outputPfoCollection{};
  try {
    inputPfoCollection = pLCEvent->getCollection(m_inputPfoCollection);
    outputPfoCollection = pLCEvent->getCollection(m_outputPfoCollection);
  } catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "Input/Output collection not found in event " << pLCEvent->getEventNumber() << std::endl;
  }
}

void PFOSelection::end() {
  //	std::cout << " END : processed events: " << m_nEvtSum << std::endl;
}
