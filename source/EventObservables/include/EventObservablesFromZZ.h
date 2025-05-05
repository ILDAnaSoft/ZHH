#ifndef EventObservablesFromZZ_h
#define EventObservablesFromZZ_h 1

#include <string>
#include <vector>
#include "marlin/Processor.h"
#include "EVENT/ReconstructedParticle.h"
#include "TTree.h"

class EventObservablesFromZZ {
	public:
        virtual ~EventObservablesFromZZ() = default;
        void zz_init(TTree *pTTree);
        void zz_clear();
        void zz_update(ReconstructedParticleVec input4JetCollection);

    protected:
        // data members
        std::vector<unsigned short> m_zz_bestdijetpairing{};
		float m_zz_bestchi2{};
		float m_zz_mz1{};
		float m_zz_mz2{};
};

#endif