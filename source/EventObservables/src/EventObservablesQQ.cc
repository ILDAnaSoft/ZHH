#include "EventObservablesQQ.h"

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ"),
m_JMP("best_perm_qq") {
    _description = "EventObservablesQQ writes relevant observables to root-file " ;
}

void EventObservablesQQ::prepareChannelTree() {
    TTree* ttree = getTTree();

	if (m_write_ttree) {
        ttree->Branch("px31", &m_px31, "px31/F");
		ttree->Branch("py31", &m_py31, "py31/F");
		ttree->Branch("pz31", &m_pz31, "pz31/F");
		ttree->Branch("e31", &m_e31, "e31/F");

		ttree->Branch("px32", &m_px32, "px32/F");
		ttree->Branch("py32", &m_py32, "py32/F");
		ttree->Branch("pz32", &m_pz32, "pz32/F");
		ttree->Branch("e32", &m_e32, "e32/F");

        ttree->Branch("me_jet_matching_chi2", &m_lcme_jet_matching_chi2, "me_jet_matching_chi2/F");
        ttree->Branch("me_jet_matching_mz", &m_lcme_jet_matching_mz, "me_jet_matching_chi2/F");
        ttree->Branch("me_jet_matching_mh1", &m_lcme_jet_matching_mh1, "me_jet_matching_mh1/F");
        ttree->Branch("me_jet_matching_mh2", &m_lcme_jet_matching_mh2, "me_jet_matching_mh2/F");
    }
};

void EventObservablesQQ::clearChannelValues() {

};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    
    if ( inputJetCollection->getNumberOfElements() == m_nAskedJets() ) {
        // find best matching to Z boson to eliminate some combinatorics
        std::vector<unsigned short> jet_matching_by_mass;
        vector<float> dijetmasses;

        std::tie(jet_matching_by_mass, dijetmasses, m_lcme_jet_matching_chi2) = pairJetsByMass(getElements(inputJetCollection, {0, 1, 2, 3, 4, 5}), {23, 25, 25});

        m_lcme_jet_matching_mz = dijetmasses[0];
        m_lcme_jet_matching_mh1 = dijetmasses[1];
        m_lcme_jet_matching_mh2 = dijetmasses[2];
        
        TLorentzVector v4_jet_from_z1 = v4(inputJetCollection->getElementAt(jet_matching_by_mass[0]));
        TLorentzVector v4_jet_from_z2 = v4(inputJetCollection->getElementAt(jet_matching_by_mass[1]));

        // assume charm (background), per default
        int z1_decay_pdg = 4;
        if ((m_bTagValues[jet_matching_by_mass[0]] + m_bTagValues[jet_matching_by_mass[1]]) > 
            (m_cTagValues[jet_matching_by_mass[0]] + m_cTagValues[jet_matching_by_mass[1]]))
            // assume bottom
            z1_decay_pdg = 5;

        m_px31 = v4_jet_from_z1.Px();
        m_py31 = v4_jet_from_z1.Py();
        m_pz31 = v4_jet_from_z1.Pz();
        m_e31 = v4_jet_from_z1.E();

        m_px32 = v4_jet_from_z2.Px();
        m_py32 = v4_jet_from_z2.Py();
        m_pz32 = v4_jet_from_z2.Pz();
        m_e32 = v4_jet_from_z2.E();

        // assume remaining jets to be b-jets
        calculateMatrixElements(4, 5, v4_jet_from_z1, v4_jet_from_z2,
                                v4(inputJetCollection->getElementAt(jet_matching_by_mass[2])), v4(inputJetCollection->getElementAt(jet_matching_by_mass[3])),
                                v4(inputJetCollection->getElementAt(jet_matching_by_mass[4])), v4(inputJetCollection->getElementAt(jet_matching_by_mass[5])));
    }
};