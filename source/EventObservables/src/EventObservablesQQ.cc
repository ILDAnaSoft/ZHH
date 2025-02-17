#include "EventObservablesQQ.h"

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ"),
m_JMP("best_perm_qq") {
    _description = "EventObservablesQQ writes relevant observables to root-file " ;
}

void EventObservablesQQ::prepareChannelTree() {
    TTree* ttree = getTTree();

	if (m_write_ttree) {
        ttree->Branch("npfosmin6j", &m_npfosmin6j, "npfosmin6j/I");
		ttree->Branch("npfosmax6j", &m_npfosmax6j, "npfosmax6j/I");

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
    }
};

void EventObservablesQQ::clearChannelValues() {
    m_npfosmin6j = 0;
    m_npfosmax6j = 0;

    m_pxj5 = 0.;
    m_pyj5 = 0.;
    m_pzj5 = 0.;
    m_ej5 = 0.;

    m_pxj6 = 0.;
    m_pyj6 = 0.;
    m_pzj6 = 0.;
    m_ej6 = 0.;

    m_lcme_jet_matching_chi2 = 0.;
    m_lcme_jet_matching_mz = 0.;
    m_lcme_jet_matching_mh1 = 0.;
    m_lcme_jet_matching_mh2 = 0.;
};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // -> m_jets
    
    if ( m_jets.size() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin6j, m_npfosmax6j) = nPFOsMinMax(inputJetCollection);
        
        // MATRIX ELEMENTS
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