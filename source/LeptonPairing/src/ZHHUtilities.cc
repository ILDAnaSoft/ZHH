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

void doPhotonRecovery(ReconstructedParticle *electron, LCCollection *colPFO, ReconstructedParticleImpl *recoElectron, Double_t fCosFSRCut, 
		      Int_t lepType, std::vector<lcio::ReconstructedParticle*> &photons) {
  //streamlog_out(MESSAGE) << "Ladida hfskdafk" << std::endl;
  // recover the BS and FSR photons
  TLorentzVector lortzElectron = TLorentzVector(electron->getMomentum(),electron->getEnergy());
  std::vector<float> electronCovMat = electron->getCovMatrix();
  recoElectron->addParticle(electron);
  Int_t nPFOs = colPFO->getNumberOfElements();
  for (Int_t i=0;i<nPFOs;i++) {
    ReconstructedParticle *recPart = dynamic_cast<ReconstructedParticle*>(colPFO->getElementAt(i));
    if (recPart == electron) continue;
    if (isolep::isFoundInVector(recPart,photons)) continue;
    Bool_t isFSR = isolep::getFSRTag(electron,recPart,fCosFSRCut);
    if (! isFSR) continue;
    photons.push_back(recPart);
    recoElectron->addParticle(recPart);
    if (lepType == 11) {
      // do split algorithm only for electron
      Bool_t isSplit = isolep::getSplitTag(electron,recPart);
      if (isSplit) continue;
    }
    else if (lepType == 13) {
    }
    lortzElectron += TLorentzVector(recPart->getMomentum(),recPart->getEnergy());
    std::transform(electronCovMat.begin(), electronCovMat.end(), recPart->getCovMatrix().begin(), 
		   electronCovMat.begin(), std::plus<float>());
  }
  Double_t energy = lortzElectron.E();
  Double_t mass   = lortzElectron.M();
  Double_t momentum[3] = {lortzElectron.Px(),lortzElectron.Py(),lortzElectron.Pz()};
  Double_t charge = electron->getCharge();
  recoElectron->setMomentum(momentum);
  recoElectron->setEnergy(energy);
  recoElectron->setMass(mass);
  recoElectron->setCharge(charge);
  recoElectron->setType(94);
  recoElectron->setCovMatrix(electronCovMat);
}

} // namespace mylib