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
#include <TTree.h>
#include <Math/Vector4D.h>
#include "TFile.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2I.h"
#include "TH2F.h"

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
  float m_PxZinv;
  float m_PyZinv;
  float m_PzZinv;
  float m_EnergyZvv;
  float m_ThetaZvv;   
  float m_PhiZvv;   
  float m_PxZvv;   
  float m_PyZvv;   
  float m_PzZvv;   
  float m_EnergyResidual;
  float m_ThetaResidual;   
  float m_PhiResidual;
  float m_PxResidual;
  float m_PyResidual;
  float m_PzResidual;
  
 private:

  string m_inputJetCollection{};
  string _MCParticleCollectionName{};
  string _recoParticleCollectionName{};
  string _OutZinvisibleResidualsCol{};
  string m_outputFile{};
  TFile	*m_pTFile{};
  TTree *m_pTTree = new TTree("ZinvisibleErrorAnalysis", "ZinvisibleErrorAnalysis");

};

#endif
