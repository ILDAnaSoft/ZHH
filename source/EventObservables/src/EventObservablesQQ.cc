#include "EventObservablesQQ.h"

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ"),
m_JMP("best_perm_qq") {
    _description = "EventObservablesQQ writes relevant observables to root-file " ;

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "4JetCollectionName" ,
        "Name of the Jet collection"  ,
        m_input4JetCollection ,
        std::string("Refined4Jets")
        );
}

void EventObservablesQQ::prepareChannelTree() {
    TTree* ttree = getTTree();

	if (m_write_ttree) {
        ttree->Branch("npfosmin6j", &m_npfosmin6j, "npfosmin6j/I");
		ttree->Branch("npfosmax6j", &m_npfosmax6j, "npfosmax6j/I");

        ttree->Branch("cosjmax4", &m_cosjmax4, "cosjmax4/F");
        ttree->Branch("ptjmax4", &m_ptjmax4, "ptjmax4/F");
        ttree->Branch("pjmax4", &m_pjmax4, "pjmax4/F");

        ttree->Branch("pxj5", &m_pxj5, "pxj5/F");
		ttree->Branch("pyj5", &m_pyj5, "pyj5/F");
		ttree->Branch("pzj5", &m_pzj5, "pzj5/F");
		ttree->Branch("ej5", &m_ej5, "ej5/F");

		ttree->Branch("pxj6", &m_pxj6, "pxj6/F");
		ttree->Branch("pyj6", &m_pyj6, "pyj6/F");
		ttree->Branch("pzj6", &m_pzj6, "pzj6/F");
		ttree->Branch("ej6", &m_ej6, "ej6/F");

        ttree->Branch("me_jet_matching_chi2", &m_lcme_jet_matching_chi2, "me_jet_matching_chi2/F");
        ttree->Branch("me_jet_matching_mz", &m_lcme_jet_matching_mz, "me_jet_matching_chi2/F");
        ttree->Branch("me_jet_matching_mh1", &m_lcme_jet_matching_mh1, "me_jet_matching_mh1/F");
        ttree->Branch("me_jet_matching_mh2", &m_lcme_jet_matching_mh2, "me_jet_matching_mh2/F");

        zz_init(ttree);

        // ttbar
        ttree->Branch("tt_mw1", &m_tt_mw1, "tt_mw1/F");
        ttree->Branch("tt_mw2", &m_tt_mw2, "tt_mw2/F");
        ttree->Branch("tt_mt1", &m_tt_mt1, "tt_mt1/F");
        ttree->Branch("tt_mt2", &m_tt_mt2, "tt_mt2/F");
        ttree->Branch("tt_chi2", &m_tt_chi2, "tt_chi2/F");

        // ZHH
        ttree->Branch("zhh_mz", &m_zhh_mz, "zhh_mz/F");
        ttree->Branch("zhh_mh1", &m_zhh_mh1, "zhh_mh1/F");
        ttree->Branch("zhh_mh2", &m_zhh_mh2, "zhh_mh2/F");
        ttree->Branch("zhh_chi2", &m_zhh_chi2, "zhh_chi2/F");
    }

    m_tt_target_masses = { kMassW, kMassW, kMassTop, kMassTop };
    m_tt_target_resolutions = { kSigmaMassW, kSigmaMassW, kSigmaMassTop, kSigmaMassTop };

    m_zhh_target_masses = { kMassZ, kMassH, kMassH };
    m_zhh_target_resolutions = { kSigmaMassZ, kSigmaMassH, kSigmaMassH };
};

