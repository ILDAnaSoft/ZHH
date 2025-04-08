#ifndef LeptonErrorAnalysis_h
#define LeptonErrorAnalysis_h 1

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
#include <marlinutil/GeometryUtil.h>
class TFile;
class TH1F;
class TH1I;
class TH2I;
class TH2F;
class TTree;

using namespace lcio ;
using namespace marlin ;

class LeptonErrorAnalysis : public Processor , public TrueJet_Parser
{

 public:

  virtual Processor*  newProcessor()
  {
    return new LeptonErrorAnalysis;
  }

  LeptonErrorAnalysis();
  virtual ~LeptonErrorAnalysis() = default;
  LeptonErrorAnalysis(const LeptonErrorAnalysis&) = delete;
  LeptonErrorAnalysis& operator=(const LeptonErrorAnalysis&) = delete;

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
   * called for every pair of true and reconstructed particles
   */
  void getLeptonResiduals( EVENT::LCEvent *pLCEvent ,EVENT::ReconstructedParticle* lepton, float(&leptonResiduals)[ 3 ]);
  void calculateResiduals( EVENT::ReconstructedParticle* lepton, TLorentzVector mcpFourMomentum, float(&leptonResiduals)[ 3 ] );
  //std::vector<float> getLeptonResiduals( EVENT::ReconstructedParticle* lepton , TLorentzVector mcpFourMomentum );

  virtual void check();

  /*
   * Called after data processing for clean up.
   */
  virtual void end();

  std::string get_recoMCTruthLink()
    {
      return _recoMCTruthLink;
    };

 protected:

  /*
   * Input collection name.
   */
  int m_nRun;
  int m_nEvt;

  float m_Bfield;
  float c;
  float mm2m;
  float eV2GeV;
  float eB;
  std::vector<float> m_NormResidualInvPt{};
  std::vector<float> m_NormResidualTheta{};
  std::vector<float> m_NormResidualPhi{};

  
 private:


  std::string _MCParticleCollectionName{};
  std::string _recoParticleCollectionName{};
  std::string m_inputLepCollection{};
  std::string m_TrackMCTruthLinkCollection{};
  std::string m_MCTruthTrackLinkCollection{};

  std::string _OutLeptonResidualsCol{};
  std::string m_outputFile{};
  TFile *m_pTFile{};
  TTree *m_pTTree{};
};

#endif
