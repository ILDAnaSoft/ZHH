#include "EventObservablesVV.h"

// errorCodes:
// -

EventObservablesVV aEventObservablesVV;

EventObservablesVV::EventObservablesVV(): EventObservablesBase("EventObservablesVV") {
  _description = "EventObservablesVV writes relevant observables to root-file " ;
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "5JetCollectionName" ,
			  "Name of the Jet collection"  ,
			  m_input5JetCollection ,
			  std::string("Refined5Jets")
			  );
  
  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "6JetCollectionName" ,
			  "Name of the Jet collection"  ,
			  m_input6JetCollection ,
			  std::string("Refined6Jets")
			  );
}

void EventObservablesVV::prepareChannelTree() {
  TTree* ttree = getTTree();
  
  m_fit4C_masses = std::vector<float>(m_nAskedJets());
  
  if (m_write_ttree) {
    ttree->Branch("ptjmax6", &m_ptjmax6, "ptjmax6/F");
    ttree->Branch("pjmax6", &m_pjmax6, "pjmax6/F");
    
    // ttbar 5j
    ttree->Branch("npfosmin5j", &m_npfosmin5j, "npfosmin5j/I");
    ttree->Branch("npfosmax5j", &m_npfosmax5j, "npfosmax5j/I");
    ttree->Branch("ptjmax5", &m_ptjmax5, "ptjmax5/F");
    ttree->Branch("pjmax5", &m_pjmax5, "pjmax5/F");
    
    ttree->Branch("yminus5j", &m_yMinus5j, "yminus5j/F");
    ttree->Branch("yplus5j", &m_yPlus5j, "yplus5j/F");
    
    ttree->Branch("massW1tt5j", &m_massW1tt5j, "massW1tt5j/F");
    ttree->Branch("massW2tt5j", &m_massW2tt5j, "massW2tt5j/F");
    ttree->Branch("massT1tt5j", &m_massT1tt5j, "massT1tt5j/F");
    ttree->Branch("massT2tt5j", &m_massT2tt5j, "massT2tt5j/F");
    
    // ttbar 4j
    ttree->Branch("massWtt4j", &m_massWtt4j, "massWtt4j/F");
    ttree->Branch("massT1tt4j", &m_massT1tt4j, "massT1tt4j/F");
    ttree->Branch("massT2tt4j", &m_massT2tt4j, "massT2tt4j/F");
    
    // ZZ
    zz_init(ttree);
    
    // MC truth information
    ttree->Branch("massDiNeutrino", &m_diNeutrinoMass, "massDiNeutrino/F");
    
    // 4 jets
    ttree->Branch("mbmax12", &m_mbmax12, "mbmax12/F");
    ttree->Branch("mbmax34", &m_mbmax34, "mbmax34/F");
  }
};

void EventObservablesVV::clearChannelValues() {
  m_5jets.clear();
  //m_6jets.clear();
  
  m_ptjmax6 = 0.;
  m_pjmax6 = 0.;
  
  // ttbar 5j
  m_npfosmin5j = 0;
  m_npfosmax5j = 0;
  m_ptjmax5 = 0.;
  m_pjmax5 = 0.;
  
  m_yMinus5j = 0.;
  m_yPlus5j = 0.;
  
  m_massW1tt5j = 0.;
  m_massW2tt5j = 0.;
  m_massT1tt5j = 0.;
  m_massT2tt5j = 0.;
  
  // ttbar 4j
  m_massWtt4j = 0;
  m_massT1tt4j = 0;
  m_massT2tt4j = 0;
  
  // ZZ
  zz_clear();
  
  m_diNeutrinoMass = 0.;
  
  m_jets4cKinFit_4v.clear();
  m_fit4C_masses.clear();
  
  // 4 jets
  m_mbmax12 = 0.;
  m_mbmax34 = 0.;
};

