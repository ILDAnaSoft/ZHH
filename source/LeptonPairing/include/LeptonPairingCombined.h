#ifndef LeptonPairingCombined_h
#define LeptonPairingCombined_h 1

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

class LeptonPairingCombined : public Processor 
{
 public: 
  virtual Processor*  newProcessor() {
    return new LeptonPairingCombined;
  }
  LeptonPairingCombined();
  virtual ~LeptonPairingCombined() = default;
  LeptonPairingCombined(const LeptonPairingCombined&) = delete;
  LeptonPairingCombined& operator=(const LeptonPairingCombined&) = delete;
  virtual void init();
  virtual void Clear();
  virtual void processRunHeader(LCRunHeader* run);
  virtual void processEvent( EVENT::LCEvent *pLCEvent );
  virtual void check(EVENT::LCEvent *pLCEvent);
  virtual void end();

 protected:
  virtual float evaluateIsoLeptons(EVENT::LCCollection * col, int leptonType);

  double m_diLepInvMass = 91.2;
  string m_rootFile{};
  bool m_doPhotonRecovery = false;
  bool m_fillRootTree = false;
  int m_nRun;
  int m_nEvt;
  vector<float> m_IsoLepsInvMass{};
  vector<float> m_RecoLepsInvMass{};
  vector<ReconstructedParticle*> m_bestLeptonPair{};
  vector<int> m_bestLeptonPairIDx{};
  int m_bestLeptonType{};
  float m_bestLeptonDelta{};
  TFile *m_pTFile{};
  TTree *m_pTTree = new TTree("LeptonPairingCombined", "LeptonPairingCombined");

  string m_inputIsoElectronCollection{};
  string m_inputIsoMuonCollection{};
  string m_inputIsoTauCollection{};
  string m_inputPFOsWOIsoLepCollection{};

  string m_outputPFOsWOLepPairCollection{};
  string m_outputIsoLepPairCollection{};
  string m_outputLepPairCollection{};
  

};
#endif
