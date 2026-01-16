#ifndef PFOSelection_h
#define PFOSelection_h 1
#include <marlin/Processor.h>
#include <marlin/Global.h>
#include "UTIL/LCRelationNavigator.h"
#include <EVENT/MCParticle.h>
#include "IMPL/LCCollectionVec.h"
#include <IMPL/ReconstructedParticleImpl.h>
#include "DDMarlinCED.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "lcio.h"
#include <string>
#include <vector>
#include <math.h>
#include <set>
#include <vector>
#include "TMatrixD.h"
#include <TFile.h>
#include <TTree.h>

class TFile;
class TDirectory;
class TH1F;
class TH1I;
class TH2I;
class TH2F;
class TTree;

using namespace lcio ;
using namespace marlin ;

class PFOSelection : public Processor {
public:
  
  virtual Processor*  newProcessor() {
    return new PFOSelection;
  }
  PFOSelection();
  virtual ~PFOSelection() = default;
  PFOSelection(const PFOSelection&) = delete;
  PFOSelection& operator=(const PFOSelection&) = delete;
  virtual void init();
  virtual void Clear();
  virtual void processRunHeader();
  virtual void processEvent( EVENT::LCEvent *pLCEvent );
  void pfofromcluster(ReconstructedParticleImpl* outputPFO);
  std::vector<float> getNeutralCovMat( TVector3 clusterPosition , float pfoEc , float pfoMass , std::vector<float> clusterPositionError , float clusterEnergyError );
  virtual void check( EVENT::LCEvent *pLCEvent );
  virtual void end();
  
private:
  std::string				m_inputPfoCollection{};
  std::string				m_outputPfoCollection{};
  
};
#endif
