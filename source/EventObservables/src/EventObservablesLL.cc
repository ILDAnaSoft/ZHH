#include "EventObservablesLL.h"

// errorCodes:
// 1001: lepton type neither 11 nor 13 (?). we force it to 11

EventObservablesLL aEventObservablesLL;

EventObservablesLL::EventObservablesLL(): EventObservablesBase("EventObservablesLL") {
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

    m_2jetTags = std::vector<std::vector<float>>(2);
    m_bTagValues_2Jets  = std::vector<double>(2, -1.);
    m_bTagValues_2Jets2 = std::vector<double>(2, -1.);

    m_leps4v = std::vector<ROOT::Math::PxPyPzEVector>(m_nAskedIsoLeps());
    m_2jets4v = std::vector<ROOT::Math::PxPyPzEVector>(2);
    m_fit4C_masses = std::vector<float>(m_nAskedJets());

	if (m_write_ttree) {
        ttree->Branch("npfosmin4j", &m_npfosmin4j, "npfosmin4j/I");
		ttree->Branch("npfosmax4j", &m_npfosmax4j, "npfosmax4j/I");

        ttree->Branch("lep1_4v", &m_leps4v[0]);
        ttree->Branch("typel1", &m_typel1, "typel1/I");

        ttree->Branch("lep2_4v", &m_leps4v[1]);
        ttree->Branch("typel2", &m_typel2, "typel2/I");

        ttree->Branch("plmin", &m_plmin, "plmin/F");
        ttree->Branch("plmax", &m_plmax, "plmax/F");
        ttree->Branch("mvalepminus", &m_mvalepminus, "mvalepminus/F");
        ttree->Branch("mvalepplus", &m_mvalepplus, "mvalepplus/F");
        ttree->Branch("mzll", &m_mzll, "mzll/F");
        ttree->Branch("mzll_pre_pairing", &m_mzll_pre_pairing, "mzll_pre_pairing/F");
        
        // 2 jets
        ttree->Branch("2jets_m_inv", &m_2jet_m_inv, "2jets_m_inv/F");
        
        ttree->Branch("2jet1_4v", &m_jets4v[0]);
		ttree->Branch("2jet1_m", &m_2jet1_m, "2jet1_m/F");

        ttree->Branch("2jet2_4v", &m_jets4v[0]);
		ttree->Branch("2jet2_m", &m_2jet2_m, "2jet2_m/F");

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

        ttree->Branch("yminus2", &m_yMinus2, "yminus2/F");
        ttree->Branch("yplus2", &m_yPlus2, "yplus2/F");

        ttree->Branch("bmax1_2Jets", &m_bmax1_2Jets, "bmax1_2Jets/F");
        ttree->Branch("bmax2_2Jets", &m_bmax2_2Jets, "bmax2_2Jets/F");

        ttree->Branch("2jetTags", &m_2jetTags);
        ttree->Branch("bTagValues_2Jets", &m_bTagValues_2Jets);

        if (m_use_tags2) {
            ttree->Branch("bmax12_2Jets", &m_bmax12_2Jets, "bmax12_2Jets/F");
            ttree->Branch("bmax22_2Jets", &m_bmax22_2Jets, "bmax22_2Jets/F");

            ttree->Branch("bTagValues_2Jets2", &m_bTagValues_2Jets2);
        }
        

        // 4 jets
        ttree->Branch("mbmax12", &m_mbmax12, "mbmax12/F");
        ttree->Branch("mbmax34", &m_mbmax34, "mbmax34/F");

        ttree->Branch("mcmax12", &m_mcmax12, "mcmax12/F");
        ttree->Branch("mcmax34", &m_mcmax34, "mcmax34/F");
    }
};

