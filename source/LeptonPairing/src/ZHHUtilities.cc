// *******************************************************
// some useful functions
// *******************************************************
#include "Utilities.h"
#include "TVector3.h"
#include "TMath.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLorentzVector.h"
#include <UTIL/LCRelationNavigator.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <streamlog/streamlog.h>
#include "marlin/VerbosityLevels.h"

namespace ZHH {

void doPhotonRecovery(ReconstructedParticle *lepton, LCCollection *colPFO, ReconstructedParticleImpl *recoLepton, Double_t fCosFSRCut, 
  Int_t lepType, std::vector<lcio::ReconstructedParticle*> &photons) {
  
  // recover the BS and FSR photons
  TLorentzVector lortzLepton = TLorentzVector(lepton->getMomentum(), lepton->getEnergy());
  std::vector<float> leptonCovMat = lepton->getCovMatrix();
  recoLepton->addParticle(lepton);
  Int_t nPFOs = colPFO->getNumberOfElements();

  for (Int_t i=0; i<nPFOs; i++) {
    ReconstructedParticle *recPart = dynamic_cast<ReconstructedParticle*>(colPFO->getElementAt(i));
    if (recPart == lepton)
      continue;

    if (isolep::isFoundInVector(recPart, photons))
      continue;
    
    // electrons and muons: the flavour is known (lepType) and passed on, so getFSRTag does not
    // re-identify the lepton from its calorimeter energies (fails e.g. for muons without a yoke signal)
    Bool_t isFSR = isolep::getFSRTag(lepton, recPart, fCosFSRCut,
      (abs(lepType) == 11 || abs(lepType) == 13) ? lepType : 0);
    if (!isFSR)
      continue;

    photons.push_back(recPart);
    recoLepton->addParticle(recPart);
    if (abs(lepType) == 11) {
      // do split algorithm only for electrons and positrons
      Bool_t isSplit = isolep::getSplitTag(lepton, recPart);
      if (isSplit) continue;
    }
    lortzLepton += TLorentzVector(recPart->getMomentum(), recPart->getEnergy());
    
    std::transform(leptonCovMat.begin(), leptonCovMat.end(), recPart->getCovMatrix().begin(), 
      leptonCovMat.begin(), std::plus<float>());
  }

  Double_t momentum[3] = {lortzLepton.Px(), lortzLepton.Py(), lortzLepton.Pz()};
  recoLepton->setMomentum(momentum);
  recoLepton->setEnergy(lortzLepton.E());
  recoLepton->setMass(lortzLepton.M());
  recoLepton->setCharge(lepton->getCharge());
  recoLepton->setType(lepType);
  recoLepton->setCovMatrix(leptonCovMat);
  
}

} // namespace mylib
