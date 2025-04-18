#ifndef ZHH_v4_h
#define ZHH_v4_h 1

#include <EVENT/ReconstructedParticle.h>
#include <Math/Vector4D.h>
#include <vector>
#include "ParticleConstraint.h"

inline ROOT::Math::PxPyPzEVector v4(ParticleConstraint* p){
	double fourVec[4] = {0};
	p->getFourMomentum(1, fourVec);
	streamlog_out(MESSAGE) << "FOURVEC E:pX:pY:pZ" << fourVec[0] << " " << fourVec[1] << " " << fourVec[2] << " " << fourVec[3] << std::endl;
	return ROOT::Math::PxPyPzEVector( fourVec[1], fourVec[2], fourVec[3], fourVec[0] );
}
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