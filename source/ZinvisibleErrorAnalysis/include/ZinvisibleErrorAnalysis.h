#ifndef ZinvisibleErrorAnalysis_h
#define ZinvisibleErrorAnalysis_h 1

//#include "TrueJet.h"
#include "marlin/Processor.h"
#include "lcio.h"
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCRelation.h>
#include <EVENT/LCFloatVec.h>
#include <UTIL/LCRelationNavigator.h>
#include "IMPL/LCCollectionVec.h"
#include <IMPL/ReconstructedParticleImpl.h>
#include <IMPL/ParticleIDImpl.h>
#include <iostream>
#include <string>
#include "TrueJet_Parser.h"
#include "TLorentzVector.h"
#include <TFile.h>
#include <TTree.h>
#include <Math/Vector4D.h>
class TFile;
class TH1F;
class TH1I;
class TH2I;
class TH2F;
class TTree;

using namespace lcio;
using namespace marlin;
using namespace std;

class ZinvisibleErrorAnalysis : public Processor , public TrueJet_Parser
{

 public:

  virtual Processor*  newProcessor()
  {
    return new ZinvisibleErrorAnalysis;
  }

  ZinvisibleErrorAnalysis();
  virtual ~ZinvisibleErrorAnalysis() = default;
  ZinvisibleErrorAnalysis(const ZinvisibleErrorAnalysis&) = delete;
  ZinvisibleErrorAnalysis& operator=(const ZinvisibleErrorAnalysis&) = delete;

  /*
   * Called at the begin of the job before anything is read.
   * Use to initialize the processor, e.g. book histograms.
   */
  virtual void init();
  virtual void Clear();

  /*
   * Called for every run.
   */
  virtual void processRunHeader();

  /*
   * Called for every event - the working horse.
   */
  virtual void processEvent( LCEvent * evt );

  /*
   * called for every pair of true and reconstructed jets
   */

  virtual void check();

  /*
   * Called after data processing for clean up.
   */
  virtual void end();

  string get_recoMCTruthLink()
    {
      return _recoMCTruthLink;
    };

 protected:

  /*
   * Input collection name.
   */
  int m_nRun;
  int m_nEvt;
  int m_nRunSum;
  int m_nEvtSum;
  unsigned int m_nJets;
  float m_EnergyZinv;
  float m_ThetaZinv;   
  float m_PhiZinv;
  float m_EnergyZvv;
  float m_ThetaZvv;   
  float m_PhiZvv;   
  float m_EnergyResidual;
  float m_ThetaResidual;   
  float m_PhiResidual;
  
 private:

  string m_inputJetCollection{};
  string _MCParticleCollectionName{};
  string _recoParticleCollectionName{};
  string _OutZinvisibleResidualsCol{};
  string m_outputFile{};
  TFile	*m_pTFile{};
  TTree *m_pTTree{};

};

#endif
