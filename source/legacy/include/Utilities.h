#ifndef UTILITIES_H
#define UTILITIES_H

// *******************************************************
// some useful functions
//                ----tianjp
// *******************************************************

#include "lcio.h"
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>

#include "TROOT.h"
#include "TVector3.h"
#include "TLorentzVector.h"

using namespace lcio ;
using namespace std;

Int_t getMCSerial(MCParticle *mcPart, LCCollection *colMCP);
//MCParticle* getLinkedMCParticle(ReconstructedParticle *recPart, LCCollection *colMCTL, Double_t &weight, Int_t &nMCTL);
Int_t getLinkedMCParticle(ReconstructedParticle *recPart, LCCollection *colMCTL, Double_t &weight, Int_t &nMCTL);
Int_t getOriginalPDG(MCParticle *mcPart);
Double_t getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone);
Double_t getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, Int_t mode);
Double_t getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, 
		       std::vector<lcio::ReconstructedParticle*> &conePFOs);
Double_t getInvariantMass(ReconstructedParticle *recPart1, ReconstructedParticle *recPart2);
Double_t getInvariantMass(ReconstructedParticle *recPart1, ReconstructedParticle *recPart2, ReconstructedParticle *recPart3);

Int_t getLeptonID(ReconstructedParticle *recPart);
Bool_t getFSRTag(ReconstructedParticle *motherPart, ReconstructedParticle *recPart, Double_t fCosFSRCut = 0.999);
Bool_t getSplitTag(ReconstructedParticle *motherPart, ReconstructedParticle *recPart); 
//TVector3 getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, Bool_t woFSR);
//void getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, Bool_t woFSR, TVector3 coneEnergy0);
void getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, Bool_t woFSR, Double_t coneEnergy[3], Double_t pFSR[4]);
void getConeEnergy(ReconstructedParticle *recPart, LCCollection *colPFO, Double_t cosCone, Bool_t woFSR, Double_t coneEnergy[3], Double_t pFSR[4], 
		   Double_t conCone2, Double_t pCone2[4], Int_t &nConePhoton);
TLorentzVector getFSRMomentum(ReconstructedParticle *recPart, LCCollection *colPFO);
Int_t isSelectedByFastJet( ReconstructedParticle *pfo, LCCollection *colFastJet, Double_t &ratioEPartEJet, Double_t &ratioPTMJet);
  

#endif
