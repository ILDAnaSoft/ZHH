#ifndef LeptonPairing_h
#define LeptonPairing_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <vector>
#include "TLorentzVector.h"
#include "EVENT/LCCollection.h"
#include "EVENT/ReconstructedParticle.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/ReconstructedParticleImpl.h"
#include "IMPL/ParticleIDImpl.h"
#include "UTIL/PIDHandler.h"
#include "marlin/VerbosityLevels.h"
#include "TFile.h"
#include "TH1I.h"
#include "TTree.h"


using namespace lcio ;
using namespace marlin ;
using namespace std;

class LeptonPairing : public Processor 
{
 public: 
  virtual Processor*  newProcessor() {
    return new LeptonPairing;
  }
  LeptonPairing();
  virtual ~LeptonPairing() = default;
  LeptonPairing(const LeptonPairing&) = delete;
  LeptonPairing& operator=(const LeptonPairing&) = delete;
  virtual void init();
  virtual void Clear();
  virtual void processRunHeader(LCRunHeader* run);
  virtual void processEvent( EVENT::LCEvent *pLCEvent );
  virtual void check(EVENT::LCEvent *pLCEvent);
  virtual void end();

 protected:
  double m_diLepInvMass = 91.2;
  string m_rootFile{};
  bool m_doPhotonRecovery = false;
  bool m_fillRootTree = false;
  int m_nRun;
  int m_nEvt;
  vector<float> m_IsoLepsInvMass;
  vector<float> m_RecoLepsInvMass;
  TFile *m_pTFile{};
  TTree *m_pTTree{};

  string m_inputIsoLepCollection{};
  string m_inputPFOsWOIsoLepCollection{};
  string m_outputLepPairCollection{};
  string m_outputPFOsWOLepPairCollection{};
  

};
#endif
