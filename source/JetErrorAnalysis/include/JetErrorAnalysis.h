#ifndef JetErrorAnalysis_h
#define JetErrorAnalysis_h 1

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
class TFile;
class TH1F;
class TH1I;
class TH2I;
class TH2F;
class TTree;

using namespace lcio;
using namespace marlin;
using namespace std;

class JetErrorAnalysis : public Processor , public TrueJet_Parser
{

 public:

  virtual Processor*  newProcessor()
  {
    return new JetErrorAnalysis;
  }

  JetErrorAnalysis();
  virtual ~JetErrorAnalysis() = default;
  JetErrorAnalysis(const JetErrorAnalysis&) = delete;
  JetErrorAnalysis& operator=(const JetErrorAnalysis&) = delete;

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
  //virtual void getJetResiduals( TLorentzVector trueJetFourMomentum , EVENT::ReconstructedParticle *recoJet );
  // virtual void doProperGaussianFit( TH1F *histogram , float fitMin , float fitMax , float fitRange );
  void getJetResidualsByAngularSpace( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<vector<double>> &JetResiduals);
  bool getJetResidualsByLeadingParticle( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<vector<double>> &JetResiduals);
  void calculateResiduals( EVENT::ReconstructedParticle* recoJet, TLorentzVector trueJetFourMomentum, vector<double>&jetResiduals );

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
  unsigned int m_nTrueJets;
  unsigned int m_nRecoJets;


 private:

  string m_recoJetCollectionName{};
  string _MCParticleColllectionName{};
  string _recoParticleCollectionName{};
  string _recoMCTruthLink{};
  string _OutJetResidualsCol{};
  int m_matchMethod{};
};

#endif
