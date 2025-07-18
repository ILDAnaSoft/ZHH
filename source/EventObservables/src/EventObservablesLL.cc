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

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "ISOElectrons" ,
        "Name of the isolated electron collection"  ,
        m_inputIsoElectrons ,
        std::string("ISOElectrons")
        );
    
    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "ISOMuons" ,
        "Name of the isolated electron collection"  ,
        m_inputIsoMuons ,
        std::string("ISOMuons")
        );

    registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
        "ISOTaus" ,
        "Name of the isolated tau collection"  ,
        m_inputIsoTaus ,
        std::string("ISOTaus")
        );
}

void EventObservablesLL::prepareChannelTree() {
    TTree* ttree = getTTree();

    m_bTagValues_2Jets  = std::vector<double>(2, -1.);
    m_bTagValues_2Jets2 = std::vector<double>(2, -1.);

	if (m_write_ttree) {
        ttree->Branch("npfosmin4j", &m_npfosmin4j, "npfosmin4j/I");
		ttree->Branch("npfosmax4j", &m_npfosmax4j, "npfosmax4j/I");

        ttree->Branch("pxl1", &m_pxl1, "pxl1/F");
		ttree->Branch("pyl1", &m_pyl1, "pyl1/F");
		ttree->Branch("pzl1", &m_pzl1, "pzl1/F");
		ttree->Branch("el1", &m_el1, "el1/F");
        ttree->Branch("typel1", &m_typel1, "typel1/I");

		ttree->Branch("pxl2", &m_pxl2, "pxl2/F");
		ttree->Branch("pyl2", &m_pyl2, "pyl2/F");
		ttree->Branch("pzl2", &m_pzl2, "pzl2/F");
		ttree->Branch("el2", &m_el2, "el2/F");
        ttree->Branch("typel2", &m_typel2, "typel2/I");

        ttree->Branch("plmin", &m_plmin, "plmin/F");
        ttree->Branch("plmax", &m_plmax, "plmax/F");
        ttree->Branch("mvalepminus", &m_mvalepminus, "mvalepminus/F");
        ttree->Branch("mvalepplus", &m_mvalepplus, "mvalepplus/F");
        ttree->Branch("mzll", &m_mzll, "mzll/F");
        ttree->Branch("mzll_pre_pairing", &m_mzll_pre_pairing, "mzll_pre_pairing/F");
        

        // 2 jets
        ttree->Branch("pxj1_2Jets", &m_pxj1_2Jets, "pxj1_2Jets/F");
		ttree->Branch("pyj1_2Jets", &m_pyj1_2Jets, "pyj1_2Jets/F");
		ttree->Branch("pzj1_2Jets", &m_pzj1_2Jets, "pzj1_2Jets/F");
		ttree->Branch("ej1_2Jets", &m_ej1_2Jets, "ej1_2Jets/F");

		ttree->Branch("pxj2_2Jets", &m_pxj2_2Jets, "pxj2_2Jets/F");
		ttree->Branch("pyj2_2Jets", &m_pyj2_2Jets, "pyj2_2Jets/F");
		ttree->Branch("pzj2_2Jets", &m_pzj2_2Jets, "pzj2_2Jets/F");
		ttree->Branch("ej2_2Jets", &m_ej2_2Jets, "ej2_2Jets/F");

        ttree->Branch("cosJ1_2Jets", &m_cosJ1_2Jets, "cosJ1_2Jets/F");
        ttree->Branch("cosJ2_2Jets", &m_cosJ2_2Jets, "cosJ2_2Jets/F");
        ttree->Branch("cosJ12_2Jets", &m_cosJ12_2Jets, "cosJ12_2Jets/F");
        ttree->Branch("cosJ1Z_2Jets", &m_cosJ1Z_2Jets, "cosJ1Z_2Jets/F");
        ttree->Branch("cosJ2Z_2Jets", &m_cosJ2Z_2Jets, "cosJ2Z_2Jets/F");
        ttree->Branch("cosJZMax_2Jets", &m_cosJZMax_2Jets, "cosJZMax_2Jets/F");

        ttree->Branch("ptjmin2", &m_ptjmin2, "ptjmin2/F");
        ttree->Branch("pjmin2", &m_pjmin2, "pjmin2/F");

        ttree->Branch("ptjmax2", &m_ptjmax2, "ptjmax2/F");
        ttree->Branch("pjmax2", &m_pjmax2, "pjmax2/F");

        ttree->Branch("m_inv_2Jets", &m_m_inv_2Jets, "m_inv_2Jets/F");

        ttree->Branch("yminus2", &m_yMinus2, "yminus2/F");
        ttree->Branch("yplus2", &m_yPlus2, "yplus2/F");

        ttree->Branch("bmax1_2Jets", &m_bmax1_2Jets, "bmax1_2Jets/F");
        ttree->Branch("bmax2_2Jets", &m_bmax2_2Jets, "bmax2_2Jets/F");

        ttree->Branch("bTagValues_2Jets", &m_bTagValues_2Jets);

        if (m_use_tags2) {
            ttree->Branch("bmax12_2Jets", &m_bmax12_2Jets, "bmax12_2Jets/F");
            ttree->Branch("bmax22_2Jets", &m_bmax22_2Jets, "bmax22_2Jets/F");

            ttree->Branch("bTagValues_2Jets2", &m_bTagValues_2Jets2);
        }
        

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
    m_typel1 = 0;

    m_pxl2 = 0.;
    m_pyl2 = 0.;
    m_pzl2 = 0.;
    m_el2 = 0.;
    m_typel2 = 0;

    m_plmin = 0;
    m_plmax = 0;
    m_mvalepminus = 0;
    m_mvalepplus = 0;
    m_mzll = 0.;
	m_mzll_pre_pairing = 0.;

    // 2 jets
    m_pxj1_2Jets = 0.;
	m_pyj1_2Jets = 0.;
	m_pzj1_2Jets = 0.;
	m_ej1_2Jets  = 0.;

    m_pxj2_2Jets = 0.;
	m_pyj2_2Jets = 0.;
	m_pzj2_2Jets = 0.;
	m_ej2_2Jets  = 0.;

    m_ptjmin2 = 0.;
    m_pjmin2 = 0.;

    m_ptjmax2 = 0.;
    m_pjmax2 = 0.;

    m_m_inv_2Jets = 0.;

    m_cosJ1_2Jets = 0.;
	m_cosJ2_2Jets = 0.;
    m_cosJ12_2Jets = 0.;
    m_cosJ1Z_2Jets = 0.;
    m_cosJ2Z_2Jets = 0.;
    m_cosJZMax_2Jets = 0.;

    m_yMinus2 = 0.;
    m_yPlus2 = 0.;

    m_bTagValues_2Jets.clear();
    m_bTagValues_2Jets2.clear();

    // 4 jets
    m_mbmax12 = 0.;
    m_mbmax34 = 0.;
};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetCharges();
    
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    LCCollection *input2JetCollection = pLCEvent->getCollection( m_input2JetCollection );
    LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );

    if ( inputLepPairCollection->getNumberOfElements() == m_nAskedIsoLeps() && inputJetCollection->getNumberOfElements() == m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin4j, m_npfosmax4j, std::ignore) = nPFOsMinMax(inputJetCollection);

        //m_mzll = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
        //m_m_diff_z = fabs( m_mzll - 91.2 );
        m_mzll_pre_pairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");
        
        int pairedLeptonType = inputLepPairCollection->parameters().getIntVal("PairedType");
        LCCollection *inputLeptonCollection;

        switch(pairedLeptonType) {
            case 11: inputLeptonCollection = pLCEvent->getCollection( m_inputIsoElectrons ); break;
            case 13: inputLeptonCollection = pLCEvent->getCollection( m_inputIsoMuons ); break;
            case 15: inputLeptonCollection = pLCEvent->getCollection( m_inputIsoTaus ); break;
        }

        IntVec pairedLeptonIDx;
        inputLepPairCollection->parameters().getIntVals("PairedLeptonIDx", pairedLeptonIDx);
        
        // ---------- SAVE TYPES OF PAIRED ISOLATED LEPTONS ----------
        ReconstructedParticle* paired_isolep1 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(0));
        ReconstructedParticle* paired_isolep2 = dynamic_cast<ReconstructedParticle*>( inputLepPairCollection->getElementAt(1));

        // for ME calculation, leptons must be stored consistently
        // we store lepton first, then anti-lepton
        if (paired_isolep1->getCharge() > 0)
            std::swap(paired_isolep1, paired_isolep2);

        TLorentzVector v4_paired_isolep1 = v4old(paired_isolep1);
        TLorentzVector v4_paired_isolep2 = v4old(paired_isolep2);

        m_plmin = min(v4_paired_isolep1.P(), v4_paired_isolep2.P());
        m_plmax = max(v4_paired_isolep1.P(), v4_paired_isolep2.P());

        if (pairedLeptonType == 11 || pairedLeptonType == 13) {
            FloatVec mvaOutputIsoLepTagging;
            inputLeptonCollection->getParameters().getFloatVals("ISOLepTagging", mvaOutputIsoLepTagging);

            float mvaOutputIsoLep1 = mvaOutputIsoLepTagging[pairedLeptonIDx[0]];
            float mvaOutputIsoLep2 = mvaOutputIsoLepTagging[pairedLeptonIDx[1]];

            m_mvalepminus = min(mvaOutputIsoLep1, mvaOutputIsoLep2);
            m_mvalepplus = max(mvaOutputIsoLep1, mvaOutputIsoLep2);
        }

        // START EVALUATE 2 JET COLLECTION
        TLorentzVector momentumZv4 = v4_paired_isolep1 + v4_paired_isolep2;
        TVector3 momentumZ = momentumZv4.Vect();

        ReconstructedParticle* jets_2Jets[2] = {dynamic_cast<ReconstructedParticle*>(input2JetCollection->getElementAt(0)),
                                                dynamic_cast<ReconstructedParticle*>(input2JetCollection->getElementAt(1))}; 

        ROOT::Math::PxPyPzEVector p4J1_2Jets = v4(jets_2Jets[0]);
        ROOT::Math::PxPyPzEVector p4J2_2Jets = v4(jets_2Jets[1]);

        m_pxj1_2Jets = p4J1_2Jets.X();
        m_pyj1_2Jets = p4J1_2Jets.Y();
        m_pzj1_2Jets = p4J1_2Jets.Z();
        m_ej1_2Jets  = p4J1_2Jets.E();

        m_pxj2_2Jets = p4J2_2Jets.X();
        m_pyj2_2Jets = p4J2_2Jets.Y();
        m_pzj2_2Jets = p4J2_2Jets.Z();
        m_ej2_2Jets  = p4J2_2Jets.E();

        double pJ1_2Jets = p4J1_2Jets.P();
        double pJ2_2Jets = p4J2_2Jets.P();

        m_ptjmin2 = std::min(p4J1_2Jets.Pt(), p4J2_2Jets.Pt());
        m_pjmin2 = std::min(pJ1_2Jets, pJ2_2Jets);

        m_ptjmax2 = std::max(p4J1_2Jets.Pt(), p4J2_2Jets.Pt());
        m_pjmax2 = std::max(pJ1_2Jets, pJ2_2Jets);

        m_m_inv_2Jets = (p4J1_2Jets + p4J2_2Jets).M();

        TVector3 momentum1_2Jets = jets_2Jets[0]->getMomentum();
        TVector3 momentum2_2Jets = jets_2Jets[1]->getMomentum();

        m_cosJ1_2Jets = momentum1_2Jets.CosTheta();
        m_cosJ2_2Jets = momentum2_2Jets.CosTheta();
        m_cosJ12_2Jets = momentum1_2Jets.Dot(momentum2_2Jets)/pJ1_2Jets/pJ2_2Jets;
        m_cosJ1Z_2Jets = momentum1_2Jets.Dot(momentumZ)/pJ1_2Jets/momentumZ.Mag();
        m_cosJ2Z_2Jets = momentum2_2Jets.Dot(momentumZ)/pJ2_2Jets/momentumZ.Mag();
        m_cosJZMax_2Jets = std::max(m_cosJ1Z_2Jets, m_cosJ2Z_2Jets);

        PIDHandler jet2PIDh(input2JetCollection);

        int algo_y = jet2PIDh.getAlgorithmID("yth");
        const ParticleID & ythID = jet2PIDh.getParticleID(jets_2Jets[0], algo_y); // same arguments for all jets

        FloatVec params_y = ythID.getParameters();
        m_yMinus2 = params_y[jet2PIDh.getParameterIndex(algo_y, "y12")];
        m_yPlus2 = params_y[jet2PIDh.getParameterIndex(algo_y, "y23")];

        // flavor tagging
        int _FTAlgoID = jet2PIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm);
		int _FTAlgoID2 = m_use_tags2 ? jet2PIDh.getAlgorithmID(m_JetTaggingPIDAlgorithm2) : -1;

        int BTagID = jet2PIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterB);
        //int CTagID = jet2PIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterC);

        int BTagID2 = m_use_tags2 ? jet2PIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterB2) : -1;
        //int CTagID2 = m_use_tags2 ? jet2PIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterC2) : -1;

        // extract flavor tag values
        for (int i=0; i<2; ++i) {
            const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(jet2PIDh.getParticleID(jets_2Jets[i], _FTAlgoID));
            const FloatVec& FTPara = FTImpl.getParameters();

            m_bTagValues_2Jets[i] = FTPara[BTagID];

            if (m_use_tags2) {
                const ParticleIDImpl& FTImpl2 = dynamic_cast<const ParticleIDImpl&>(jet2PIDh.getParticleID(jets_2Jets[i], _FTAlgoID2));
                const FloatVec& FTPara2 = FTImpl2.getParameters();

                m_bTagValues_2Jets2[i] = FTPara2[BTagID2];
            }
        }

        m_bmax1_2Jets = std::max(m_bTagValues_2Jets[0], m_bTagValues_2Jets[1]);
        m_bmax2_2Jets = std::min(m_bTagValues_2Jets[0], m_bTagValues_2Jets[1]);

        if (m_use_tags2) {
            m_bmax12_2Jets = std::max(m_bTagValues_2Jets2[0], m_bTagValues_2Jets2[1]);
            m_bmax22_2Jets = std::min(m_bTagValues_2Jets2[0], m_bTagValues_2Jets2[1]);
        }

        // END EVALUATE 2 JET COLLECTION

        // TREAT 4 JET COLELCTION

        // VANILLA JET MATCHING
        std::vector<float> zhh_masses;
        std::vector<ROOT::Math::PxPyPzEVector> jet_v4 = v4(m_jets);

        std::tie(m_zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass(jet_v4, { 25, 25 });

        m_zhh_mh1 = std::min(zhh_masses[0], zhh_masses[1]);
        m_zhh_mh2 = std::max(zhh_masses[0], zhh_masses[1]);
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

        if (m_pairedLepType != 11 && m_pairedLepType != 13 && m_pairedLepType != 15) { // this should not happen...? but it does seldom...
            throw EVENT::Exception("Invalid paired IsoLepton type");
            std::cerr << "Got lepton type " << m_pairedLepType << " in event " << m_nEvt << std::endl;
            m_errorCodes.push_back(1001);
        }
    
        m_pxl1 = v4_paired_isolep1.Px();
        m_pyl1 = v4_paired_isolep1.Py();
        m_pzl1 = v4_paired_isolep1.Pz();
        m_el1 = v4_paired_isolep1.E();
        m_typel1 = paired_isolep1->getType();

        m_pxl2 = v4_paired_isolep2.Px();
        m_pyl2 = v4_paired_isolep2.Py();
        m_pzl2 = v4_paired_isolep2.Pz();
        m_el2 = v4_paired_isolep2.E();
        m_typel2 = paired_isolep2->getType();

        m_mzll = momentumZv4.M();
        m_zhh_mz = m_mzll;

        streamlog_out(DEBUG) << "PairedLeptons of type " << m_pairedLepType << " to M=" << m_mzll << std::endl;

        calculateMatrixElements(m_pairedLepType, 5, v4_paired_isolep1, v4_paired_isolep2,
                                v4old(inputJetCollection->getElementAt(0)), v4old(inputJetCollection->getElementAt(1)),
                                v4old(inputJetCollection->getElementAt(2)), v4old(inputJetCollection->getElementAt(3)), false);

        // TODO: try calculating with kinfit outputs
    }
};

void EventObservablesLL::calculateSimpleZHHChi2() {
	// do it in the above loop instead
}