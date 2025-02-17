#ifndef ZHH_v4_h
#define ZHH_v4_h 1

#include <EVENT/ReconstructedParticle.h>
#include <Math/Vector4D.h>

ROOT::Math::PxPyPzEVector v4(EVENT::ReconstructedParticle* p){
	return ROOT::Math::PxPyPzEVector( p->getMomentum()[0], p->getMomentum()[1], p->getMomentum()[2], p->getEnergy() );
}

#endif