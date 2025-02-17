#include "EventObservablesFromZZ.h"
#include "EventObservablesBase.h"

void EventObservablesFromZZ::zz_init(TTree *pTTree){
    pTTree->Branch("zz_bestdijetpairing", &m_zz_bestdijetpairing);
    pTTree->Branch("zz_bestchi2", &m_zz_bestchi2, "zz_bestchi2/F");
    pTTree->Branch("zz_mz1", &m_zz_mz1, "zz_mz1/F");
    pTTree->Branch("zz_mz2", &m_zz_mz2, "zz_mz2/F");
};

void EventObservablesFromZZ::zz_update(ReconstructedParticleVec input4JetCollection){
    FloatVec zz_bestdijetmasses;
    std::tie(m_zz_bestdijetpairing, zz_bestdijetmasses, m_zz_bestchi2) = EventObservablesBase::pairJetsByMass(input4JetCollection, {23, 23});

    m_zz_mz1 = zz_bestdijetmasses[0];
    m_zz_mz2 = zz_bestdijetmasses[1];
};

void EventObservablesFromZZ::zz_clear(){
    m_zz_bestdijetpairing.clear();
    m_zz_bestchi2 = 0.;
    m_zz_mz1 = 0.;
    m_zz_mz2 = 0.;
};