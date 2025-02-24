#ifndef ZHH_v4_h
#define ZHH_v4_h 1

#include <EVENT/ReconstructedParticle.h>
#include <Math/Vector4D.h>
#include <vector>

inline ROOT::Math::PxPyPzEVector v4(EVENT::ReconstructedParticle* p){
	return ROOT::Math::PxPyPzEVector( p->getMomentum()[0], p->getMomentum()[1], p->getMomentum()[2], p->getEnergy() );
}
inline ROOT::Math::PxPyPzEVector v4(EVENT::LCObject* p){
	return v4((ReconstructedParticle*)p);
}
inline std::vector<ROOT::Math::PxPyPzEVector> v4(std::vector<EVENT::ReconstructedParticle*> in){
	std::vector<ROOT::Math::PxPyPzEVector> result;

	for (size_t i = 0; i < in.size(); i++) {
		result.push_back( v4(in[i]) );
	}

	return result;
}

#endif