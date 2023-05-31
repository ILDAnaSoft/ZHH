#ifndef Misclustering_h
#define Misclustering_h 1

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

class Misclustering : public Processor , public TrueJet_Parser
{

 public:

  virtual Processor*  newProcessor()
  {
    return new Misclustering;
  }

  Misclustering();
  virtual ~Misclustering() = default;
  Misclustering(const Misclustering&) = delete;
  Misclustering& operator=(const Misclustering&) = delete;

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
  float getMatchingByAngularSpace( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<pair<int,int>> &idxpair);
  bool getMatchingByLeadingParticle( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<pair<int,int>> &idxpair);

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
  vector<float> m_nrecodijetPFOs{};
  vector<float> m_ntruedijetPFOs{};
  vector<float> m_overlap{};
  vector<float> m_recodijetmass{};
  vector<float> m_truedijetmass{};
  vector<float> m_energyfrac_reco{};
  vector<float> m_energyfrac_true{};
  vector<float> m_nrecodijetPFOs_ICNs{};
  vector<float> m_ntruedijetPFOs_ICNs{};
  vector<float> m_overlap_ICNs{};
  vector<float> m_recodijetmass_ICNs{};
  vector<float> m_truedijetmass_ICNs{};
  vector<float> m_energyfrac_reco_ICNs{};
  vector<float> m_energyfrac_true_ICNs{};
  vector<float> m_y_45{};
  vector<float> m_y_34{};
  vector<float> m_sumofangles{};
  vector<float> m_recodijet_energy{};
  vector<float> m_recodijet_theta{};
  vector<float> m_recodijet_phi{};
  vector<float> m_recodijet_energy_ICNs{};
  vector<float> m_recodijet_theta_ICNs{};
  vector<float> m_recodijet_phi_ICNs{};
  vector<float> m_regionXX{};
  vector<float> m_regionXX_ICNs{};

  TFile *m_pTFile{};
  TTree *m_pTTree{};


 private:

  string m_recoJetCollectionName{};
  string _MCParticleColllectionName{};
  string _recoParticleCollectionName{};
  string _recoMCTruthLink{};
  int m_matchMethod{};
  string m_outputFile{};
};

#endif
