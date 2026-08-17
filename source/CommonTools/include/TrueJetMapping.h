#ifndef TrueJetMapping_h
#define TrueJetMapping_h 1

#include <EVENT/MCParticle.h>
#include "TVector3.h"
#include "lcio.h"
#include <vector>

using namespace lcio;

// Groups one or more TrueJet indices (see TrueJet_Parser) that originate from the
// same initial elementon, e.g. a hard quark and any TrueJet(s) created by a
// subsequent quark -> quark + gluon splitting. This mirrors the class of the same
// name in MarlinMLFlavorTagging/common, see JetFilterProcessor::matchTrueJetsByAngularSpace
// for the reference recombination logic.
class TrueJetMapping {
  private:
    std::vector<int> m_truejet_idx;
    TVector3 m_momentum;
    double m_energy;
    MCParticle* m_initial_elementon;

  public:
    TrueJetMapping ():
      m_truejet_idx({}),
      m_momentum(TVector3({ 0., 0., 0. })),
      m_energy(0.),
      m_initial_elementon(nullptr) {};
    TrueJetMapping (std::vector<int> idx, TVector3 mom, double energy, MCParticle* initial);
    TrueJetMapping(const TrueJetMapping&) = default;
    TrueJetMapping& operator=(const TrueJetMapping&) = default;

    std::vector<int> getIndex() const { return m_truejet_idx; };
    TVector3 getMomentum() const { return m_momentum; };
    double getEnergy() const { return m_energy; };
    TVector3 getUnitVector() { TVector3 mom = getMomentum(); mom.SetMag(1.); return mom; };
    MCParticle* getInitialElementon() const { return m_initial_elementon; };

    bool operator<(const TrueJetMapping& other) const {
      return m_truejet_idx < other.m_truejet_idx;
    }
};

#endif
