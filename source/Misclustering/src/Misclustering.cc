#include "Misclustering.h"
#include <stdlib.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include "TH1F.h"
#include "TH2F.h"
#include "TF1.h"
#include "TPaveStats.h"
#include <Math/Vector4D.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#ifdef MARLIN_USE_AIDA
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
//#include <AIDA/IHistogram1D.h>
#endif // MARLIN_USE_AIDA


using namespace lcio ;
using namespace marlin ;
using namespace std ;

template<class T>
double inv_mass(T* p1, T* p2){
  double e = p1->getEnergy()+p2->getEnergy() ;
  double px = p1->getMomentum()[0]+p2->getMomentum()[0];
  double py = p1->getMomentum()[1]+p2->getMomentum()[1];
  double pz = p1->getMomentum()[2]+p2->getMomentum()[2];
  return( sqrt( e*e - px*px - py*py - pz*pz  ) );
}

Misclustering aMisclustering ;

Misclustering::Misclustering() : Processor("Misclustering"),
				       m_nRun(0),
				       m_nEvt(0),
				       m_nRunSum(0),
				       m_nEvtSum(0),
				       m_nTrueJets(0),
				       m_nRecoJets(0)

{

  // modify processor description
  _description = "Misclustering does whatever it does ..." ;


  // register steering parameters: name, description, class-variable, default value


  // Inputs: MC-particles, Reco-particles, the link between the two

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoJetCollection" ,
			   "Name of the input Reconstructed Jet collection"  ,
			   m_recoJetCollectionName ,
			   string("Durham_nJets")
			   );

  registerOutputCollection(	LCIO::RECONSTRUCTEDPARTICLE,
					"outputJetCollection",
					"Name of output fitted jet collection",
					m_outputJetCollection,
					std::string("Durham_4JetsKinFit")
				);

  registerInputCollection( LCIO::MCPARTICLE,
			   "MCParticleCollection" ,
			   "Name of the MCParticle collection"  ,
			   _MCParticleColllectionName ,
			   string("MCParticlesSkimmed")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "RecoParticleCollection" ,
			   "Name of the ReconstructedParticles input collection"  ,
			   _recoParticleCollectionName ,
			   string("PandoraPFOs")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "RecoMCTruthLink",
			   "Name of the RecoMCTruthLink input collection"  ,
			   _recoMCTruthLink,
			   string("RecoMCTruthLink")
			   );

  // Inputs: True jets (as a recoparticle, will be the sum of the _reconstructed particles_
  // created by the true particles in each true jet, in the RecoMCTruthLink sense.
  // link jet-to-reco particles, link jet-to-MC-particles.

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "TrueJets" ,
			   "Name of the TrueJetCollection input collection",
			   _trueJetCollectionName ,
			   string("TrueJets")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "FinalColourNeutrals" ,
			   "Name of the FinalColourNeutralCollection input collection"  ,
			   _finalColourNeutralCollectionName ,
			   string("FinalColourNeutrals")
			   );

  registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
			   "InitialColourNeutrals" ,
			   "Name of the InitialColourNeutralCollection input collection"  ,
			   _initialColourNeutralCollectionName ,
			   string("InitialColourNeutrals")
			   );


  registerInputCollection( LCIO::LCRELATION,
			   "TrueJetPFOLink" ,
			   "Name of the TrueJetPFOLink input collection"  ,
			   _trueJetPFOLink,
			   string("TrueJetPFOLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "TrueJetMCParticleLink" ,
			   "Name of the TrueJetMCParticleLink input collection"  ,
			   _trueJetMCParticleLink,
			   string("TrueJetMCParticleLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "FinalElementonLink rueJetMCParticleLink" ,
			   "Name of the  FinalElementonLink input collection",
			   _finalElementonLink,
			   string("FinalElementonLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			   "InitialElementonLink" ,
			   "Name of the  InitialElementonLink input collection"  ,
			   _initialElementonLink,
			   string("InitialElementonLink")
			   );

  registerInputCollection( LCIO::LCRELATION,
			 "FinalColourNeutralLink" ,
			 "Name of the  FinalColourNeutralLink input collection"  ,
			 _finalColourNeutralLink,
			 string("FinalColourNeutralLink")
			 );

 registerInputCollection( LCIO::LCRELATION,
			  "InitialColourNeutralLink" ,
			  "Name of the  InitialColourNeutralLink input collection"  ,
			  _initialColourNeutralLink,
			  string("InitialColourNeutralLink")
			  );

 registerProcessorParameter("matchMethod",
			    "0 = by angular space, 1 = by leading particle",
			    m_matchMethod,
			    int(0)
			    );

 registerProcessorParameter("outputFilename",
			    "name of output root file",
			    m_outputFile,
			    std::string("")
			    );
}


void Misclustering::init()
{
  streamlog_out(DEBUG6) << "   init called  " << endl ;

  m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
  m_pTTree = new TTree("eventTree","eventTree");
  m_pTTree->SetDirectory(m_pTFile);

  m_pTTree->Branch("run", &m_nRun, "run/I");
  m_pTTree->Branch("event", &m_nEvt, "event/I");
  m_pTTree->Branch("nrecodijetPFOs", &m_nrecodijetPFOs);
  m_pTTree->Branch("ntruedijetPFOs", &m_ntruedijetPFOs);
  m_pTTree->Branch("overlap", &m_overlap);
  m_pTTree->Branch("recodijetmass", &m_recodijetmass);
  m_pTTree->Branch("truedijetmass", &m_truedijetmass);
  m_pTTree->Branch("energyfrac_reco", &m_energyfrac_reco);
  m_pTTree->Branch("energyfrac_true", &m_energyfrac_true);
  m_pTTree->Branch("nrecodijetPFOs_ICNs", &m_nrecodijetPFOs_ICNs);
  m_pTTree->Branch("ntruedijetPFOs_ICNs", &m_ntruedijetPFOs_ICNs);
  m_pTTree->Branch("overlap_ICNs", &m_overlap_ICNs);
  m_pTTree->Branch("recodijetmass_ICNs", &m_recodijetmass_ICNs);
  m_pTTree->Branch("truedijetmass_ICNs", &m_truedijetmass_ICNs);
  m_pTTree->Branch("energyfrac_reco_ICNs", &m_energyfrac_reco_ICNs);
  m_pTTree->Branch("energyfrac_true_ICNs", &m_energyfrac_true_ICNs);
  m_pTTree->Branch("y_45", &m_y_45);
  m_pTTree->Branch("y_34", &m_y_34);
  m_pTTree->Branch("sumofangles", &m_sumofangles);
  m_pTTree->Branch("recodijet_energy", &m_recodijet_energy);
  m_pTTree->Branch("recodijet_theta", &m_recodijet_theta);
  m_pTTree->Branch("recodijet_phi", &m_recodijet_phi);
  m_pTTree->Branch("recodijet_energy_ICNs", &m_recodijet_energy_ICNs);
  m_pTTree->Branch("recodijet_theta_ICNs", &m_recodijet_theta_ICNs);
  m_pTTree->Branch("recodijet_phi_ICNs", &m_recodijet_phi_ICNs);
  m_pTTree->Branch("regionXX", &m_regionXX);
  m_pTTree->Branch("regionXX_ICNs", &m_regionXX_ICNs);
  streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void Misclustering::Clear()
{
  streamlog_out(DEBUG) << "   Clear called  " << endl;
  m_nTrueJets = 0;
  m_nRecoJets = 0;
  m_nRun = 0;
  m_nEvt = 0;
  m_nrecodijetPFOs.clear();
  m_ntruedijetPFOs.clear();
  m_overlap.clear();
  m_recodijetmass.clear();
  m_truedijetmass.clear();
  m_energyfrac_reco.clear();
  m_energyfrac_true.clear();
  m_nrecodijetPFOs_ICNs.clear();
  m_ntruedijetPFOs_ICNs.clear();
  m_overlap_ICNs.clear();
  m_recodijetmass_ICNs.clear();
  m_truedijetmass_ICNs.clear();
  m_energyfrac_reco_ICNs.clear();
  m_energyfrac_true_ICNs.clear();
  m_y_45.clear();
  m_y_34.clear();
  m_recodijet_energy.clear();
  m_recodijet_theta.clear();
  m_recodijet_phi.clear();
  m_recodijet_energy_ICNs.clear();
  m_recodijet_theta_ICNs.clear();
  m_recodijet_phi_ICNs.clear();
  m_regionXX.clear();
  m_regionXX_ICNs.clear();
}

void Misclustering::processRunHeader()
{
  m_nRun++ ;
}


void Misclustering::processEvent( LCEvent* pLCEvent)
{
  this->Clear();
  LCCollection *recoJetCol{};
  //LCCollection *trueJetCol{};
  m_nRun = pLCEvent->getRunNumber();
  m_nEvt = pLCEvent->getEventNumber();
  string trueJetType[6]{ "hadronic (string)" , "leptonic" , "hadronic(cluster)" , "ISR" , "overlay" , "M.E. photon" };
  string icnType[6]{ "quark pair" , "lepton pair" , "quark pair" , "ISR" , "???" , "M.E. photon" };
  streamlog_out(MESSAGE) << "" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////Processing event: " << m_nEvt << "////////////////////" << endl;
  streamlog_out(MESSAGE) << "////////////////////////////////////////////////////////////////////////////" << endl;

  try {
    recoJetCol= pLCEvent->getCollection( m_recoJetCollectionName );
    TrueJet_Parser* trueJet= this;
    trueJet->getall(pLCEvent);
    
    m_nRecoJets = recoJetCol->getNumberOfElements();
    streamlog_out(DEBUG3) << "Number of Reconstructed Jets: " << m_nRecoJets << endl;
    
    int njets = trueJet->njets();
    streamlog_out(DEBUG3) << "Number of True Jets: " << njets << endl;
    for (int i_jet = 0 ; i_jet < njets ; i_jet++ ) {
      if ( type_jet( i_jet ) == 1 ) {
	++m_nTrueJets;
      }
    }
    streamlog_out(DEBUG3) << "Number of True Hadronic Jets(type = 1): " << m_nTrueJets << endl;
    if ( m_nRecoJets == m_nTrueJets ) {
      //Vector of Durham jets
      vector<ReconstructedParticle*> recoJets; 
      for (unsigned int i=0; i<m_nRecoJets; ++i) {
	recoJets.push_back((ReconstructedParticle*) recoJetCol->getElementAt(i));
      }
      const EVENT::LCParameters& params = recoJetCol->getParameters() ;
      m_y_45.push_back(params.getFloatVal("y_{n,n+1}"));
      m_y_34.push_back(params.getFloatVal("y_{n-1,n}"));
      //Vector of matched indices {i_dj, i_tj}
      vector<pair<int,int>> reco2truejetindex;
      bool foundTrueJets = true;
      //Do matching 
      if (m_matchMethod == 0) {
	float SmallestSumCosAngle = getMatchingByAngularSpace(pLCEvent, recoJets, reco2truejetindex);
	m_sumofangles.push_back(SmallestSumCosAngle);
      } 
      if (m_matchMethod == 1) {
	foundTrueJets = getMatchingByLeadingParticle(pLCEvent, recoJets, reco2truejetindex);
      }
      if (foundTrueJets) {
	//Pairing of Durham jets into dijets
	vector<vector<int>> perms = {
	  {0, 1, 2, 3},
	  {0, 2, 1, 3},
	  {0, 3, 1, 2}
	};

	float chi2min = 99999. ;
	float higgs1 = 0. ;
	float higgs2 = 0. ;
	vector<int> recojetpermChi2;
	for (auto perm : perms) {
	  float m1 = inv_mass(recoJets.at(perm[0]),recoJets.at(perm[1]));
	  float m2 = inv_mass(recoJets.at(perm[2]),recoJets.at(perm[3]));
	  float chi2 = (m1-125)*(m1-125)+(m2-125)*(m2-125);
	  if (chi2 < chi2min) {
	    chi2min = chi2;
	    higgs1 = m1;
	    higgs2 = m2;
	    recojetpermChi2 = perm;
	  }
	}
	streamlog_out(DEBUG3) << "Matching of reco jet to true jet: { ";
	for (auto pair : reco2truejetindex) streamlog_out(DEBUG3) << "[" << pair.first << "," << pair.second << "] ";
	streamlog_out(DEBUG3) << "}" << endl;
	
	vector<int> truejetpermChi2;
	for (int i=0; i<4; i++) {
	  int ijet = recojetpermChi2[i];
          streamlog_out(DEBUG3) << "ijet = "<< ijet << endl;
          auto it = std::find_if( reco2truejetindex.begin(), reco2truejetindex.end(),
                                  [ijet](const pair<int, int>& element){ return element.first == ijet;} );
	  auto pair = *it;
	  truejetpermChi2.push_back(pair.second);
	}
	streamlog_out(DEBUG3) << "Pairing of reco dijets from chi2: { ";
	for (auto idx : recojetpermChi2) streamlog_out(DEBUG3) << idx << " ";
	streamlog_out(DEBUG3) << "}" << endl;
	streamlog_out(DEBUG3) << "Pairing of true dijets from chi2: { ";
	for (auto idx : truejetpermChi2) streamlog_out(DEBUG3) << idx << " ";
	streamlog_out(DEBUG3) << "}" << endl;

	//Pairing of di-Truejet from initial colour neutrals
	vector<int> truejetpermICNs;
	int nicn = trueJet->nicn();
	streamlog_out(DEBUG3) << "number of icns = " << nicn << endl;
	for (int i_icn = 0 ; i_icn < nicn ; i_icn++ ) {
	  auto siblings = jets_of_initial_cn(i_icn);
	  streamlog_out(DEBUG3) << "TrueJet pair (parent = " << pdg_icn_parent(i_icn) << "): ";
	  for (auto tj : siblings) streamlog_out(DEBUG3) << tj << " ";
	  streamlog_out(DEBUG3) << endl;
	  if (pdg_icn_parent(i_icn) == 25) {
	    for (auto tj : siblings) {
	      if (type_jet(tj) == 1) {
		truejetpermICNs.push_back(tj);
	      }
	    }
	  }
	}
	vector<int> recojetpermICNs;
        for (int i=0; i<4; i++) {
          int ijet = truejetpermICNs[i];
          streamlog_out(DEBUG3) << "ijet = "<< ijet << endl;
          auto it = std::find_if( reco2truejetindex.begin(), reco2truejetindex.end(),
                                  [ijet](const pair<int, int>& element){ return element.second == ijet;} );
	  auto pair = *it;
          recojetpermICNs.push_back(pair.first);
        }
        streamlog_out(DEBUG3) << "Pairing of true dijets from ICNs: { ";
        for (auto idx : truejetpermICNs) streamlog_out(DEBUG3) << idx << " ";
        streamlog_out(DEBUG3) << "}" << endl;
        streamlog_out(DEBUG3) << "Pairing of reco dijets from ICNs: { ";
        for (auto idx : recojetpermICNs) streamlog_out(DEBUG3) << idx << " ";
        streamlog_out(DEBUG3) << "}" << endl;

	//const IntVec& jets_of_initial_cn( int iicn ); 
	// the list of the jets initial colour-neutral iicn gives rise to
	//int nicn() { return icncol->getNumberOfElements(); };
	// Number of initial colour neutrals


	//For each dijet and corresponding di-Truejet, get list of PFOs and find intersection and calculate higgs masses
	vector<const char*> region;
	vector<const char*> region_ICNs;
	for (int i =0; i<2; i++){
	  vector<ReconstructedParticle*> recodijetPFOs;
	  vector<ReconstructedParticle*> truedijetPFOs;
	  ROOT::Math::PxPyPzEVector recodijet_v4(0,0,0,0);
	  vector<ReconstructedParticle*> recodijetPFOs_ICNs;
	  vector<ReconstructedParticle*> truedijetPFOs_ICNs;
	  ROOT::Math::PxPyPzEVector recodijet_v4_ICNs(0,0,0,0);
	  for (int j =0; j<2; j++){
	    //Chi2 pairing  
	    int i_recojetChi2 = recojetpermChi2[i*2+j];
	    int i_truejetChi2 = truejetpermChi2[i*2+j];
	    auto jet = recoJets.at(i_recojetChi2);
	    vector<ReconstructedParticle*> recoparticlevec = jet->getParticles();
	    vector<ReconstructedParticle*> trueparticlevec = seen_partics(i_truejetChi2);
	    recodijetPFOs.insert(recodijetPFOs.end(), recoparticlevec.begin(), recoparticlevec.end());
	    truedijetPFOs.insert(truedijetPFOs.end(), trueparticlevec.begin(), trueparticlevec.end());
	    ROOT::Math::PxPyPzEVector recojet_v4(jet->getMomentum()[0],jet->getMomentum()[1],jet->getMomentum()[2],jet->getEnergy());
	    recodijet_v4+=recojet_v4;
	    //ICNs pairing  
	    int i_truejetICNs = truejetpermICNs[i*2+j];
	    int i_recojetICNs = recojetpermICNs[i*2+j];
	    auto jet_ICNs = recoJets.at(i_recojetICNs);
	    vector<ReconstructedParticle*> recoparticlevec_ICNs = jet_ICNs->getParticles();
	    vector<ReconstructedParticle*> trueparticlevec_ICNs = seen_partics(i_truejetICNs);
	    recodijetPFOs_ICNs.insert(recodijetPFOs_ICNs.end(), recoparticlevec_ICNs.begin(), recoparticlevec_ICNs.end());
	    truedijetPFOs_ICNs.insert(truedijetPFOs_ICNs.end(), trueparticlevec_ICNs.begin(), trueparticlevec_ICNs.end());
	    ROOT::Math::PxPyPzEVector recojet_v4_ICNs(jet_ICNs->getMomentum()[0],jet_ICNs->getMomentum()[1],jet_ICNs->getMomentum()[2],jet_ICNs->getEnergy());
	    recodijet_v4_ICNs+=recojet_v4_ICNs;
	  }
	  //Chi2 pairing
	  vector<ReconstructedParticle*> dijet_intersection;
	  sort(recodijetPFOs.begin(), recodijetPFOs.end());
	  sort(truedijetPFOs.begin(), truedijetPFOs.end());
	  set_intersection(recodijetPFOs.begin(), recodijetPFOs.end(), truedijetPFOs.begin(), truedijetPFOs.end(), back_inserter(dijet_intersection));
	  m_nrecodijetPFOs.push_back(recodijetPFOs.size());
	  m_ntruedijetPFOs.push_back(truedijetPFOs.size());
	  m_overlap.push_back(float(dijet_intersection.size())/float(recodijetPFOs.size()));
	  m_recodijetmass.push_back(inv_mass(recoJets.at(recojetpermChi2[i*2]),recoJets.at(recojetpermChi2[i*2+1])));
	  m_truedijetmass.push_back(inv_mass(jet(truejetpermChi2[i*2]),jet(truejetpermChi2[i*2+1])));
	  double recodijetenergy = 0;
	  double truedijetenergy = 0;
	  double intersectionenergy = 0;
	  for (auto p : dijet_intersection) intersectionenergy += p->getEnergy();
	  for (auto p : recodijetPFOs) recodijetenergy += p->getEnergy();
	  for (auto p : truedijetPFOs) truedijetenergy += p->getEnergy();
	  m_energyfrac_reco.push_back(intersectionenergy/recodijetenergy);
	  m_energyfrac_true.push_back(intersectionenergy/truedijetenergy);
	  m_recodijet_energy.push_back(recodijet_v4.E());
	  m_recodijet_theta.push_back(recodijet_v4.Theta());
	  m_recodijet_phi.push_back(recodijet_v4.Phi());
	  //ICNs pairing
	  vector<ReconstructedParticle*> dijet_intersection_ICNs;
	  sort(recodijetPFOs_ICNs.begin(), recodijetPFOs_ICNs.end());
	  sort(truedijetPFOs_ICNs.begin(), truedijetPFOs_ICNs.end());
	  set_intersection(recodijetPFOs_ICNs.begin(), recodijetPFOs_ICNs.end(), truedijetPFOs_ICNs.begin(), truedijetPFOs_ICNs.end(), back_inserter(dijet_intersection_ICNs));
	  m_nrecodijetPFOs_ICNs.push_back(recodijetPFOs_ICNs.size());
	  m_ntruedijetPFOs_ICNs.push_back(truedijetPFOs_ICNs.size());
	  m_overlap_ICNs.push_back(float(dijet_intersection_ICNs.size())/float(recodijetPFOs_ICNs.size()));
	  m_recodijetmass_ICNs.push_back(inv_mass(recoJets.at(recojetpermICNs[i*2]),recoJets.at(recojetpermICNs[i*2+1])));
	  m_truedijetmass_ICNs.push_back(inv_mass(jet(truejetpermICNs[i*2]),jet(truejetpermICNs[i*2+1])));
	  double recodijetenergy_ICNs = 0;
	  double truedijetenergy_ICNs = 0;
	  double intersectionenergy_ICNs = 0;
	  for (auto p : dijet_intersection_ICNs) intersectionenergy_ICNs += p->getEnergy();
	  for (auto p : recodijetPFOs_ICNs) recodijetenergy_ICNs += p->getEnergy();
	  for (auto p : truedijetPFOs_ICNs) truedijetenergy_ICNs += p->getEnergy();
	  m_energyfrac_reco_ICNs.push_back(intersectionenergy_ICNs/recodijetenergy_ICNs);
	  m_energyfrac_true_ICNs.push_back(intersectionenergy_ICNs/truedijetenergy_ICNs);
	  m_recodijet_energy_ICNs.push_back(recodijet_v4_ICNs.E());
	  m_recodijet_theta_ICNs.push_back(recodijet_v4_ICNs.Theta());
	  m_recodijet_phi_ICNs.push_back(recodijet_v4_ICNs.Phi());

	  if (m_energyfrac_reco.back()>=0.95 && m_energyfrac_true.back()>=0.95) region.emplace_back("A");
	  if (m_energyfrac_reco.back()< 0.95 && m_energyfrac_true.back()>=0.95) region.emplace_back("B");
	  if (m_energyfrac_reco.back()>=0.95 && m_energyfrac_true.back()< 0.95) region.emplace_back("C");
	  if (m_energyfrac_reco.back()< 0.95 && m_energyfrac_true.back()< 0.95) region.emplace_back("D");
	  if (m_energyfrac_reco_ICNs.back()>=0.95 && m_energyfrac_true_ICNs.back()>=0.95) region_ICNs.emplace_back("A");
	  if (m_energyfrac_reco_ICNs.back()< 0.95 && m_energyfrac_true_ICNs.back()>=0.95) region_ICNs.emplace_back("B");
	  if (m_energyfrac_reco_ICNs.back()>=0.95 && m_energyfrac_true_ICNs.back()< 0.95) region_ICNs.emplace_back("C");
	  if (m_energyfrac_reco_ICNs.back()< 0.95 && m_energyfrac_true_ICNs.back()< 0.95) region_ICNs.emplace_back("D");
	}
	sort(region.begin(), region.end());
	sort(region_ICNs.begin(), region_ICNs.end());
	string XX;
	string XX_ICNs;
	if(region.size() >= 2) XX = string(region[0])+string(region[1]);
	if(region.size() >= 2) XX_ICNs = string(region_ICNs[0])+string(region_ICNs[1]);
	map<string, float> dict {
	  {"AA",0.}, {"AB",1.}, {"AC",2.}, {"AD",3.}, {"BB",4.}, {"BC",5.}, {"BD",6.}, {"CC",7.}, {"CD",8.}, {"DD",9.}
	};
	//{AA,AB,AC,AD,BB,BC,BD,CC,CD,DD}
	//{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
	m_regionXX.push_back(dict[XX]);
	m_regionXX_ICNs.push_back(dict[XX_ICNs]);
      }
    }
    for (auto n: m_nrecodijetPFOs) streamlog_out(DEBUG3) << n << endl;
    m_pTTree->Fill();
    m_nEvtSum++;
    m_nEvt++ ;
  }
  catch(DataNotAvailableException &e) {
    streamlog_out(MESSAGE) << "Check : Input collections not found in event " << m_nEvt << endl;
  }

}

float Misclustering::getMatchingByAngularSpace( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<pair<int,int>> &reco2truejetindex)
{

  assert(recoJets.size()==m_nRecoJets);

  TrueJet_Parser* trueJet = this;
  trueJet->getall( pLCEvent );

  int njets = trueJet->njets();
  vector<int> trueHadronicJetIndices; 
  for (int i_jet = 0 ; i_jet < njets ; i_jet++ ) {
    streamlog_out(DEBUG) << "type of jet " << i_jet << ": " << type_jet( i_jet ) << endl;   
    if ( type_jet( i_jet ) == 1 ) {
      trueHadronicJetIndices.push_back( i_jet );
    }
  }
  
  vector<int> arr(m_nRecoJets);
  for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++) {
    arr[i_array] = i_array;
  }
  
  float SmallestSumCosAngle = 99999.0;
  vector<int> matchedRecoJetIndices(m_nRecoJets);
  do {
    float sumcosangle = 0.0;
    
    //streamlog_out(DEBUG) << "Permutation of indices: [ "<< endl;
    //for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++) streamlog_out(DEBUG)<< arr[i_array] << " ";
    //streamlog_out(DEBUG) << "]" << endl;
    
    for (unsigned int i_Jet = 0 ; i_Jet < m_nRecoJets ; ++i_Jet ) {
      TVector3 trueJetMomentum(ptrueseen(trueHadronicJetIndices[i_Jet])[0], ptrueseen(trueHadronicJetIndices[i_Jet])[1], ptrueseen(trueHadronicJetIndices[i_Jet])[2]);
      TVector3 trueJetMomentumUnit = trueJetMomentum; trueJetMomentumUnit.SetMag(1.0);
      TVector3 recoJetMomentum( recoJets.at(arr[i_Jet])->getMomentum() );
      TVector3 recoJetMomentumUnit = recoJetMomentum; recoJetMomentumUnit.SetMag(1.0);
      //streamlog_out(DEBUG) << "true momentum = (" << trueJetMomentum(0) << ", " << trueJetMomentum(1) << ", " << trueJetMomentum(2) << ")" << endl;
      //streamlog_out(DEBUG) << "reco momentum = (" << recoJetMomentum(0) << ", " << recoJetMomentum(1) << ", " << recoJetMomentum(2) << ")" << endl;
      //streamlog_out(DEBUG) << "angle = " << acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit )) << endl;
      sumcosangle += acos(trueJetMomentumUnit.Dot( recoJetMomentumUnit ));
    }
    //streamlog_out(DEBUG) << "Sum of angles: " << sumcosangle << endl;
    if (sumcosangle<SmallestSumCosAngle) {
      SmallestSumCosAngle = sumcosangle;
      for (unsigned int i_array = 0; i_array < m_nRecoJets; i_array++)
	{
	  matchedRecoJetIndices[i_array] = arr[i_array];
	}
    }
  } while (next_permutation(arr.begin(),arr.begin()+m_nRecoJets));
  //m_sumofangles.push_back(SmallestSumCosAngle);
  for (unsigned int i_Jet = 0 ; i_Jet < m_nRecoJets ; ++i_Jet ) {
    streamlog_out(DEBUG2) << "True(seen) Jet Momentum[ " << trueHadronicJetIndices[ i_Jet ] << " ]: (" 
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 0 ] << " , " 
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 1 ] << " , "
			  << ptrueseen( trueHadronicJetIndices[ i_Jet ] )[ 2 ] << ")" << endl;
    streamlog_out(DEBUG2) << "True(seen) Jet [ " << trueHadronicJetIndices[ i_Jet ] << " ] is matched with RecoJet [ " << matchedRecoJetIndices[i_Jet] << " ]" << endl;

    reco2truejetindex.push_back(make_pair(matchedRecoJetIndices[i_Jet], trueHadronicJetIndices[i_Jet]));
  }
  return SmallestSumCosAngle;
}

