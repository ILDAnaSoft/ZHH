#ifndef bHadronAnalysis_h
#define bHadronAnalysis_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <Math/Vector4D.h>

class TFile;
class TH1F;
class TH1I;
class TH2I;
class TTree;

using namespace lcio ;
using namespace marlin ;
class bHadronAnalysis : public Processor
{
public:
  
  virtual Processor*  newProcessor()
  {
    return new bHadronAnalysis;
  }
  bHadronAnalysis();
  virtual ~bHadronAnalysis() = default;
  bHadronAnalysis(const bHadronAnalysis&) = delete;
  bHadronAnalysis& operator=(const bHadronAnalysis&) = delete;
  virtual void init();
  virtual void processRunHeader();
  virtual void processEvent( EVENT::LCEvent *pLCEvent );
  virtual void check();
  virtual void end();
  void Clear();
  
private:
  
  typedef std::vector<int>     	IntVector;
  typedef std::vector<double>	DoubleVector;
  typedef std::vector<float>	FloatVector;
  
  std::string m_mcParticleCollection{};
  std::string m_bHadronAnalysisCollection{};
  std::string m_outputFile{};
  int m_nRun;
  int m_nEvt;
  float m_bhadronEnergy; 
  float m_bhadronTheta; 
  float m_bhadronPhi; 
  float m_bhadronfromZEnergy; 
  float m_bhadronfromZTheta; 
  float m_bhadronfromZPhi; 
  float m_bhadronfromHEnergy; 
  float m_bhadronfromHTheta; 
  float m_bhadronfromHPhi; 
  float m_bhadronBeta; 
  float m_bhadronGamma; 
  float m_bhadronBetaGamma; 
  float m_bdecaylength; 
  float m_cdecaylength; 
  float m_longestbdecaylength; 
  float m_2ndlongestbdecaylength; 
  float m_3rdlongestbdecaylength; 
  float m_4thlongestbdecaylength; 
  
  TFile *m_pTFile{};
  TTree *m_pTTree{};
};

#endif
