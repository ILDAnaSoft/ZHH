#include "EventObservablesQQ.h"
#include <Math/Boost.h>
#include <cmath>

// errorCodes:
// -

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ") {
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

        ttree->Branch("bmax5", &m_bmax5, "bmax5/F");
        ttree->Branch("bmax6", &m_bmax6, "bmax6/F");
        ttree->Branch("btagz", &m_bTagZ, "btagz/F");

        zz_init(ttree);

        #define QQ_BRANCH(name) ttree->Branch(#name, &m_x.name, #name "/F")
        QQ_BRANCH(zhh6_chi2); QQ_BRANCH(zhh6_mz); QQ_BRANCH(zhh6_mh1); QQ_BRANCH(zhh6_mh2); QQ_BRANCH(zhh6_mhh);
        QQ_BRANCH(zhh6_recoil_hh); QQ_BRANCH(zhh6_recoil_z);
        QQ_BRANCH(zhh6_cosz); QQ_BRANCH(zhh6_coshh); QQ_BRANCH(zhh6_cos_hh); QQ_BRANCH(zhh6_cos_zjj);
        QQ_BRANCH(zhh6_cosh1star); QQ_BRANCH(zhh6_cosh2star);
        QQ_BRANCH(zzh6_chi2); QQ_BRANCH(zzh6_mh); QQ_BRANCH(zzh6_mz1); QQ_BRANCH(zzh6_mz2);
        QQ_BRANCH(ww4_chi2); QQ_BRANCH(ww4_mw1); QQ_BRANCH(ww4_mw2);
        QQ_BRANCH(sumBTags); QQ_BRANCH(btag4sum); QQ_BRANCH(btag4prod); QQ_BRANCH(ctagz);
        QQ_BRANCH(pzmiss); QQ_BRANCH(cosmiss);
        QQ_BRANCH(sphericity); QQ_BRANCH(aplanarity); QQ_BRANCH(fw2);
        QQ_BRANCH(cosjj_max); QQ_BRANCH(mjj_min);
        QQ_BRANCH(kfzhh_mhh); QQ_BRANCH(kfzhh_cosz); QQ_BRANCH(kfzhh_cos_hh);
        #undef QQ_BRANCH
        ttree->Branch("nbjets05", &m_nbjets05, "nbjets05/I");

        // ttbar
        ttree->Branch("tt_mw1", &m_tt_mw1, "tt_mw1/F");
        ttree->Branch("tt_mw2", &m_tt_mw2, "tt_mw2/F");
        ttree->Branch("tt_mt1", &m_tt_mt1, "tt_mt1/F");
        ttree->Branch("tt_mt2", &m_tt_mt2, "tt_mt2/F");
        ttree->Branch("tt_chi2", &m_tt_chi2, "tt_chi2/F");
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

    m_jet5_q = 0.;
    m_jet5_qdyn = 0.;

    m_jet6_q = 0.;
    m_jet6_qdyn = 0.;

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

    m_x = QQExtraFeatures{};
    m_nbjets05 = 0;
};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {
    setJetCharges();
    
    LCCollection *inputJetCollection = pLCEvent->getCollection( m_inputJetCollection ); // -> m_jets
    LCCollection *input4JetCollection = pLCEvent->getCollection( m_input4JetCollection ); // -> m_4jets
    
    if ( static_cast<int>(m_jets.size()) == m_nAskedJets() ) {
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

        // C. ADDITIONAL FEATURES (ZHH/ZZH/WW hypotheses, angles, flavor tags, event shape, kinfit results)
        calculateExtraFeatures(pLCEvent);

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

        #ifdef CALC_ME
        // assume remaining jets to be b-jets
        calculateMatrixElements(z1_decay_pdg, 5, v4_jet_from_z1, v4_jet_from_z2,
                                v4old(m_jets[2 + m_zhh_jet_matching[0]]), v4old(m_jets[2 + m_zhh_jet_matching[1]]),
                                v4old(m_jets[2 + m_zhh_jet_matching[2]]), v4old(m_jets[2 + m_zhh_jet_matching[3]]), true);
        #endif
    }
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

// ---------------------------------------------------------------------------------------------
// additional features; requires m_jets, m_4jets, m_bTagValues, m_cTagValues, m_bTagsSorted to be filled
// ---------------------------------------------------------------------------------------------
namespace {
    using LV = ROOT::Math::PxPyPzEVector;

    inline float cosAngle(const LV &a, const LV &b) {
        double pa = a.P(), pb = b.P();
        return (pa > 0 && pb > 0) ? (a.Vect().Dot(b.Vect()) / (pa * pb)) : 0.;
    }

    // |cos| of the angle between a daughter (in the rest frame of the parent) and the flight direction of the parent
    inline float helicityCos(const LV &parent, const LV &daughter) {
        if (parent.M2() <= 0 || parent.P() <= 0) return 0.;
        ROOT::Math::Boost to_rest(parent.BoostToCM());
        LV d = to_rest(daughter);
        return std::abs(cosAngle(d, parent));
    }

    // eigenvalues of a symmetric 3x3 matrix, sorted descending (analytic, Smith 1961)
    std::array<double, 3> eigenvalues3(const double a[3][3]) {
        double p1 = a[0][1]*a[0][1] + a[0][2]*a[0][2] + a[1][2]*a[1][2];
        double q = (a[0][0] + a[1][1] + a[2][2]) / 3.;
        if (p1 < 1e-18) {
            std::array<double, 3> r = { a[0][0], a[1][1], a[2][2] };
            std::sort(r.begin(), r.end(), std::greater<double>());
            return r;
        }
        double p2 = std::pow(a[0][0]-q, 2) + std::pow(a[1][1]-q, 2) + std::pow(a[2][2]-q, 2) + 2*p1;
        double p = std::sqrt(p2 / 6.);
        double B[3][3];
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) B[i][j] = (a[i][j] - (i == j ? q : 0.)) / p;
        double detB = B[0][0]*(B[1][1]*B[2][2] - B[1][2]*B[2][1])
                    - B[0][1]*(B[1][0]*B[2][2] - B[1][2]*B[2][0])
                    + B[0][2]*(B[1][0]*B[2][1] - B[1][1]*B[2][0]);
        double r = std::max(-1., std::min(1., detB / 2.));
        double phi = std::acos(r) / 3.;
        double e1 = q + 2*p*std::cos(phi);
        double e3 = q + 2*p*std::cos(phi + 2*M_PI/3.);
        return { e1, 3*q - e1 - e3, e3 };
    }
}