void EventObservablesQQ::clearChannelValues() {
    m_4jets.clear();

    m_npfosmin6j = 0;
    m_npfosmax6j = 0;

    m_cosjmax4 = 0.;
    m_pjmax4 = 0.;
    m_ptjmax4 = 0.;

    m_pxj5 = 0.;
    m_pyj5 = 0.;
    m_pzj5 = 0.;
    m_ej5 = 0.;

    m_pxj6 = 0.;
    m_pyj6 = 0.;
    m_pzj6 = 0.;
    m_ej6 = 0.;

    // ttbar
    m_tt_mw1 = 0.;
    m_tt_mw2 = 0.;
    m_tt_mt1 = 0.;
    m_tt_mt2 = 0.;
    m_tt_chi2 = 0.;

    // ZHH
    m_zhh_mz = 0.;
    m_zhh_mh1 = 0.;
    m_zhh_mh2 = 0.;
    m_zhh_chi2 = 0.;

    // matrix elements
    m_lcme_jet_matching_chi2 = 0.;
    m_lcme_jet_matching_mz = 0.;
    m_lcme_jet_matching_mh1 = 0.;
    m_lcme_jet_matching_mh2 = 0.;

    zz_clear();
};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // -> m_jets
    LCCollection *input4JetCollection = pLCEvent->getCollection( m_input4JetCollection ); // -> m_4jets
    
    if ( m_jets.size() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin6j, m_npfosmax6j) = nPFOsMinMax(inputJetCollection);

        // TREAT 4 JET COLLECTION
        for (int i=0; i < 4; ++i) {
            ReconstructedParticle* jet = (ReconstructedParticle*) input4JetCollection->getElementAt(i);
            ROOT::Math::PxPyPzEVector jet_v4 = v4(jet);
				
			float currentJetMomentum = jet_v4.P();

            if (currentJetMomentum > m_pjmax4)
                m_cosjmax4 = jet_v4.Pz() / currentJetMomentum;

            m_ptjmax4 = std::max(m_ptjmax4, (float)v4(jet).Pt());
            m_pjmax4 = std::max(m_pjmax4, currentJetMomentum);
            m_4jets.push_back(jet);
        }

        // ZZ
        zz_update(m_4jets);

        // TREAT 6 JET COLLECTION
        // A. GENERAL OBSERVABLES

        // sort four vectors by b-tag value (ascending); entries A, B assumed to be b jets
        std::vector<ROOT::Math::PxPyPzEVector> v4_6jets_sorted {
            v4(m_jets[m_bTagsSorted[5].first]),
            v4(m_jets[m_bTagsSorted[4].first]),
            v4(m_jets[m_bTagsSorted[3].first]),
            v4(m_jets[m_bTagsSorted[2].first]),
            v4(m_jets[m_bTagsSorted[1].first]), // A
            v4(m_jets[m_bTagsSorted[0].first])  // B
        };

        // B. TTBAR CHI2        
        std::vector<unsigned short> tt_jet_matching;
        std::vector<float> tt_masses;

        std::tie(tt_jet_matching, tt_masses, m_tt_chi2) = pairJetsByMass(v4_6jets_sorted, m_tt_target_masses, m_tt_target_resolutions,
            []( const std::vector<ROOT::Math::PxPyPzEVector> v4j,
                const std::vector<unsigned short> perm,
                std::vector<float> &masses,
                const std::vector<float> targets,
                const std::vector<float> resolutions) {    

                if ((perm[4] == 4 || perm[4] == 5) && (perm[5] == 4 || perm[5] == 5)) {
                    masses[0] = inv_mass(v4j[perm[0]], v4j[perm[1]]); // W1
                    masses[1] = inv_mass(v4j[perm[2]], v4j[perm[3]]); // W2
                    masses[2] = inv_mass(v4j[perm[0]], v4j[perm[1]], v4j[perm[4]]); // Top1
                    masses[3] = inv_mass(v4j[perm[2]], v4j[perm[3]], v4j[perm[5]]); // Top2

                    return std::pow(((masses[0] - targets[0])/resolutions[0]), 2)
                         + std::pow(((masses[1] - targets[1])/resolutions[1]), 2) 
                         + std::pow(((masses[2] - targets[2])/resolutions[2]), 2)
                         + std::pow(((masses[3] - targets[3])/resolutions[3]), 2);
                } else {
                    // skip permutations where entries 4,5 are not the b jets
                    return 999999.;
                }
            }
        );

        m_tt_mw1 = tt_masses[0];
        m_tt_mw2 = tt_masses[1];
        m_tt_mt1 = tt_masses[2];
        m_tt_mt2 = tt_masses[3];

        // C. ZHH
        std::vector<unsigned short> zhh_jet_matching;
        std::vector<float> zhh_masses;

        std::tie(zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass(v4_6jets_sorted, m_zhh_target_masses, m_zhh_target_resolutions,
            []( const std::vector<ROOT::Math::PxPyPzEVector> v4j,
                const std::vector<unsigned short> perm,
                std::vector<float> &masses,
                const std::vector<float> targets,
                const std::vector<float> resolutions) {    

                if ((perm[4] == 4 || perm[4] == 5) && (perm[5] == 4 || perm[5] == 5)) {
                    masses[0] = inv_mass(v4j[perm[0]], v4j[perm[1]]); // Z
                    masses[1] = inv_mass(v4j[perm[2]], v4j[perm[3]]); // H
                    // masses[2] always the same, sufficient to calculate after chi2

                    return std::pow(((masses[0] - targets[0])/resolutions[0]), 2)
                         + std::pow(((masses[1] - targets[1])/resolutions[1]), 2);
                } else {
                    // skip permutations where entries 4,5 are not the b jets
                    return 999999.;
                }
            }
        );
        zhh_masses[2] = inv_mass(v4_6jets_sorted[4], v4_6jets_sorted[5]);

        m_zhh_mz = zhh_masses[0];
        m_zhh_mh1 = zhh_masses[1];
        m_zhh_mh2 = zhh_masses[2];

        // B.1. CHI2
        
        // B.2. MATRIX ELEMENTS
        // find best matching to Z boson to eliminate some combinatorics
        std::vector<unsigned short> jet_matching_by_mass;
        std::vector<float> dijetmasses;

        // reconstruct Z and H boson using jets with lowest b tag values
        std::tie(jet_matching_by_mass, dijetmasses, m_lcme_jet_matching_chi2) = pairJetsByMass(getElements(inputJetCollection,
            std::vector<int>{m_bTagsSorted[5].first, m_bTagsSorted[4].first, m_bTagsSorted[3].first, m_bTagsSorted[2].first} ), {23, 25});

        m_lcme_jet_matching_mz = dijetmasses[0];
        m_lcme_jet_matching_mh1 = dijetmasses[1];
        m_lcme_jet_matching_mh2 = dijetmasses[2];
        
        TLorentzVector v4_jet_from_z1 = v4old(m_jets[jet_matching_by_mass[0]]);
        TLorentzVector v4_jet_from_z2 = v4old(m_jets[jet_matching_by_mass[1]]);

        // assume charm (background), per default
        unsigned int z1_decay_pdg = 4;
        if ((m_bTagValues[jet_matching_by_mass[0]] + m_bTagValues[jet_matching_by_mass[1]]) > 
            (m_cTagValues[jet_matching_by_mass[0]] + m_cTagValues[jet_matching_by_mass[1]]))
            // assume bottom
            z1_decay_pdg = 5;

        // assume remaining jets to be b-jets
        calculateMatrixElements(z1_decay_pdg, 5, v4_jet_from_z1, v4_jet_from_z2,
                                v4old(m_jets[jet_matching_by_mass[2]]), v4old(m_jets[jet_matching_by_mass[3]]),
                                v4old(m_jets[m_bTagsSorted[0].first]), v4old(m_jets[m_bTagsSorted[1].first]), true);
    }
};

void EventObservablesQQ::setJetMomenta() {
    EventObservablesBase::setJetMomenta();

	m_pxj5 = m_jets[4]->getMomentum()[0];
	m_pyj5 = m_jets[4]->getMomentum()[1];
	m_pzj5 = m_jets[4]->getMomentum()[2];
	m_ej5  = m_jets[4]->getEnergy();

    m_pxj6 = m_jets[5]->getMomentum()[0];
	m_pyj6 = m_jets[5]->getMomentum()[1];
	m_pzj6 = m_jets[5]->getMomentum()[2];
	m_ej6  = m_jets[5]->getEnergy();
};