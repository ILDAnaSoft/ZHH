#include "EventObservablesLL.h"

// errorCodes:
// 1001: lepton type neither 11 nor 13 (?). we force it to 11

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

        ttree->Branch("pxl1", &m_pxl1, "pxl1/F");
		ttree->Branch("pyl1", &m_pyl1, "pyl1/F");
		ttree->Branch("pzl1", &m_pzl1, "pzl1/F");
		ttree->Branch("el1", &m_el1, "el1/F");

		ttree->Branch("pxl2", &m_pxl2, "pxl2/F");
		ttree->Branch("pyl2", &m_pyl2, "pyl2/F");
		ttree->Branch("pzl2", &m_pzl2, "pzl2/F");
		ttree->Branch("el2", &m_el2, "el2/F");

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

    m_pxl1 = 0.;
    m_pyl1 = 0.;
    m_pzl1 = 0.;
    m_el1 = 0.;

    m_pxl2 = 0.;
    m_pyl2 = 0.;
    m_pzl2 = 0.;
    m_el2 = 0.;

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

        // VANILLA JET MATCHING
        std::vector<float> zhh_masses;
        std::vector<ROOT::Math::PxPyPzEVector> jet_v4 = v4(m_jets);

        std::tie(m_zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass(jet_v4, { 25, 25 });

        m_zhh_mh1 = zhh_masses[0];
        m_zhh_mh2 = zhh_masses[1];
        m_zhh_mhh = (jet_v4[0] + jet_v4[1] + jet_v4[2] + jet_v4[3]).M();

        std::vector<ROOT::Math::PxPyPzEVector> dijets = {
            v4(paired_isolep1) + v4(paired_isolep2),
            jet_v4[m_zhh_jet_matching[0]] + jet_v4[m_zhh_jet_matching[1]],
            jet_v4[m_zhh_jet_matching[2]] + jet_v4[m_zhh_jet_matching[3]]
        };
    
        for (ROOT::Math::PxPyPzEVector dijet: dijets) {
            if (dijet.P() > m_zhh_p1st) {
                m_zhh_p1st = dijet.P();
                m_zhh_cosTh1st = cos(dijet.Theta());
            }
        }

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
        if (m_paired_lep_type != 11 && m_paired_lep_type != 13) { // this should not happen...? but it does seldom...
            m_paired_lep_type = 11; 
            m_errorCodes.push_back(1001);
        }
    
        m_pxl1 = v4_paired_isolep1.Px();
        m_pyl1 = v4_paired_isolep1.Py();
        m_pzl1 = v4_paired_isolep1.Pz();
        m_el1 = v4_paired_isolep1.E();

        m_pxl2 = v4_paired_isolep2.Px();
        m_pyl2 = v4_paired_isolep2.Py();
        m_pzl2 = v4_paired_isolep2.Pz();
        m_el2 = v4_paired_isolep2.E();

        m_mzll = momentumZv4.M();
        m_zhh_mz = m_mzll;

        if (m_paired_lep_type != 11 && m_paired_lep_type != 13)
            throw EVENT::Exception("Invalid pairedLepType " + std::to_string( m_paired_lep_type));

        streamlog_out(DEBUG) << "PairedLeptons of type " << m_paired_lep_type << " to M=" << m_mzll << std::endl;

        calculateMatrixElements(m_paired_lep_type, 5, v4_paired_isolep1, v4_paired_isolep2,
                                v4old(inputJetCollection->getElementAt(0)), v4old(inputJetCollection->getElementAt(1)),
                                v4old(inputJetCollection->getElementAt(2)), v4old(inputJetCollection->getElementAt(3)), false);

        // TODO: try calculating with kinfit outputs
    }
};

void EventObservablesLL::calculateSimpleZHHChi2() {
	// do it in the above loop instead
}