bool Misclustering::getMatchingByLeadingParticle( EVENT::LCEvent *pLCEvent, vector<EVENT::ReconstructedParticle*> recoJets, vector<pair<int,int>> &reco2truejetindex)
{
  assert(recoJets.size()==m_nRecoJets);
  TrueJet_Parser* trueJet = this;
  trueJet->getall( pLCEvent );

  streamlog_out(DEBUG0) << " looking for leading particles in jets " << endl;
  for (unsigned int i=0; i<m_nRecoJets; ++i) {
    ReconstructedParticle* leadingParticleJet = nullptr;
    float leadingEnergyJet = 0.0;

    streamlog_out(DEBUG0) << " Jet[" << i << "] has "<< ( recoJets.at(i)->getParticles() ).size() << " particles" << endl;    
    for ( unsigned int i_par = 0 ; i_par < ( recoJets.at(i)->getParticles() ).size() ; ++i_par ) {
      ReconstructedParticle* pfo = ( ReconstructedParticle* )recoJets.at(i)->getParticles()[ i_par ];
      if ( abs( pfo->getType() ) == 12 || abs( pfo->getType() ) == 14 || abs( pfo->getType() ) == 16 ) continue;
      if ( pfo->getEnergy() > leadingEnergyJet ) {
	leadingParticleJet = pfo;
	leadingEnergyJet = pfo->getEnergy();
	streamlog_out(DEBUG0) << " So far, the energy of leading particle is: " << leadingEnergyJet << " GeV" << endl;
      }
    }
    if (leadingParticleJet == nullptr) {
      return false;
    }
    LCObjectVec jetvec = reltjreco->getRelatedFromObjects( leadingParticleJet );
    streamlog_out(DEBUG0) << jetvec.size() << " true Jet found for leading particle of jet[" << i << "]" << endl;
    if (jetvec.size() == 0) {
      return false;
    }   
    int trueJet_index = jetindex( dynamic_cast<ReconstructedParticle*>( jetvec[ 0 ] ) ); //truejet function
    streamlog_out(DEBUG0) << " true Jet[ " << trueJet_index << " ] has the leading particle of jet[" << i << "]" << endl;
    streamlog_out(DEBUG4) << "----------------------------------------------------------------------" << endl;
    streamlog_out(DEBUG4) << "    trueJet[" << i << "] TYPE:  " << type_jet( trueJet_index ) << endl;
    streamlog_out(DEBUG4) << "    trueJet[" << i << "] (Px,Py,Pz,E):  " 
			  << ptrue( trueJet_index )[ 0 ] << " , " 
			  << ptrue( trueJet_index )[ 1 ] << " , " 
			  << ptrue( trueJet_index )[ 2 ] << " , " 
			  << Etrue( trueJet_index ) << endl;
    streamlog_out(DEBUG4) << "----------------------------------------------------------------------" << endl;
    reco2truejetindex.push_back(make_pair(i,trueJet_index));

  }
  assert (reco2truejetindex.size() == m_nRecoJets);
  return true;
}

void Misclustering::check()
{

}

void Misclustering::end()
{
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;
}

