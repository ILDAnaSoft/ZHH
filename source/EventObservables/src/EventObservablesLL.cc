#include "EventObservablesLL.h"

EventObservablesLL aEventObservablesLL;

EventObservablesLL::EventObservablesLL(): EventObservablesBase("EventObservablesLL"),
m_JMP("best_perm_ll") {
    _description = "EventObservablesLL writes relevant observables to root-file " ;
}

void EventObservablesLL::prepareChannelTree() {
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

        ttree->Branch("mzll_pre_pairing", &m_mzll_pre_pairing, "mzll_pre_pairing/F");
        ttree->Branch("paired_lep_type", &m_paired_lep_type, "paired_lep_type/I");
    }
};

void EventObservablesLL::clearChannelValues() {
    m_px31 = 0.;
    m_py31 = 0.;
    m_pz31 = 0.;
    m_e31 = 0.;

    m_px32 = 0.;
    m_py32 = 0.;
    m_pz32 = 0.;
    m_e32 = 0.;

    // m_mzll = 0.;
    // m_m_diff_z = 0.;
	m_mzll_pre_pairing = 0.;
    m_paired_lep_type = 0;
	
};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );
    LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );

    if ( inputLepPairCollection->getNumberOfElements() == m_nAskedIsoLeps() ) {
        //m_mzll = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
        //m_m_diff_z = fabs( m_mzll - 91.2 );
        m_mzll_pre_pairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");
        

        // ---------- SAVE TYPES OF PAIRED ISOLATED LEPTONS ----------
        ReconstructedParticle* paired_isolep1 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(0));
        ReconstructedParticle* paired_isolep2 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(1));
        m_paired_lep_type = paired_isolep1->getType();

        // ---------- SAVE MOMENTA + ENERGIES OF ISOLATED LEPTONS ----------
        int lep_type = abs(m_paired_lep_type);

        assert(lep_type == 11 || lep_type == 13);

        // following convention in GENNumCon.h
        // calculateMatrixElements();
        
        //_zzh->SetMomentumFinal(zzh_lortz);
        //_zzh->SetMomentumFinal(zzh_lortz);

    }
};