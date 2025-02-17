#include "EventObservablesLL.h"

EventObservablesLL aEventObservablesLL;

EventObservablesLL::EventObservablesLL(): EventObservablesBase("EventObservablesLL"),
m_JMP("best_perm_ll") {
    _description = "EventObservablesLL writes relevant observables to root-file " ;
}

void EventObservablesLL::prepareChannelTree() {
    TTree* ttree = getTTree();

	if (m_write_ttree) {
        ttree->Branch("npfosmin4j", &m_npfosmin4j, "npfosmin4j/I");
		ttree->Branch("npfosmax4j", &m_npfosmax4j, "npfosmax4j/I");

        ttree->Branch("px31", &m_px31, "px31/F");
		ttree->Branch("py31", &m_py31, "py31/F");
		ttree->Branch("pz31", &m_pz31, "pz31/F");
		ttree->Branch("e31", &m_e31, "e31/F");

		ttree->Branch("px32", &m_px32, "px32/F");
		ttree->Branch("py32", &m_py32, "py32/F");
		ttree->Branch("pz32", &m_pz32, "pz32/F");
		ttree->Branch("e32", &m_e32, "e32/F");

        ttree->Branch("mzll_pre_pairing", &m_mzll_pre_pairing, "mzll_pre_pairing/F");
        ttree->Branch("paired_lep_type", &m_paired_lep_type, "paired_lep_type/I");
    }
};

void EventObservablesLL::clearChannelValues() {
    m_npfosmin4j = 0;
    m_npfosmax4j = 0;

    m_px31 = 0.;
    m_py31 = 0.;
    m_pz31 = 0.;
    m_e31 = 0.;

    m_px32 = 0.;
    m_py32 = 0.;
    m_pz32 = 0.;
    m_e32 = 0.;

    m_paired_lep_type = 0;

    // m_mzll = 0.;
    // m_m_diff_z = 0.;
	m_mzll_pre_pairing = 0.;

	
};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
    // LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );

    if ( inputLepPairCollection->getNumberOfElements() == m_nAskedIsoLeps() && inputJetCollection->getNumberOfElements() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin4j, m_npfosmax4j) = nPFOsMinMax(inputJetCollection);

        // MATRIX ELEMENT
        //m_mzll = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
        //m_m_diff_z = fabs( m_mzll - 91.2 );
        m_mzll_pre_pairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");
        
        // ---------- SAVE TYPES OF PAIRED ISOLATED LEPTONS ----------
        ReconstructedParticle* paired_isolep1 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(0));
        ReconstructedParticle* paired_isolep2 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(1));

        // for ME calculation, first lepton must be the negative charged one
        if (paired_isolep1->getCharge() > 0){
            std::swap(paired_isolep1, paired_isolep2);
        }

        m_paired_lep_type = abs(paired_isolep1->getType());

        TLorentzVector v4_paired_isolep1 = v4old(paired_isolep1);
        TLorentzVector v4_paired_isolep2 = v4old(paired_isolep2);

        m_px31 = v4_paired_isolep1.Px();
        m_py31 = v4_paired_isolep1.Py();
        m_pz31 = v4_paired_isolep1.Pz();
        m_e31 = v4_paired_isolep1.E();

        m_px32 = v4_paired_isolep2.Px();
        m_py32 = v4_paired_isolep2.Py();
        m_pz32 = v4_paired_isolep2.Pz();
        m_e32 = v4_paired_isolep2.E();

        assert(m_paired_lep_type == 11 || m_paired_lep_type == 13);

        calculateMatrixElements(m_paired_lep_type, 5, v4_paired_isolep1, v4_paired_isolep2,
                                v4old(inputJetCollection->getElementAt(0)), v4old(inputJetCollection->getElementAt(1)),
                                v4old(inputJetCollection->getElementAt(2)), v4old(inputJetCollection->getElementAt(3)), false);
    }
};