void EventObservablesQQ::calculateExtraFeatures(EVENT::LCEvent *pLCEvent) {
    QQExtraFeatures &x = m_x;
    std::vector<LV> jv = v4(m_jets);

    // A. ZHH hypothesis: (Z, H, H) from all 45 pairings; chi2 contains the Z mass term
    {
        std::vector<float> masses;
        std::vector<unsigned short> perm;
        std::tie(perm, masses, x.zhh6_chi2) = pairJetsByMass(jv, { 23, 25, 25 });

        LV z  = jv[perm[0]] + jv[perm[1]];
        LV h1 = jv[perm[2]] + jv[perm[3]];
        LV h2 = jv[perm[4]] + jv[perm[5]];

        // h1 is the Higgs candidate closer to 125 GeV
        bool swap = std::abs(h1.M() - kMassH) > std::abs(h2.M() - kMassH);
        if (swap) { std::swap(h1, h2); std::swap(perm[2], perm[4]); std::swap(perm[3], perm[5]); }

        LV hh = h1 + h2;

        x.zhh6_mz  = z.M();
        x.zhh6_mh1 = h1.M();
        x.zhh6_mh2 = h2.M();
        x.zhh6_mhh = hh.M();
        x.zhh6_recoil_hh = (m_ecms - hh).M();
        x.zhh6_recoil_z  = (m_ecms - z).M();

        x.zhh6_cosz  = z.P() > 0 ? z.Pz() / z.P() : 0.;
        x.zhh6_coshh = hh.P() > 0 ? hh.Pz() / hh.P() : 0.;
        x.zhh6_cos_hh  = cosAngle(h1, h2);
        x.zhh6_cos_zjj = cosAngle(jv[perm[0]], jv[perm[1]]);
        x.zhh6_cosh1star = helicityCos(h1, jv[perm[2]]);
        x.zhh6_cosh2star = helicityCos(h2, jv[perm[4]]);
    }

    // B. ZZH hypothesis: (H, Z, Z)
    {
        std::vector<float> masses;
        std::vector<unsigned short> perm;
        std::tie(perm, masses, x.zzh6_chi2) = pairJetsByMass(jv, { 25, 23, 23 });

        x.zzh6_mh = masses[0];
        x.zzh6_mz1 = std::max(masses[1], masses[2]);
        x.zzh6_mz2 = std::min(masses[1], masses[2]);
    }

    // C. WW/ZZ -> 4 quarks from the 4 jet clustering
    if (m_4jets.size() == 4) {
        FloatVec masses;
        std::vector<unsigned short> perm;
        std::tie(perm, masses, x.ww4_chi2) = pairJetsByMass(m_4jets, { 24, 24 });
        x.ww4_mw1 = std::max(masses[0], masses[1]);
        x.ww4_mw2 = std::min(masses[0], masses[1]);
    }

    // D. flavor tagging; m_bTagsSorted is sorted descending
    x.btag4prod = 1.;
    for (size_t i = 0; i < m_bTagsSorted.size(); i++) {
        float b = m_bTagsSorted[i].second;
        if (std::isnan(b)) continue;

        x.sumBTags += b;
        if (i < 4) { x.btag4sum += b; x.btag4prod *= b; }
        if (b > 0.5) m_nbjets05++;
    }
    x.ctagz = m_cTagValues[m_bTagsSorted[4].first] + m_cTagValues[m_bTagsSorted[5].first];

    // E. missing momentum
    x.pzmiss  = m_pmis.Pz();
    x.cosmiss = m_pmis.P() > 0 ? m_pmis.Pz() / m_pmis.P() : 0.;

    // F. jet topology: smallest opening angle (largest cosine) and smallest invariant mass of any jet pair
    x.cosjj_max = -1.;
    x.mjj_min = 1e9;
    for (size_t i = 0; i < jv.size(); i++)
        for (size_t j = i + 1; j < jv.size(); j++) {
            x.cosjj_max = std::max(x.cosjj_max, cosAngle(jv[i], jv[j]));
            x.mjj_min = std::min(x.mjj_min, (float)(jv[i] + jv[j]).M());
        }

    // G. event shape from the PFOs: sphericity, aplanarity, 2nd normalized Fox-Wolfram moment
    {
        LCCollection *pfos = pLCEvent->getCollection(m_inputPfoCollection);
        std::vector<std::array<double, 3>> p;
        for (int i = 0; i < pfos->getNumberOfElements(); i++) {
            const double *m = static_cast<ReconstructedParticle*>(pfos->getElementAt(i))->getMomentum();
            p.push_back({ m[0], m[1], m[2] });
        }

        double S[3][3] = {{0}};
        double psum2 = 0.;
        for (auto &v : p) {
            for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) S[a][b] += v[a] * v[b];
            psum2 += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
        }

        if (psum2 > 0) {
            for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) S[a][b] /= psum2;
            auto ev = eigenvalues3(S);
            x.sphericity = 1.5 * (ev[1] + ev[2]);
            x.aplanarity = 1.5 * ev[2];

            double h0 = 0., h2 = 0.;
            for (size_t i = 0; i < p.size(); i++) {
                double pi = std::sqrt(p[i][0]*p[i][0] + p[i][1]*p[i][1] + p[i][2]*p[i][2]);
                for (size_t j = 0; j < p.size(); j++) {
                    double pj = std::sqrt(p[j][0]*p[j][0] + p[j][1]*p[j][1] + p[j][2]*p[j][2]);
                    if (pi == 0 || pj == 0) continue;
                    double c = (p[i][0]*p[j][0] + p[i][1]*p[j][1] + p[i][2]*p[j][2]) / (pi * pj);
                    h0 += pi * pj;
                    h2 += pi * pj * 0.5 * (3*c*c - 1);
                }
            }
            x.fw2 = h0 > 0 ? h2 / h0 : 0.;
        }
    }

    // H. post-fit kinematics of the ZHH (H,H,Z) kinematic fit
    try {
        LCCollection *zhh = pLCEvent->getCollection(m_inputJetKinFitZHHCollection);

        std::vector<int> perm;
        zhh->parameters().getIntVals("permutation", perm);

        // jets are stored in the order of the input jets; perm[k] is the input index of the k-th fit object
        if (zhh->getNumberOfElements() >= 6 && perm.size() >= 6) {
            std::vector<LV> f;
            for (int k = 0; k < 6; k++)
                f.push_back(v4((ReconstructedParticle*)zhh->getElementAt(perm[k])));

            LV h1 = f[0] + f[1], h2 = f[2] + f[3], z = f[4] + f[5];

            x.kfzhh_mhh = (h1 + h2).M();
            x.kfzhh_cosz = z.P() > 0 ? z.Pz() / z.P() : 0.;
            x.kfzhh_cos_hh = cosAngle(h1, h2);
        }
    } catch (DataNotAvailableException &e) {
        streamlog_out(MESSAGE) << "ZHH kinfit jet collection not found; kfzhh_* will be 0" << std::endl;
    }
}
