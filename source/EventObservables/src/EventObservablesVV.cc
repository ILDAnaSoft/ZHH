#include "EventObservablesVV.h"

// errorCodes:
// -

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
        ttree->Branch("ptjmax6", &m_ptjmax6, "ptjmax6/F");
        ttree->Branch("pjmax6", &m_pjmax6, "pjmax6/F");

        // ttbar 5j
        ttree->Branch("npfosmin5j", &m_npfosmin5j, "npfosmin5j/I");
		ttree->Branch("npfosmax5j", &m_npfosmax5j, "npfosmax5j/I");
        ttree->Branch("ptjmax5", &m_ptjmax5, "ptjmax5/F");
        ttree->Branch("pjmax5", &m_pjmax5, "pjmax5/F");

        ttree->Branch("yminus5j", &m_yMinus5j, "yminus5j/F");
        ttree->Branch("yplus5j", &m_yPlus5j, "yplus5j/F");

        ttree->Branch("massWtt5j", &m_massWtt5j, "massWtt5j/F");
        ttree->Branch("massT1tt5j", &m_massT1tt5j, "massT1tt5j/F");
        ttree->Branch("massT2tt5j", &m_massT2tt5j, "massT2tt5j/F");

        // ttbar 4j
        ttree->Branch("massWtt4j", &m_massWtt4j, "massWtt4j/F");
        ttree->Branch("massT1tt4j", &m_massT1tt4j, "massT1tt4j/F");
        ttree->Branch("massT2tt4j", &m_massT2tt4j, "massT2tt4j/F");

        // ZZ
        zz_init(ttree);
    }
};

void EventObservablesVV::clearChannelValues() {
    m_5jets.clear();
    //m_6jets.clear();

	m_ptjmax6 = 0.;
	m_pjmax6 = 0.;

    // ttbar 5j
    m_npfosmin5j = 0;
    m_npfosmax5j = 0;
    m_ptjmax5 = 0.;
    m_pjmax5 = 0.;

    m_yMinus5j = 0.;
	m_yPlus5j = 0.;

    m_massWtt5j = 0.;
	m_massT1tt5j = 0.;
	m_massT2tt5j = 0.;

    // ttbar 4j
    m_massWtt4j = 0;
    m_massT1tt4j = 0;
    m_massT2tt4j = 0;

    // ZZ
    zz_clear();
};

void EventObservablesVV::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetCharges();
    
    LCCollection *input5JetCollection = pLCEvent->getCollection( m_input5JetCollection );
    LCCollection *input6JetCollection = pLCEvent->getCollection( m_input6JetCollection );

    // VANILLA JET MATCHING
    std::vector<float> zhh_masses;
    std::vector<ROOT::Math::PxPyPzEVector> jet_v4 = v4(m_jets);

    std::tie(m_zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass(jet_v4, { 25, 25 });

    m_zhh_mh1 = std::min(zhh_masses[0], zhh_masses[1]);
    m_zhh_mh2 = std::max(zhh_masses[0], zhh_masses[1]);
    m_zhh_mhh = (jet_v4[0] + jet_v4[1] + jet_v4[2] + jet_v4[3]).M();

    std::vector<ROOT::Math::PxPyPzEVector> dijets = {
        jet_v4[m_zhh_jet_matching[0]] + jet_v4[m_zhh_jet_matching[1]],
        jet_v4[m_zhh_jet_matching[2]] + jet_v4[m_zhh_jet_matching[3]]
    };

    for (ROOT::Math::PxPyPzEVector dijet: dijets) {
        if (dijet.P() > m_zhh_p1st) {
            m_zhh_p1st = dijet.P();
            m_zhh_cosTh1st = cos(dijet.Theta());
        }
    }

    // TREAT 5 JET COLLECTION

    // assume jet given by jet5IDxPFOsMin with least No of PFOs is the tau jet
    int jet5IDxPFOsMin;
    std::tie(m_npfosmin5j, m_npfosmax5j, jet5IDxPFOsMin) = nPFOsMinMax(input5JetCollection);

    // reconstruct the semileptonic ttbar system
    // in the 5 jet case, assume the least b-probable jet is a tau jet
    std::vector<std::pair<int, float>> sortedBTagsJ5 =
        sortedTagging(input5JetCollection, m_JetTaggingPIDAlgorithm, m_JetTaggingPIDParameterB);

    // order is with decreasing b-tag    
    for (size_t i=0; i < sortedBTagsJ5.size(); ++i) {
        ReconstructedParticle* jet = (ReconstructedParticle*) input5JetCollection->getElementAt(i);

        m_pjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
        m_ptjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
        
        m_5jets.push_back(jet);
    }

    ReconstructedParticle *jetbmax1_5jets = m_5jets[sortedBTagsJ5[0].first];
    ReconstructedParticle *jetbmax2_5jets = m_5jets[sortedBTagsJ5[1].first];
    ReconstructedParticle *jetbmax3_5jets = m_5jets[sortedBTagsJ5[2].first];
    ReconstructedParticle *jetbmax4_5jets = m_5jets[sortedBTagsJ5[3].first];

    m_massWtt5j = inv_mass(jetbmax3_5jets, jetbmax4_5jets);
    m_massT1tt5j = inv_mass(jetbmax1_5jets, jetbmax3_5jets, jetbmax4_5jets);
    m_massT2tt5j = inv_mass(jetbmax2_5jets, jetbmax3_5jets, jetbmax4_5jets);

    // get yMinus and yPlus
    PIDHandler jet5PIDh(input5JetCollection);
    int algo_y = jet5PIDh.getAlgorithmID("yth");
    const ParticleID & ythID = jet5PIDh.getParticleID(m_5jets[0], algo_y); // same arguments for all jets

    FloatVec params_y = ythID.getParameters();
    m_yMinus5j = params_y[jet5PIDh.getParameterIndex(algo_y, "y45")];
    m_yPlus5j = params_y[jet5PIDh.getParameterIndex(algo_y, "y56")];

    // TREAT 6 JET COLLECTION
    for (int i=0; i < input6JetCollection->getNumberOfElements(); ++i) {
        ReconstructedParticle* jet = (ReconstructedParticle*) input6JetCollection->getElementAt(i);

        m_pjmax6 = std::max(m_pjmax6, (float)v4(jet).P());
        m_ptjmax6 = std::max(m_ptjmax6, (float)v4(jet).Pt());
        //m_6jets.push_back(jet);
    }

    // TREAT 4 JET COLLECTION
    // TTBAR: FORM W and TOPS
    ReconstructedParticle *jetbmax1 = m_jets[m_bTagsSorted[0].first];
    ReconstructedParticle *jetbmax2 = m_jets[m_bTagsSorted[1].first];
    ReconstructedParticle *jetbmax3 = m_jets[m_bTagsSorted[2].first];
    ReconstructedParticle *jetbmax4 = m_jets[m_bTagsSorted[3].first];

    TVector3 pjbmax1 = TVector3(jetbmax1->getMomentum());
    TVector3 pjbmax2 = TVector3(jetbmax2->getMomentum());

    m_massWtt4j = inv_mass(jetbmax3, jetbmax4);
    m_massT1tt4j = inv_mass(jetbmax1, jetbmax3, jetbmax4);
    m_massT2tt4j = inv_mass(jetbmax2, jetbmax3, jetbmax4);

    // ZZ: CHECK BY CHI2
    zz_update(m_jets);
};

void EventObservablesVV::calculateSimpleZHHChi2() {
	
}