#include "TrueJetMapping.h"

TrueJetMapping::TrueJetMapping(std::vector<int> idx, TVector3 mom, double energy, MCParticle* initial):
  m_truejet_idx(idx),
  m_momentum(mom),
  m_energy(energy),
  m_initial_elementon(initial) {};