void EventObservablesVV::updateChannelValues(EVENT::LCEvent *pLCEvent) {
  setJetCharges();
  
  LCCollection *input5JetCollection = pLCEvent->getCollection( m_input5JetCollection );
  LCCollection *input6JetCollection = pLCEvent->getCollection( m_input6JetCollection );
  
  // VANILLA JET MATCHING
  std::vector<float> zhh_masses;
  std::vector<ROOT::Math::PxPyPzEVector> jet_v4 = v4(m_jets);
  
  std::tie(m_zhh_jet_matching, zhh_masses, m_zhh_chi2) = pairJetsByMass(jet_v4, { 25, 25 });
  
  if (abs(zhh_masses[0]-125.) < abs(zhh_masses[1]-125.)) {
    m_zhh_mh1 = zhh_masses[0];
    m_zhh_mh2 = zhh_masses[1];
  } else {
    m_zhh_mh1 = zhh_masses[1];
    m_zhh_mh2 = zhh_masses[0];
  }
  m_zhh_mhh = (jet_v4[0] + jet_v4[1] + jet_v4[2] + jet_v4[3]).M();
  
  std::vector<ROOT::Math::PxPyPzEVector> dijets = {
    jet_v4[m_zhh_jet_matching[0]] + jet_v4[m_zhh_jet_matching[1]],
    jet_v4[m_zhh_jet_matching[2]] + jet_v4[m_zhh_jet_matching[3]]
  };
  
  for (ROOT::Math::PxPyPzEVector dijet: dijets) {
    if (dijet.P() > m_zhh_p1st) {
      m_zhh_p1st = dijet.P();
      m_zhh_cosTh1st = cos(dijet.Theta());
    }
  }
  
  for (int i = 0; i < inputJKF_solveNuCollection->getNumberOfElements(); i++) {
    ReconstructedParticle* jet = (ReconstructedParticle*) inputJKF_solveNuCollection->getElementAt(i);
    m_jets4cKinFit_4v.push_back(v4(jet));
  }
  
  m_JMK_best = (m_fitchi2_ZHH <= m_fitchi2_ZZH ? m_JMK_ZHH : m_JMK_ZZH);
  
  streamlog_out(MESSAGE) << m_leps4cKinFit_4v.size() << " vs " << m_nAskedIsoLeps() << endl;
  streamlog_out(MESSAGE) << m_JMK_best.size() << " vs " << m_nAskedJets() << endl;
  
  if (m_leps4cKinFit_4v.size() == m_nAskedIsoLeps() && m_JMK_best.size() >= m_nAskedJets()) {
    m_fit4C_masses.push_back((m_jets4cKinFit_4v[m_JMK_best[0]]+m_jets4cKinFit_4v[m_JMK_best[1]]).M());
    m_fit4C_masses.push_back((m_jets4cKinFit_4v[m_JMK_best[2]]+m_jets4cKinFit_4v[m_JMK_best[3]]).M());
    if (abs(m_fit4C_masses[0]-125.) < abs(m_fit4C_masses[1]-125.)) {
      m_fit4C_mh1 = m_fit4C_masses[0];
      m_fit4C_mh2 = m_fit4C_masses[1];
    } else {
      m_fit4C_mh1 = m_fit4C_masses[1];
      m_fit4C_mh2 = m_fit4C_masses[0];
    }
    ROOT::Math::PxPyPzEVector Total_4v;
    for (ROOT::Math::PxPyPzEVector jet_4v : m_jets4cKinFit_4v) {
      Total_4v += jet_4v;
    }
    m_fit4C_mhh = Total_4v.M();
  } else {
    streamlog_out(MESSAGE) << "Kinfit not used" << endl;
    m_fit4C_mh1 = m_zhh_mh1;
    m_fit4C_mh2 = m_zhh_mh2;
    m_fit4C_mhh = m_zhh_mhh;      
  }      
  
  // TREAT 5 JET COLLECTION
  
  // assume jet given by jet5IDxPFOsMin with least No of PFOs is the tau jet
  int jet5IDxPFOsMin;
  std::tie(m_npfosmin5j, m_npfosmax5j, jet5IDxPFOsMin) = nPFOsMinMax(input5JetCollection);
  vector<float> btagValues_tvbbqq;
  PIDHandler jetPIDh_tvbbqq(input5JetCollection);
  int _FTAlgoID_tvbbqq = jetPIDh_tvbbqq.getAlgorithmID(m_JetTaggingPIDAlgorithm);
  
  int BTagID_tvbbqq = jetPIDh_tvbbqq.getParameterIndex(_FTAlgoID_tvbbqq, m_JetTaggingPIDParameterB);
  int BbarTagID_tvbbqq = jetPIDh_tvbbqq.getParameterIndex(_FTAlgoID_tvbbqq, m_JetTaggingPIDParameterBbar);
  // reconstruct the semileptonic ttbar system for the 5-jet system
  for (int i=0; i<input5JetCollection->getNumberOfElements(); i++) {
    streamlog_out(MESSAGE) << "jet "<< i << ":" << endl; 
    ReconstructedParticle* jet = dynamic_cast<ReconstructedParticle*>(input5JetCollection->getElementAt(i));
    //streamlog_out(MESSAGE) << "ladida" << endl;
    if (i == jet5IDxPFOsMin) {
      streamlog_out(MESSAGE) << "Number of PFOs" << jet->getParticles().size() << "<---- tau jet" << endl;
      btagValues_tvbbqq.push_back(-1.); //Artificially set b-tag value of tau-jet to be the lowest
      continue;
    }
    streamlog_out(MESSAGE) << "Number of PFOs" << jet->getParticles().size() << endl;
    const ParticleIDImpl& FTImpl_tvbbqq = dynamic_cast<const ParticleIDImpl&>(jetPIDh_tvbbqq.getParticleID(jet, _FTAlgoID_tvbbqq));
    const FloatVec& FTPara_tvbbqq = FTImpl_tvbbqq.getParameters();
    float bTagValue = FTPara_tvbbqq[BTagID_tvbbqq] + FTPara_tvbbqq[BbarTagID_tvbbqq];
    streamlog_out(MESSAGE) << "btag-value" << bTagValue << endl;
    btagValues_tvbbqq.push_back(bTagValue);
  }
  
  std::vector<std::pair<int, float>> sortedBTags_tvbbqq = sortedTagging(btagValues_tvbbqq); //Note: b-tag value of tau-jet artificially lowest
  // order is with decreasing b-tag    
  for (size_t i=0; i < sortedBTags_tvbbqq.size(); ++i) {
    ReconstructedParticle* jet = (ReconstructedParticle*) input5JetCollection->getElementAt(i);
      
    m_pjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
    m_ptjmax5 = std::max(m_ptjmax5, (float)v4(jet).Pt());
    
    m_5jets.push_back(jet);
  }
  
  ReconstructedParticle *jetbmax1_tvbbqq = m_5jets[sortedBTags_tvbbqq[0].first]; //b1
  ReconstructedParticle *jetbmax2_tvbbqq = m_5jets[sortedBTags_tvbbqq[1].first]; //b2
  ReconstructedParticle *jetbmax3_tvbbqq = m_5jets[sortedBTags_tvbbqq[2].first]; //q1 of W->qq
  ReconstructedParticle *jetbmax4_tvbbqq = m_5jets[sortedBTags_tvbbqq[3].first]; //q2 of W->qq

  if(sortedBTags_tvbbqq[4].first != jet5IDxPFOsMin) streamlog_out(MESSAGE) << "Issue with identifying tau-jet" << endl;
  //mass of W -> tau nu system
  ReconstructedParticle* tau_jet = m_5jets[jet5IDxPFOsMin];
  ROOT::Math::PxPyPzEVector tau_v4 = v4(tau_jet);
  ROOT::Math::PxPyPzEVector W1_v4 = tau_v4+m_pmis;
  m_massW1tt5j = W1_v4.M();
  //mass of W -> qq system 
  m_massW2tt5j = inv_mass(jetbmax3_tvbbqq, jetbmax4_tvbbqq);
  //mass of t-> bW -> bqq for the two combinations with b
  m_massT1tt5j = inv_mass(jetbmax1_tvbbqq, jetbmax3_tvbbqq, jetbmax4_tvbbqq);
  m_massT2tt5j = inv_mass(jetbmax2_tvbbqq, jetbmax3_tvbbqq, jetbmax4_tvbbqq);
  
  // get yMinus and yPlus
  PIDHandler jet5PIDh(input5JetCollection);
  int algo_y = jet5PIDh.getAlgorithmID("yth");
  const ParticleID & ythID = jet5PIDh.getParticleID(m_5jets[0], algo_y); // same arguments for all jets
  
  FloatVec params_y = ythID.getParameters();
  m_yMinus5j = params_y[jet5PIDh.getParameterIndex(algo_y, "y45")];
  m_yPlus5j = params_y[jet5PIDh.getParameterIndex(algo_y, "y56")];

  // TREAT 6 JET COLLECTION
  for (int i=0; i < input6JetCollection->getNumberOfElements(); ++i) {
    ReconstructedParticle* jet = (ReconstructedParticle*) input6JetCollection->getElementAt(i);
    
    m_pjmax6 = std::max(m_pjmax6, (float)v4(jet).P());
    m_ptjmax6 = std::max(m_ptjmax6, (float)v4(jet).Pt());
    //m_6jets.push_back(jet);
  }
  
  // TREAT 4 JET COLLECTION
  // TTBAR: FORM W and TOPS
  ReconstructedParticle *jetbmax1 = m_jets[m_bTagsSorted[0].first];
  ReconstructedParticle *jetbmax2 = m_jets[m_bTagsSorted[1].first];
  ReconstructedParticle *jetbmax3 = m_jets[m_bTagsSorted[2].first];
  ReconstructedParticle *jetbmax4 = m_jets[m_bTagsSorted[3].first];
  
  TVector3 pjbmax1 = TVector3(jetbmax1->getMomentum());
  TVector3 pjbmax2 = TVector3(jetbmax2->getMomentum());
  
  m_massWtt4j = inv_mass(jetbmax3, jetbmax4);
  m_massT1tt4j = inv_mass(jetbmax1, jetbmax3, jetbmax4);
  m_massT2tt4j = inv_mass(jetbmax2, jetbmax3, jetbmax4);
  
  // ZZ: CHECK BY CHI2
  zz_update(m_jets);
  
  // process MC truth data
  const std::string process_name = pLCEvent->getParameters().getStringVal("processName");
  
  if (std::find(di_neutrino_processes.begin(), di_neutrino_processes.end(), process_name) != di_neutrino_processes.end()) {
    LCCollection *mcParticles = pLCEvent->getCollection( "MCParticlesSkimmed" );
    
    IntVec fsIndices;
    mcParticles->parameters().getIntVals("FINAL_STATE_PARTICLE_INDICES", fsIndices);
    
    MCParticle* neutrino1 = (MCParticle*)mcParticles->getElementAt(fsIndices[0]);
    MCParticle* neutrino2 = (MCParticle*)mcParticles->getElementAt(fsIndices[1]);
    
    assert(neutrino1->getPDG() >= 11 && neutrino1->getPDG() <= 16);
    assert(neutrino2->getPDG() >= 11 && neutrino2->getPDG() <= 16);
    
    m_diNeutrinoMass = (v4(neutrino1) + v4(neutrino2)).M();
  }
  
  // mb12 and mb34
  m_mbmax12 = (v4(jetbmax1) + v4(jetbmax2)).M();
  m_mbmax34 = (v4(jetbmax3) + v4(jetbmax4)).M();
};

void EventObservablesVV::calculateSimpleZHHChi2() {
  
}
