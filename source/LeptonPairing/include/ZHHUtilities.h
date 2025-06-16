#ifndef ZHHUTILITIES_H
#define ZHHUTILITIES_H

// *******************************************************
// some useful functions for the ZHH analysis
// *******************************************************

#include "lcio.h"
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <string>


using namespace lcio ;

namespace ZHH{

  void doPhotonRecovery(ReconstructedParticle *lepton, LCCollection *colPFO, ReconstructedParticleImpl *recoLepton, Double_t fCosFSRCut, 
			Int_t lepType, std::vector<lcio::ReconstructedParticle*> &photons);


}

#endif
