#include "EventObservablesQQ.h"

// errorCodes:
// -

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

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "ZHHKinfitJetCollection" ,
        "Name of the input ZHH jet kinfit collection. jet pairing used for bTagZ"  ,
        m_zhhKinfitJetCollection ,
        std::string("JetsKinFitQQ_ZHH")
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

        ttree->Branch("bmax5", &m_bmax5, "bmax5/F");
        ttree->Branch("bmax6", &m_bmax6, "bmax6/F");
        ttree->Branch("btagz", &m_bTagZ, "btagz/F");

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

    m_bmax5 = 0.;
    m_bmax6 = 0.;
    m_bTagZ = 0.;

    // ttbar
    m_tt_mw1 = 0.;
    m_tt_mw2 = 0.;
    m_tt_mt1 = 0.;
    m_tt_mt2 = 0.;
    m_tt_chi2 = 0.;

    zz_clear();
};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetMomenta();
    
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // -> m_jets
    LCCollection *input4JetCollection = pLCEvent->getCollection( m_input4JetCollection ); // -> m_4jets
    
    if ( m_jets.size() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin6j, m_npfosmax6j, std::ignore) = nPFOsMinMax(inputJetCollection);

        // TREAT 4 JET COLLECTION
        for (int i=0; i < input4JetCollection->getNumberOfElements(); ++i) {
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

        m_bmax5 = m_bTagsSorted[4].second;
        m_bmax6 = m_bTagsSorted[5].second;

        // use jet matching of Kinfit processors to find btags from jets used to form Z boson
        /*
        try {
            std::vector<int> jet_matching{};

            LCCollection *zhhKinfitCollection = pLCEvent->getCollection( m_zhhKinfitJetCollection ); // -> m_jets

            zhhKinfitCollection->getParameters().getIntVals(m_jetMatchingParameter().c_str(), jet_matching);

			assert(jet_matching.size() == m_nJets);

            m_bTagZ = m_bTagValues[jet_matching[0]] + m_bTagValues[jet_matching[1]];
        } catch (DataNotAvailableException &e) {
            streamlog_out(MESSAGE) << "ZHHKinfitJetCollection not found in event. bTagZ will be 0" << std::endl;
        }*/

        // assume Z consists of two jets with least b tag
        m_bTagZ = m_bmax5 + m_bmax6;

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
        

        // B.1. CHI2
        
        // B.2. MATRIX ELEMENTS        
        TLorentzVector v4_jet_from_z1 = v4old(m_jets[m_bTagsSorted[5].first]);
        TLorentzVector v4_jet_from_z2 = v4old(m_jets[m_bTagsSorted[4].first]);

        // assume s per default
        unsigned int z1_decay_pdg = 3;

        float bTagSum = m_bTagValues[m_bTagsSorted[5].first] +  m_bTagValues[m_bTagsSorted[4].first];
        float cTagSum = m_cTagValues[m_bTagsSorted[5].first] +  m_cTagValues[m_bTagsSorted[4].first];
        float oTagSum = 1 - bTagSum - cTagSum;

        if (bTagSum > oTagSum && bTagSum > cTagSum)
            z1_decay_pdg = 5; // b
        else if (cTagSum > oTagSum && cTagSum > bTagSum)
            z1_decay_pdg = 4; // c

        // assume remaining jets to be b-jets
        calculateMatrixElements(z1_decay_pdg, 5, v4_jet_from_z1, v4_jet_from_z2,
                                v4old(m_jets[2 + m_zhh_jet_matching[0]]), v4old(m_jets[2 + m_zhh_jet_matching[1]]),
                                v4old(m_jets[2 + m_zhh_jet_matching[2]]), v4old(m_jets[2 + m_zhh_jet_matching[3]]), true);
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

void EventObservablesQQ::calculateSimpleZHHChi2() {
	std::vector<float> zhh_masses;
    std::vector<ROOT::Math::PxPyPzEVector> jet_v4 = v4(m_jets);

    std::tie(m_zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass({
        jet_v4[m_bTagsSorted[0].first],
        jet_v4[m_bTagsSorted[1].first],
        jet_v4[m_bTagsSorted[2].first],
        jet_v4[m_bTagsSorted[3].first]
    }, { 25, 25 });

    const unsigned short idx_translation [4] = {
        m_bTagsSorted[0].first,
        m_bTagsSorted[1].first,
        m_bTagsSorted[2].first,
        m_bTagsSorted[3].first};

    std::vector<ROOT::Math::PxPyPzEVector> dijets = {
        jet_v4[m_bTagsSorted[4].first] + jet_v4[m_bTagsSorted[5].first], // Z
        jet_v4[idx_translation[m_zhh_jet_matching[0]]] + jet_v4[idx_translation[m_zhh_jet_matching[1]]], // H
        jet_v4[idx_translation[m_zhh_jet_matching[2]]] + jet_v4[idx_translation[m_zhh_jet_matching[3]]] // H
    };

    for (ROOT::Math::PxPyPzEVector dijet: dijets) {
        if (dijet.P() > m_zhh_p1st) {
            m_zhh_p1st = dijet.P();
            m_zhh_cosTh1st = cos(dijet.Theta());
        }
    }

    m_zhh_mz = dijets[0].M();
    m_zhh_mh1 = zhh_masses[0];
    m_zhh_mh2 = zhh_masses[1];
    m_zhh_mhh = (jet_v4[m_bTagsSorted[0].first] + jet_v4[m_bTagsSorted[1].first] + jet_v4[m_bTagsSorted[2].first] + jet_v4[m_bTagsSorted[3].first]).M();
}