void EventObservablesLL::clearChannelValues() {
    m_npfosmin4j = 0;
    m_npfosmax4j = 0;

    m_leps4v[0].SetPxPyPzE(0., 0., 0., 0.);
    m_typel1 = 0;

    m_leps4v[1].SetPxPyPzE(0., 0., 0., 0.);
    m_typel2 = 0;

    m_plmin = 0;
    m_plmax = 0;
    m_mvalepminus = 0;
    m_mvalepplus = 0;
    m_mzll = 0.;
	m_mzll_pre_pairing = 0.;

    // 2 jets
    m_2jets4v[0].SetPxPyPzE(0., 0., 0., 0.);
    m_2jet1_m = 0.;

    m_2jets4v[1].SetPxPyPzE(0., 0., 0., 0.);
    m_2jet2_m = 0.;

    m_ptjmin2 = 0.;
    m_pjmin2 = 0.;

    m_ptjmax2 = 0.;
    m_pjmax2 = 0.;

    m_2jet_m_inv = 0.;

    m_cosJ1_2Jets = 0.;
	m_cosJ2_2Jets = 0.;
    m_cosJ12_2Jets = 0.;
    m_cosJ1Z_2Jets = 0.;
    m_cosJ2Z_2Jets = 0.;
    m_cosJZMax_2Jets = 0.;

    m_yMinus2 = 0.;
    m_yPlus2 = 0.;

    m_2jetTags[0].clear();
    m_2jetTags[1].clear();
    m_bTagValues_2Jets.clear();
    m_bTagValues_2Jets2.clear();

    // 4 jets
    m_mbmax12 = 0.;
    m_mbmax34 = 0.;

    m_mcmax12 = 0.;
    m_mcmax34 = 0.;

    m_leps4cKinFit_4v.clear();
    m_jets4cKinFit_4v.clear();
    m_fit4C_masses.clear();
};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetCharges();
    
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection );
    LCCollection *input2JetCollection = pLCEvent->getCollection( m_input2JetCollection );
    LCCollection *inputLepPairCollection = pLCEvent->getCollection( m_inputLepPairCollection );
    
    if ( inputLepPairCollection->getNumberOfElements() == (int)m_nAskedIsoLeps() &&
         inputJetCollection->getNumberOfElements() == (int)m_nAskedJets() ) {
        // NPFOS MIN/MAX
        std::tie(m_npfosmin4j, m_npfosmax4j, std::ignore) = nPFOsMinMax(inputJetCollection);

        //m_mzll = inputLepPairCollection->parameters().getFloatVal("RecoLepsInvMass");
        //m_m_diff_z = fabs( m_mzll - 91.2 );
        m_mzll_pre_pairing = inputLepPairCollection->parameters().getFloatVal("IsoLepsInvMass");

        LCCollection *inputLeptonCollection;

        switch(m_pairedLepType) {
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

        if (m_pairedLepType == 11 || m_pairedLepType == 13) {
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

        m_2jets4v[0] = p4J1_2Jets;
        m_2jets4v[1] = p4J2_2Jets;

        m_2jet1_m = p4J1_2Jets.M();
        m_2jet2_m = p4J2_2Jets.M();
        m_2jet_m_inv = (p4J1_2Jets + p4J2_2Jets).M();

        double pJ1_2Jets = p4J1_2Jets.P();
        double pJ2_2Jets = p4J2_2Jets.P();

        m_ptjmin2 = std::min(p4J1_2Jets.Pt(), p4J2_2Jets.Pt());
        m_pjmin2 = std::min(pJ1_2Jets, pJ2_2Jets);

        m_ptjmax2 = std::max(p4J1_2Jets.Pt(), p4J2_2Jets.Pt());
        m_pjmax2 = std::max(pJ1_2Jets, pJ2_2Jets);

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
        int BBarTagID = jet2PIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameterBbar);
        int BTagID2 = m_use_tags2 ? jet2PIDh.getParameterIndex(_FTAlgoID2, m_JetTaggingPIDParameterB2) : -1;

        // extract flavor tag values
        for (int i=0; i<2; ++i) {
            const ParticleIDImpl& FTImpl = dynamic_cast<const ParticleIDImpl&>(jet2PIDh.getParticleID(jets_2Jets[i], _FTAlgoID));
            const FloatVec& FTPara = FTImpl.getParameters();

            m_bTagValues_2Jets[i] = FTPara[BTagID] + FTPara[BBarTagID];

            // write all requested parameters
            for (size_t j = 0; j < m_JetTaggingPIDParameters.size(); j++) {
                int param_id = jet2PIDh.getParameterIndex(_FTAlgoID, m_JetTaggingPIDParameters[j]);
                if (param_id + 1 > (int)FTPara.size()) {
                    std::cerr << "Parameter error: Param " << j << ", value=" << m_JetTaggingPIDParameters[j] << std::endl;
                    throw EVENT::Exception("No flavor tagging value for parameter");
                }
                
                m_2jetTags[i].push_back(FTPara[param_id]);
            }

            if (m_use_tags2) {
                const ParticleIDImpl& FTImpl2 = dynamic_cast<const ParticleIDImpl&>(jet2PIDh.getParticleID(jets_2Jets[i], _FTAlgoID2));
                const FloatVec& FTPara2 = FTImpl2.getParameters();

                m_bTagValues_2Jets2[i] = FTPara2[BTagID2];

                if (std::isnan(m_bTagValues_2Jets[i]) && !std::isnan(m_bTagValues_2Jets2[i]))
                    m_bTagValues_2Jets[i] = m_bTagValues_2Jets2[i];
            }
        }

        m_bmax1_2Jets = std::max(m_bTagValues_2Jets[0], m_bTagValues_2Jets[1]);
        m_bmax2_2Jets = std::min(m_bTagValues_2Jets[0], m_bTagValues_2Jets[1]);

        if (m_use_tags2) {
            m_bmax12_2Jets = std::max(m_bTagValues_2Jets2[0], m_bTagValues_2Jets2[1]);
            m_bmax22_2Jets = std::min(m_bTagValues_2Jets2[0], m_bTagValues_2Jets2[1]);
        }

        // DECAY ANGLES IN Z=qqbar REST FRAME
        // DONE IN MATRIX ELEMENT CODE SEPARATELY

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

        for (size_t i = 0; i < inputLKF_solveNuCollection->getNumberOfElements(); i++) {
        ReconstructedParticle* lepton = (ReconstructedParticle*) inputLKF_solveNuCollection->getElementAt(i);
        m_leps4cKinFit_4v.push_back(v4(lepton));
        }
        
        for (size_t i = 0; i < inputJKF_solveNuCollection->getNumberOfElements(); i++) {
        ReconstructedParticle* jet = (ReconstructedParticle*) inputJKF_solveNuCollection->getElementAt(i);
        m_jets4cKinFit_4v.push_back(v4(jet));
        }        

        if (m_leps4cKinFit_4v.size() == m_nAskedIsoLeps() && (int)m_JMK_best.size() >= m_nAskedJets()) {
            m_jets4v_post_4C_kinfit[0] = m_jets4cKinFit_4v[0];
            m_jets4v_post_4C_kinfit[1] = m_jets4cKinFit_4v[1];
            m_jets4v_post_4C_kinfit[2] = m_jets4cKinFit_4v[2];
            m_jets4v_post_4C_kinfit[3] = m_jets4cKinFit_4v[3];

            m_jetsMasses_post_4C_kinfit[0] = m_jets4cKinFit_4v[0].M();
            m_jetsMasses_post_4C_kinfit[1] = m_jets4cKinFit_4v[1].M();
            m_jetsMasses_post_4C_kinfit[2] = m_jets4cKinFit_4v[2].M();
            m_jetsMasses_post_4C_kinfit[3] = m_jets4cKinFit_4v[3].M();

            m_fit4C_mz = (m_leps4cKinFit_4v[0]+m_leps4cKinFit_4v[1]).M();
            m_fit4C_masses.push_back((m_jets4cKinFit_4v[m_JMK_best[0]]+m_jets4cKinFit_4v[m_JMK_best[1]]).M());
            m_fit4C_masses.push_back((m_jets4cKinFit_4v[m_JMK_best[2]]+m_jets4cKinFit_4v[m_JMK_best[3]]).M());

            float mHdelta1 = abs(m_fit4C_masses[0] - 125.0);
            float mHdelta2 = abs(m_fit4C_masses[1] - 125.0);

            m_fit4C_mh1 = mHdelta1 < mHdelta2 ? m_fit4C_masses[0] : m_fit4C_masses[1]; 
            m_fit4C_mh2 = mHdelta1 < mHdelta2 ? m_fit4C_masses[1] : m_fit4C_masses[0]; 
        
            streamlog_out(MESSAGE) << "lepton energies:" << m_leps4cKinFit_4v[0].E() << ", " << m_leps4cKinFit_4v[1].E() << endl;
            streamlog_out(MESSAGE) << "Fit probs: " << m_fitprob_ZHH << ", " << m_fitprob_ZZH << endl;
            streamlog_out(MESSAGE) << "Fit chi2s: " << m_fitchi2_ZHH << ", " << m_fitchi2_ZZH << endl;
            streamlog_out(MESSAGE) << "ZHH perm: " << m_JMK_ZHH[0] << ", " << m_JMK_ZHH[1] << ", " << m_JMK_ZHH[2] << ", " << m_JMK_ZHH[3] << endl;
            streamlog_out(MESSAGE) << "ZZH perm: " << m_JMK_ZZH[0] << ", " << m_JMK_ZZH[1] << ", " << m_JMK_ZZH[2] << ", " << m_JMK_ZZH[3] << endl;
            streamlog_out(MESSAGE) << "permuation: " << m_JMK_best[0] << ", " << m_JMK_best[1] << ", " << m_JMK_best[2] << ", " << m_JMK_best[3] << endl;
            streamlog_out(MESSAGE) << "jet energies:" << m_jets4cKinFit_4v[m_JMK_best[0]].E() << ", " << m_jets4cKinFit_4v[m_JMK_best[1]].E() << ", " << m_jets4cKinFit_4v[m_JMK_best[2]].E() << ", " << m_jets4cKinFit_4v[m_JMK_best[3]].E() << endl;
            streamlog_out(MESSAGE) << "jet energies:" << m_jets4cKinFit_4v[0].E() << ", " << m_jets4cKinFit_4v[1].E() << ", " << m_jets4cKinFit_4v[2].E() << ", " << m_jets4cKinFit_4v[3].E() << endl;
            streamlog_out(MESSAGE) << "dijet masses: " << m_fit4C_mz << ", " << m_fit4C_mh1 << ", " << m_fit4C_mh2 << endl;
        } else {
            streamlog_out(MESSAGE) << "Kinfit failed, will be skipped for EventObservablesLL" << endl;
        }


	    // mb12 and mb34
        m_mbmax12 = (
            v4(inputJetCollection->getElementAt(m_bTagsSorted[0].first)) +
            v4(inputJetCollection->getElementAt(m_bTagsSorted[1].first))).M();

        m_mbmax34 = (
            v4(inputJetCollection->getElementAt(m_bTagsSorted[2].first)) +
            v4(inputJetCollection->getElementAt(m_bTagsSorted[3].first))).M();
        
        m_mcmax12 = (
            v4(inputJetCollection->getElementAt(m_cTagsSorted[0].first)) +
            v4(inputJetCollection->getElementAt(m_cTagsSorted[1].first))).M();

        m_mcmax34 = (
            v4(inputJetCollection->getElementAt(m_cTagsSorted[2].first)) +
            v4(inputJetCollection->getElementAt(m_cTagsSorted[3].first))).M();

        // MATRIX ELEMENT

        if (m_pairedLepType != 11 && m_pairedLepType != 13 && m_pairedLepType != 15) { // this should not happen...? but it does seldom...
            throw EVENT::Exception("Invalid paired IsoLepton type");
            std::cerr << "Got lepton type " << m_pairedLepType << " in event " << m_nEvt << std::endl;
            m_errorCodes.push_back(1001);
        }
    
        m_leps4v[0] = v4_paired_isolep1;
        m_typel1 = paired_isolep1->getType();

        m_leps4v[1] = v4_paired_isolep2;
        m_typel2 = paired_isolep2->getType();

        m_mzll = momentumZv4.M();
        m_zhh_mz = m_mzll;

        streamlog_out(MESSAGE) << "PairedLeptons of type " << m_pairedLepType << " to M=" << m_mzll << std::endl;

        #ifdef CALC_ME
        calculateMatrixElements(m_pairedLepType, 5, v4_paired_isolep1, v4_paired_isolep2,
                                v4old(inputJetCollection->getElementAt(0)), v4old(inputJetCollection->getElementAt(1)),
                                v4old(inputJetCollection->getElementAt(2)), v4old(inputJetCollection->getElementAt(3)), false);
        #endif

        // TODO: try calculating with kinfit outputs
    }

    // handle TrueJet (if run)
    try {
		pLCEvent->getCollection(_trueJetCollectionName);
		m_useTrueJet = true;
	} catch(DataNotAvailableException &e) {
		streamlog_out(ERROR) << "No TrueJet collection " << _trueJetCollectionName << " found. Skipping recording of truth four momenta" << std::endl;
		m_useTrueJet = false;
	}

	const std::string process_name = pLCEvent->getParameters().getStringVal("processName");

	if (m_nJets == m_nAskedJets() && m_useTrueJet) {
		TrueJet_Parser* trueJet = this;
		trueJet->getall( pLCEvent );
		
		vector<int> trueHadronicJetIndices;
		vector<int> trueLeptonIndices;
		vector<int> trueChargedLeptonIndices;
		vector<int> trueISRIndices;
		float smallestSumCosAngle = getMatchingByAngularSpace(m_jets, m_reco2TrueJetIndex, m_true2RecoJetIndex, trueHadronicJetIndices, trueLeptonIndices, trueISRIndices);

		// case in which number of hadronic jets == m_nJets
		m_trueRecoJetsMapped = smallestSumCosAngle < 99;

		// store hadronic true jets
		m_trueJetN = trueHadronicJetIndices.size();

		// store leptons
		m_trueLeptonN = trueLeptonIndices.size();

		std::copy_if(trueLeptonIndices.begin(), trueLeptonIndices.end(), std::back_inserter(trueChargedLeptonIndices),
			[trueJet](int index){
				int pdg = abs( ( (ReconstructedParticle*)trueJet->jet(index) )->getParticleIDs()[0]->getPDG() );
				return (pdg == 11 || pdg == 13 || pdg == 15);
			});

		// sort leptons ASC by energy
		std::sort(trueChargedLeptonIndices.begin(), trueChargedLeptonIndices.end(), [trueJet]( const int& index1, const int& index2 ) {
			const ReconstructedParticle* true_lep1 = trueJet->jet(index1);
			const ReconstructedParticle* true_lep2 = trueJet->jet(index2);

			return true_lep1->getEnergy() > true_lep2->getEnergy();
		});

		if (trueChargedLeptonIndices.size() >= m_nAskedIsoLeps()) {
			for (unsigned int i = 0; i < m_nAskedIsoLeps(); i++) {
				int index = trueChargedLeptonIndices[i];
				const ReconstructedParticle* tjet = trueJet->jet(index);

				m_trueLeptonMomenta[i] = v4(tjet);
				m_trueLeptonPDGs[i] = tjet->getParticleIDs()[0]->getPDG();
			}
		}

		std::vector<int> icn_pdgs = {23, 25};

		if (m_trueRecoJetsMapped) {
			std::vector<int> truejetpermICNs;

			//streamlog_out(DEBUG3) << "number of icns = " << nicn << endl;
			// sort ICNs: any Higgs first; important as this is the order assumed at MEM calculation for the jet matching 
			std::vector<int> icn_indices_sorted(trueJet->nicn());
			std::iota(icn_indices_sorted.begin(), icn_indices_sorted.end(), 0);

			std::sort(icn_indices_sorted.begin(), icn_indices_sorted.end(), [trueJet]( const int& icn1_idx, const int& icn2_idx ) {
				return abs(trueJet->pdg_icn_parent(icn1_idx)) == 25; // && trueJet->E_icn(icn1_idx) > trueJet->E_icn(icn2_idx); // need to have some sorting besides PDG=25
			});

			std::cerr << "ICN order: "; 
			for (int &i_icn: icn_indices_sorted)
				std::cerr << abs(pdg_icn_parent(i_icn)) << " ";
			std::cerr << std::endl;
			
			for (int &i_icn: icn_indices_sorted) {
				std::cerr << "PDG(icn)=" << abs(pdg_icn_parent(i_icn)) << std::endl;
				auto jet_ids = jets_of_initial_cn(i_icn);

				for (const int &icn_pdg: icn_pdgs) {
					if (abs(pdg_icn_parent(i_icn)) == icn_pdg) {
						IntVec icn_comps_pdgs = pdg_icn_comps(i_icn);
						bool is_dileptonic = icn_comps_pdgs.size() == 2 && (
							abs(icn_comps_pdgs[0]) == 11 ||
							abs(icn_comps_pdgs[0]) == 13 ||
							abs(icn_comps_pdgs[0]) == 15);

						if (is_dileptonic) {
							std::cerr << "Found dileptonic decay of boson type" << icn_pdg << " at evt_idx=" << m_nEvt << std::endl;
						} else {
							// check if hadronic
							short n_jets_from_icn = 0;

							for (const int& tj: jet_ids) {
								auto it = std::find(trueHadronicJetIndices.begin(), trueHadronicJetIndices.end(), tj);
								
								if (it != trueHadronicJetIndices.end()) {
									truejetpermICNs.push_back(std::distance(trueHadronicJetIndices.begin(), it));
									n_jets_from_icn += 1;
								}
							}
							
							if (n_jets_from_icn > 0) {
								//std::cerr << "Adding ICN PDG=" << icn_pdg << " to PermICNs at position " << (truejetpermICNs.size() / 2) << std::endl;
								if (n_jets_from_icn != 2 || truejetpermICNs.size() % 2 != 0) {
									//throw EVENT::Exception("Expected H/Z -> q + qbar decay");
									streamlog_out(ERROR) << "Expected H/Z -> q + qbar decay. Skipping event";

									// setting truejetpermICNs.size() to 0 will skip below loop
									truejetpermICNs = {};
								} else {
									m_trueDijetICNPDGs[truejetpermICNs.size() / 2 - 1] = icn_pdg;
								}
							}
						}
					}
				}
			}

			if (truejetpermICNs.size()) {
				if (truejetpermICNs.size() % 2 != 0)
					throw EVENT::Exception("Expected H/Z -> q + qbar decay");

				for (size_t i = 0; i < truejetpermICNs.size(); i++) {
					int index = trueHadronicJetIndices[truejetpermICNs[i]];
					const ReconstructedParticle* tjet = trueJet->jet(index);

					//m_trueJetMomenta[i] = v4(tjet);
					const double* v4_truejet = p4true(index);
					m_trueJetMomenta[i].SetPxPyPzE(v4_truejet[1], v4_truejet[2], v4_truejet[3], v4_truejet[0]);

					m_trueJetTypes[i] = type_jet(index);
					m_trueJetPDGs[i] = tjet->getParticleIDs()[0]->getPDG();
				}
			}
		}
	} else if ((
		process_name == "e2e2hh" || process_name == "e2e2qqh"
	) && (m_nAskedIsoLeps() == 2 && m_nAskedJets() == 4) ) {		
		/*
		THIS DOES NOT INCLUDE QUARK GLUON SPLITTINGS AND OTHER MORE COMPLICATED
		CASES. PROCEED WITH CAUTION WHEN USING TRUEJET (ABOVE) OR THIS APPROACH
		*/

		LCCollection *mcParticles = pLCEvent->getCollection( "MCParticlesSkimmed" );
		
		IntVec fsIndices;
		mcParticles->parameters().getIntVals("FINAL_STATE_PARTICLE_INDICES", fsIndices);

		// show Higgses first
		std::sort(fsIndices.begin(), fsIndices.end(), [mcParticles]( const int& mcp1_idx, const int& mcp2_idx ) {
			return abs( ((MCParticle*)mcParticles->getElementAt(mcp1_idx))->getPDG() ) == 25; // && trueJet->E_icn(icn1_idx) > trueJet->E_icn(icn2_idx); // need to have some sorting besides PDG=25
		});

		vector<MCParticle*> hadronicMCPs;
		vector<MCParticle*> leptonicMCPs;

		std::cerr << "fsParticle order: "; 
		for (int &i_part: fsIndices)
			std::cerr << ((MCParticle*)mcParticles->getElementAt(i_part))->getPDG() << " ";
		std::cerr << std::endl;

		for (size_t i = 0; i < fsIndices.size(); i++) {
			MCParticle* final_state_mcp = (MCParticle*)mcParticles->getElementAt(fsIndices[i]);
			int pdg = final_state_mcp->getPDG();
			m_trueJetTypes[i] = pdg;

			if (pdg == 25) {
				MCParticleVec Hdaughters = final_state_mcp->getDaughters();

				if (Hdaughters.size() == 2) {
					int decay1PDG = abs(Hdaughters[0]->getPDG());
					int decay2PDG = abs(Hdaughters[1]->getPDG());

					if (decay1PDG == decay2PDG) {
						if (decay1PDG <= 6) {
							//m_trueJetHiggsICNPairs.push_back(hadronicMCPs.size());
							hadronicMCPs.push_back(Hdaughters[0]);

							//m_trueJetHiggsICNPairs.push_back(hadronicMCPs.size());
							hadronicMCPs.push_back(Hdaughters[1]);
						} else if (decay1PDG >= 11 && decay1PDG <= 16) {
							//leptonicMCPs.push_back(Hdaughters[0]);
							//leptonicMCPs.push_back(Hdaughters[1]);
						}
					}
				}
			} else if (abs(pdg) <= 6) {
				hadronicMCPs.push_back(final_state_mcp);
			} else if (abs(pdg) >= 11 && abs(pdg) <= 16) {
				leptonicMCPs.push_back(final_state_mcp);
			} 
		}

		m_trueJetN = hadronicMCPs.size();
		m_trueLeptonN = leptonicMCPs.size();

		std::cerr << "Found nhadronicMCPs=" << m_trueJetN << " | nleptonicMCPs=" << m_trueLeptonN << std::endl;
		// populate four momenta and PDGs of jets and leptons
		if (m_trueJetN > m_nAskedJets() || m_trueLeptonN > m_nAskedIsoLeps())
			throw EVENT::Exception("Found more hadronic/leptonic MCPs than expected");
		
		
		for (unsigned short i = 0; i < m_trueJetN; i++) {
			m_trueJetMomenta[i] = v4(hadronicMCPs[i]);
			m_trueJetPDGs[i] = hadronicMCPs[i]->getPDG();

			if (i % 2 != 0) {
				std::cerr << "M(hadronic system " << ((i-1)/2 + 1) << ") = " << (m_trueJetMomenta[i] + m_trueJetMomenta[i - 1]).M() << std::endl;
			}
		}
		
		for (unsigned short i = 0; i < m_trueLeptonN; i++) {
			m_trueLeptonMomenta[i] = v4(leptonicMCPs[i]);
			m_trueLeptonPDGs[i] = leptonicMCPs[i]->getPDG();

			if (i % 2 != 0) {
				std::cerr << "M(leptonic system " << ((i-1)/2 + 1) << ") = " << (m_trueLeptonMomenta[i] + m_trueLeptonMomenta[i - 1]).M() << std::endl;
			}
		}

		/*
		
		float smallestSumCosAngle = getMatchingByAngularSpace(m_jets, hadronicMCPs, m_reco2TrueJetIndex, m_true2RecoJetIndex);

		// case in which number of hadronic jets == m_nJets
		m_trueRecoJetsMapped = smallestSumCosAngle < 99; */
	}

	delall2();
};

void EventObservablesLL::calculateSimpleZHHChi2() {
	// do it in the above loop instead
}
