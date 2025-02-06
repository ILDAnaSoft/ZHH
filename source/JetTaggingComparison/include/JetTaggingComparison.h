#ifndef JetTaggingComparison_h
#define JetTaggingComparison_h 1

#include "marlin/Processor.h"
#include "lcio.h"
#include <string>
#include "EVENT/LCCollection.h"
#include "EVENT/ReconstructedParticle.h"
#include "IMPL/ParticleIDImpl.h"
#include "UTIL/PIDHandler.h"
#include "marlin/VerbosityLevels.h"
#include "TFile.h"
#include "TTree.h"

using namespace lcio;
using namespace marlin;
using namespace std;

class JetTaggingComparison : public Processor 
{
 public: 
  virtual Processor*  newProcessor() {
    return new JetTaggingComparison;
  }
  JetTaggingComparison();
  virtual ~JetTaggingComparison() = default;
  JetTaggingComparison(const JetTaggingComparison&) = delete;
  JetTaggingComparison& operator=(const JetTaggingComparison&) = delete;

  virtual void init();
  virtual void Clear();
  virtual void processRunHeader(LCRunHeader* run);
  virtual void processEvent( EVENT::LCEvent *pLCEvent );
  virtual void check(EVENT::LCEvent *pLCEvent);
  virtual void end();

 protected:
  string m_inputCollection{};
  string m_rootFile{};

  string m_pidAlgorithm1{};
  string m_pidAlgorithm2{};
  string m_pidAlgorithm3{};

  std::vector<std::string> m_pidParameters1 = {};
  std::vector<std::string> m_pidParameters2 = {};
  std::vector<std::string> m_pidParameters3 = {};

  int m_n_run = 0;
	int m_n_evt = 0;

  int m_n_jet = 0;
  float m_jet_energy = 0.;

  std::vector<float> m_tags1 = {};
  std::vector<float> m_tags2 = {};
  std::vector<float> m_tags3 = {};

  bool m_read_algo2{};
  bool m_read_algo3{};

  std::vector<int> m_parametersIDs1 = {};
  std::vector<int> m_parametersIDs2 = {};
  std::vector<int> m_parametersIDs3 = {};

  TFile *m_pTFile{};
  TTree *m_pTTree = new TTree("JetTaggingComparison", "JetTaggingComparison");
};
#endif
