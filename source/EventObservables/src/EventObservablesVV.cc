#include "EventObservablesVV.h"

EventObservablesVV aEventObservablesVV;

EventObservablesVV::EventObservablesVV(): EventObservablesBase("EventObservablesVV"),
m_JMP("best_perm_vv") {
    _description = "EventObservablesVV writes relevant observables to root-file " ;

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "5JetCollectionName" ,
        "Name of the Jet collection"  ,
        m_input5JetCollection ,
        std::string("Refined5Jets")
        );

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "6JetCollectionName" ,
        "Name of the Jet collection"  ,
        m_input6JetCollection ,
        std::string("Refined6Jets")
        );
}

void EventObservablesVV::prepareChannelTree() {
    TTree* ttree = getTTree();

	if (m_write_ttree) {
        ttree->Branch("npfosmin5j", &m_npfosmin5j, "npfosmin5j/I");
		ttree->Branch("npfosmax5j", &m_npfosmax5j, "npfosmax5j/I");

        //ttree->Branch("ptjmax5", &m_ptjmax5, "ptjmax5/F");
        //ttree->Branch("pjmax5", &m_pjmax5, "pjmax5/F");
        ttree->Branch("ptjmax6", &m_ptjmax6, "ptjmax6/F");
        ttree->Branch("pjmax6", &m_pjmax6, "pjmax6/F");

        // ttbar
        ttree->Branch("cosBmax12", &m_cosBmax12, "cosBmax12/F");
        ttree->Branch("massWtt4j", &m_massWtt4j, "massWtt4j/F");
        ttree->Branch("massT1tt4j", &m_massT1tt4j, "massT1tt4j/F");
        ttree->Branch("massT2tt4j", &m_massT2tt4j, "massT2tt4j/F");

        // ZZ
        zz_init(ttree);
    }
};

void EventObservablesVV::clearChannelValues() {
    m_5jets.clear();
    m_6jets.clear();

    m_npfosmin5j = 0;
    m_npfosmax5j = 0;

    //m_ptjmax5 = 0.;
    //m_pjmax5 = 0.;
	m_ptjmax6 = 0.;
	m_pjmax6 = 0.;

    // ttbar
    m_cosBmax12 = 0;
    m_massWtt4j = 0;
    m_massT1tt4j = 0;
    m_massT2tt4j = 0;

    // ZZ
    zz_clear();
};

void EventObservablesVV::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *input5JetCollection = pLCEvent->getCollection( m_input5JetCollection );
    LCCollection *input6JetCollection = pLCEvent->getCollection( m_input6JetCollection );

    // NPFOS MIN/MAX
    std::tie(m_npfosmin5j, m_npfosmax5j) = nPFOsMinMax(input5JetCollection);

    // TREAT 5 JET COLLECTION
    /*
    for (int i=0; i < 5; ++i) {
        ReconstructedParticle* jet = (ReconstructedParticle*) input5JetCollection->getElementAt(i);

        m_pjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
        m_ptjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
        m_5jets.push_back(jet);
    }*/

    // TREAT 6 JET COLLECTION
    for (int i=0; i < 6; ++i) {
        ReconstructedParticle* jet = (ReconstructedParticle*) input6JetCollection->getElementAt(i);

        m_pjmax6 = std::max(m_pjmax6, (float)v4(jet).P());
        m_ptjmax6 = std::max(m_ptjmax6, (float)v4(jet).Pt());
        m_6jets.push_back(jet);
    }

    // TREAT 4 JET COLLECTION
    // TTBAR: FORM W and TOPS
    ReconstructedParticle *jetbmax1 = m_jets[m_bTagsSorted[0].first];
    ReconstructedParticle *jetbmax2 = m_jets[m_bTagsSorted[1].first];
    ReconstructedParticle *jetbmax3 = m_jets[m_bTagsSorted[2].first];
    ReconstructedParticle *jetbmax4 = m_jets[m_bTagsSorted[3].first];

    TVector3 pjbmax1 = TVector3(jetbmax1->getMomentum());
    TVector3 pjbmax2 = TVector3(jetbmax2->getMomentum());

    m_cosBmax12 = pjbmax1.Dot(pjbmax2)/pjbmax1.Mag()/pjbmax2.Mag();
    m_massWtt4j = inv_mass(jetbmax3, jetbmax4);
    m_massT1tt4j = inv_mass(jetbmax1, jetbmax3, jetbmax4);
    m_massT2tt4j = inv_mass(jetbmax2, jetbmax3, jetbmax4);

    // ZZ: CHECK BY CHI2
    zz_update(m_jets);
};