#ifndef ZHH_inv_mass_h
#define ZHH_inv_mass_h 1

#include "EVENT/ReconstructedParticle.h"

inline double inv_mass(EVENT::ReconstructedParticle* p1, EVENT::ReconstructedParticle* p2){
  double e = p1->getEnergy() + p2->getEnergy();
  double px = p1->getMomentum()[0] + p2->getMomentum()[0];
  double py = p1->getMomentum()[1] + p2->getMomentum()[1];
  double pz = p1->getMomentum()[2] + p2->getMomentum()[2];
  return( sqrt( e*e - px*px - py*py - pz*pz  ) );
}

inline double inv_mass(EVENT::ReconstructedParticle* p1, EVENT::ReconstructedParticle* p2, EVENT::ReconstructedParticle* p3){
  double e = p1->getEnergy() + p2->getEnergy() + p3->getEnergy() ;
  double px = p1->getMomentum()[0] + p2->getMomentum()[0] + p3->getMomentum()[0];
  double py = p1->getMomentum()[1] + p2->getMomentum()[1] + p3->getMomentum()[1];
  double pz = p1->getMomentum()[2] + p2->getMomentum()[2] + p3->getMomentum()[2];
  return( sqrt( e*e - px*px - py*py - pz*pz  ) );
}

inline double inv_mass(ROOT::Math::PxPyPzEVector p1, ROOT::Math::PxPyPzEVector p2){
  return (p1 + p2).M();
}

inline double inv_mass(
  ROOT::Math::PxPyPzEVector p1,
  ROOT::Math::PxPyPzEVector p2,
  ROOT::Math::PxPyPzEVector p3){
  return (p1 + p2 + p3).M();
}

#endif