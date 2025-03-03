#include "EventObservablesLL.h"

EventObservablesLL aEventObservablesLL;

EventObservablesLL::EventObservablesLL(): EventObservablesBase("EventObservablesLL"),
m_JMP("best_perm_ll") {
    _description = "EventObservablesLL writes relevant observables to root-file " ;

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "2JetCollectionName" ,
        "Name of the Jet collection"  ,
        m_input2JetCollection ,
        std::string("Refined2Jets")
        );
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

        ttree->Branch("paired_lep_type", &m_paired_lep_type, "paired_lep_type/I");
        ttree->Branch("plmin", &m_plmin, "plmin/F");
        ttree->Branch("plmax", &m_plmax, "plmax/F");
        ttree->Branch("mvalepminus", &m_mvalepminus, "mvalepminus/F");
        ttree->Branch("mvalepplus", &m_mvalepplus, "mvalepplus/F");
        ttree->Branch("mzll", &m_mzll, "mzll/F");
        ttree->Branch("mzll_pre_pairing", &m_mzll_pre_pairing, "mzll_pre_pairing/F");
        

        // 2 jets
        ttree->Branch("cosJ1_2Jets", &m_cosJ1_2Jets, "cosJ1_2Jets/F");
        ttree->Branch("cosJ2_2Jets", &m_cosJ2_2Jets, "cosJ2_2Jets/F");
        ttree->Branch("cosJ12_2Jets", &m_cosJ12_2Jets, "cosJ12_2Jets/F");
        ttree->Branch("cosJ1Z_2Jets", &m_cosJ1Z_2Jets, "cosJ1Z_2Jets/F");
        ttree->Branch("cosJ2Z_2Jets", &m_cosJ2Z_2Jets, "cosJ2Z_2Jets/F");

        ttree->Branch("ptjmax2", &m_ptjmax2, "ptjmax2/F");
        ttree->Branch("pjmax2", &m_pjmax2, "pjmax2/F");

        // 4 jets
        ttree->Branch("mbmax12", &m_mbmax12, "mbmax12/F");
        ttree->Branch("mbmax34", &m_mbmax34, "mbmax34/F");
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

    m_plmin = 0;
    m_plmax = 0;
    m_mvalepminus = 0;
    m_mvalepplus = 0;
    m_mzll = 0.;
	m_mzll_pre_pairing = 0.;

    // 2 jets
    m_ptjmax2 = 0.;
    m_pjmax2 = 0.;

    m_cosJ1_2Jets = 0.;
	m_cosJ2_2Jets = 0.;
    m_cosJ12_2Jets = 0.;
    m_cosJ1Z_2Jets = 0.;
    m_cosJ2Z_2Jets = 0.;

    // 4 jets
    m_mbmax12 = 0.;
    m_mbmax34 = 0.;
};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetMomenta();
    
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    LCCollection *input2JetCollection = pLCEvent->getCollection( m_input2JetCollection );
    LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
    LCCollection *inputLeptonCollection = pLCEvent->getCollection( m_inputIsolatedleptonCollection );

    if ( inputLepPairCollection->getNumberOfElements() == m_nAskedIsoLeps() && inputJetCollection->getNumberOfElements() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin4j, m_npfosmax4j, std::ignore) = nPFOsMinMax(inputJetCollection);

        //m_mzll = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
        //m_m_diff_z = fabs( m_mzll - 91.2 );
        m_mzll_pre_pairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");
        
        IntVec pairedLeptonIDx;
        inputLepPairCollection->parameters().getIntVals("PairedLeptonIDx", pairedLeptonIDx);
        
        // ---------- SAVE TYPES OF PAIRED ISOLATED LEPTONS ----------
        ReconstructedParticle* paired_isolep1 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(0));
        ReconstructedParticle* paired_isolep2 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(1));

        TLorentzVector v4_paired_isolep1 = v4old(paired_isolep1);
        TLorentzVector v4_paired_isolep2 = v4old(paired_isolep2);

        m_plmin = min(v4_paired_isolep1.P(), v4_paired_isolep2.P());
        m_plmax = max(v4_paired_isolep1.P(), v4_paired_isolep2.P());

        ReconstructedParticle* isolep1 = dynamic_cast<ReconstructedParticle*>( inputLeptonCollection->getElementAt(pairedLeptonIDx[0]));

        FloatVec mvaOutputIsoLepTagging;
        inputLeptonCollection->getParameters().getFloatVals("ISOLepTagging", mvaOutputIsoLepTagging);

        float mvaOutputIsoLep1 = mvaOutputIsoLepTagging[pairedLeptonIDx[0]];
        float mvaOutputIsoLep2 = mvaOutputIsoLepTagging[pairedLeptonIDx[1]];

        m_mvalepminus = min(mvaOutputIsoLep1, mvaOutputIsoLep2);
        m_mvalepminus = max(mvaOutputIsoLep1, mvaOutputIsoLep2);

        // START EVALUATE 2 JET COLLECTION
        TLorentzVector momentumZv4 = v4_paired_isolep1 + v4_paired_isolep2;
        TVector3 momentumZ = momentumZv4.Vect();

        ReconstructedParticle* jets_2Jets[2] = {dynamic_cast<ReconstructedParticle*>(input2JetCollection->getElementAt(0)),
                                                dynamic_cast<ReconstructedParticle*>(input2JetCollection->getElementAt(1))}; 

        ROOT::Math::PxPyPzEVector p4J1_2Jets = v4(jets_2Jets[0]);
        ROOT::Math::PxPyPzEVector p4J2_2Jets = v4(jets_2Jets[1]);

        m_ptjmax2 = std::max(p4J1_2Jets.Pt(), p4J2_2Jets.Pt());
        m_pjmax2 = std::max(p4J1_2Jets.P(), p4J2_2Jets.P());

        TVector3 momentum1_2Jets = jets_2Jets[0]->getMomentum();
        TVector3 momentum2_2Jets = jets_2Jets[1]->getMomentum();
        Double_t pJ1_2Jets = momentum1_2Jets.Mag();
        Double_t pJ2_2Jets = momentum2_2Jets.Mag();

        m_cosJ1_2Jets = momentum1_2Jets.CosTheta();
        m_cosJ2_2Jets = momentum2_2Jets.CosTheta();
        m_cosJ12_2Jets = momentum1_2Jets.Dot(momentum2_2Jets)/pJ1_2Jets/pJ2_2Jets;
        m_cosJ1Z_2Jets = momentum1_2Jets.Dot(momentumZ)/pJ1_2Jets/momentumZ.Mag();
        m_cosJ2Z_2Jets = momentum2_2Jets.Dot(momentumZ)/pJ2_2Jets/momentumZ.Mag();
        // END EVALUATE 2 JET COLLECTION

        // TREAT 4 JET COLELCTION

        // mb12 and mb34
        m_mbmax12 = (
            v4(inputJetCollection->getElementAt(m_bTagsSorted[0].first)) +
            v4(inputJetCollection->getElementAt(m_bTagsSorted[1].first))).M();

        m_mbmax34 = (
            v4(inputJetCollection->getElementAt(m_bTagsSorted[2].first)) +
            v4(inputJetCollection->getElementAt(m_bTagsSorted[3].first))).M();
        
        // MATRIX ELEMENT
        // for ME calculation, first lepton must be the negative charged one
        if (paired_isolep1->getCharge() > 0){
            std::swap(paired_isolep1, paired_isolep2);
        }

        m_paired_lep_type = abs(isolep1->getType());
    
        m_px31 = v4_paired_isolep1.Px();
        m_py31 = v4_paired_isolep1.Py();
        m_pz31 = v4_paired_isolep1.Pz();
        m_e31 = v4_paired_isolep1.E();

        m_px32 = v4_paired_isolep2.Px();
        m_py32 = v4_paired_isolep2.Py();
        m_pz32 = v4_paired_isolep2.Pz();
        m_e32 = v4_paired_isolep2.E();

        m_mzll = (v4_paired_isolep1 + v4_paired_isolep2).M();

        if (m_paired_lep_type != 11 && m_paired_lep_type != 13)
            throw EVENT::Exception("Invalid pairedLepType " + std::to_string( m_paired_lep_type));

        streamlog_out(DEBUG) << "PairedLeptons of type " << m_paired_lep_type << " to M=" << m_mzll << std::endl;

        calculateMatrixElements(m_paired_lep_type, 5, v4_paired_isolep1, v4_paired_isolep2,
                                v4old(inputJetCollection->getElementAt(0)), v4old(inputJetCollection->getElementAt(1)),
                                v4old(inputJetCollection->getElementAt(2)), v4old(inputJetCollection->getElementAt(3)), false);

        // TODO: try calculating with kinfit outputs
    }
};