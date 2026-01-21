#ifndef EventObservablesBase_h
#define EventObservablesBase_h 1

#include "nlohmann/json.hpp"
#include "marlin/Processor.h"
#include "marlin/VerbosityLevels.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/ReconstructedParticle.h"
#include "EVENT/MCParticle.h"
#include "UTIL/PIDHandler.h"
#include "lcio.h"
#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include <tuple>
#include "TLorentzVector.h"
#include <EventCategory.h>
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include <tuple>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2I.h"
#include "TVector.h"
#include "inv_mass.h"
#include "v4.h"
#include "EventObservablesFromZZ.h"
#include <TrueJet_Parser.h>

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;
using namespace lcme ;

TLorentzVector v4old(ReconstructedParticle* p);
TLorentzVector v4old(LCObject* lcobj);

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
// TODO: implement, using EventCategory

struct DelMe {
  DelMe( std::function<void()> func ) : _func(func) {}
  ~DelMe() { _func(); }
  std::function<void()>  _func;
};

class EventObservablesBase: public Processor, public TrueJet_Parser {
	public:
		EventObservablesBase(const std::string& name);
		virtual ~EventObservablesBase() = default;
		EventObservablesBase(const EventObservablesBase&) = delete;
		EventObservablesBase& operator=(const EventObservablesBase&) = delete;

		// used for clearing up TrueJet parsed data
		void delall2() {
			if (  relfcn != NULL ) { delete relfcn; relfcn = NULL; }
			if (  relicn != NULL ) { delete relicn; relicn = NULL; }
			if (  relfp != NULL ) { delete relfp; relfp = NULL; }
			if (  relip != NULL ) { delete relip; relip = NULL; }
			if (  reltjreco != NULL) { delete reltjreco; reltjreco = NULL; }
			if (  reltjmcp != NULL) { delete reltjmcp; reltjmcp = NULL; }
			if (  jets != NULL) { delete jets; jets = NULL; }
			if (  finalcns != NULL) { delete finalcns; finalcns = NULL; }
			if (  initialcns != NULL ) { delete initialcns; initialcns = NULL; }
			if ( reltrue_tj  != NULL ) { delete reltrue_tj; reltrue_tj = NULL; }
		};

		virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();

		// helper functions
		std::tuple<float, float> jetCharge(ReconstructedParticle* jet);
		static ReconstructedParticleVec getElements(LCCollection *collection, std::vector<int> elements);

		// return (smallest, largest number of PFOs, index of jet with least PFOs)
		static std::tuple<int, int, int> nPFOsMinMax(LCCollection *collection);

		static float leadingMomentum(ReconstructedParticleVec jets);
		static std::vector<ROOT::Math::PxPyPzEVector> toFourVectors(ReconstructedParticleVec jets) {
			std::vector<ROOT::Math::PxPyPzEVector> result;

			for (size_t i = 0; i < jets.size(); i++)
				result.push_back(v4(jets[i]));

			return result;
		};

		// dijet_targets: a list of PDGs to constitute the jets (in this order)
		// used in the 6jet case to reduce permutation space from (1-6)=720 to (1-4)=24
		static constexpr float kMassTop = 173.76;
		static constexpr float kMassZ   = 91.1876;
		static constexpr float kMassW   = 80.377;
		static constexpr float kMassH   = 125.;

		// mass resolutions
		static constexpr float kSigmaMassTop = 20.0;
		static constexpr float kSigmaMassZ   = 6.0;
		static constexpr float kSigmaMassW   = 4.8;
		static constexpr float kSigmaMassH   = 7.2;

		static std::tuple<std::vector<unsigned short>, std::vector<float>, float> pairJetsByMass(
			std::vector<ROOT::Math::PxPyPzEVector> jets, std::vector<unsigned short> dijet_targets);

		static std::tuple<std::vector<unsigned short>, std::vector<float>, float> pairJetsByMass(
			std::vector<ReconstructedParticle*> jets, std::vector<unsigned short> dijet_targets);
		static std::tuple<std::vector<unsigned short>, std::vector<float>, float> pairJetsByMass(
			const std::vector<ROOT::Math::PxPyPzEVector> jets,
			const std::vector<float> target_masses,
			const std::vector<float> target_resolutions,
			std::function<float (
				const std::vector<ROOT::Math::PxPyPzEVector>,
				const std::vector<unsigned short>,
				std::vector<float>&,
				const std::vector<float>,
				const std::vector<float>)> calc_chi2);

		static const std::vector<std::vector<unsigned short>> dijetPerms4;
		static const std::vector<std::vector<unsigned short>> dijetPerms6;

		static void getPermutationIndex(std::vector<int> input_perm, int size, short &perm_idx);
		
	protected:
		// common properties for all channels
		void prepareBaseTree();
		void clearBaseValues();
		void updateBaseValues(EVENT::LCEvent *pLCEvent);

		// channel specific properties; must be implemented by inheriting classes
		virtual void prepareChannelTree() = 0;
		virtual void clearChannelValues() = 0;
		virtual void updateChannelValues(EVENT::LCEvent *pLCEvent) = 0;
		virtual TTree* getTTree() = 0;
		virtual int m_nAskedJets() = 0;
		virtual int m_nAskedIsoLeps() = 0;
		virtual std::string m_yMinusParameter() = 0;
		virtual std::string m_yPlusParameter() = 0;
		virtual bool m_use_matrix_elements() = 0;
		virtual void calculateSimpleZHHChi2() = 0;

		// names of input collections
		std::string m_inputIsolatedleptonCollection{};
		std::string m_inputLepPairCollection{};
		std::string m_inputJetCollection{};
		std::string m_inputPfoCollection{};
		std::string m_inputJetKinFitZHHCollection{};
		std::string m_inputJetKinFitZZHCollection{};
		std::string m_inputLeptonKinFit_solveNuCollection{};
		std::string m_inputJetKinFit_solveNuCollection{};
		std::string m_outputFile{};
		std::string m_whichPreselection{};
		std::string m_cutDefinitionsJSONFile{};

		// flavortag
		std::string m_JetTaggingPIDAlgorithm{};
		std::string m_JetTaggingPIDParameterB{};
		std::string m_JetTaggingPIDParameterBbar{};
		std::string m_JetTaggingPIDParameterC{};
		std::string m_JetTaggingPIDParameterCbar{};
		std::vector<string> m_JetTaggingPIDParameters{};

		std::string m_JetTaggingPIDAlgorithm2{};
		std::string m_JetTaggingPIDParameterB2{};
		std::string m_JetTaggingPIDParameterC2{};

		// collections
		LCCollection *inputLKF_solveNuCollection{};
    	LCCollection *inputJKF_solveNuCollection{};

		// outputs
		bool m_write_ttree{};
		TFile *m_pTFile{};

		// enable/disable certain outputs and calculations in inheriting classes
		bool m_use_tags2{};

		// PreSelection cut values and inputs
		float m_maxdileptonmassdiff{};
		float m_maxdijetmassdiff{};
		float m_mindijetmass{};
		float m_maxdijetmass{};
		float m_minmissingPT{};
		float m_maxmissingPT{};
		float m_maxthrust{};
		float m_minblikeliness{};
		int m_minnbjets{};
		float m_maxEvis{};
		float m_minHHmass{};

		float m_ECM{};
		std::vector <float> m_polarizations{};
		float m_jetChargeKappa{};
		
		// meta information and observables
		int m_nRun;
		int m_nEvt;
		int m_statusCode;

		// errorCodes
		// 1000-1999: LL
		// 2000-2999: VV
		// 3000-3999: QQ
		std::vector<int> m_errorCodes{};

		ROOT::Math::PxPyPzEVector m_pmis{};
		ROOT::Math::PxPyPzEVector m_ecms{};
		
		float m_Evis{};
		float m_missingMass{};
		float m_invJetMass{};
		float m_missingPT{};
		float m_missingE{};
		float m_thrust{}; // principal thrust value
		float m_thrustMajor{};
		float m_thrustMinor{};
		float m_thrustAxisCos{}; // cos theta of principle thrust axis

		float m_ptpfochargedmax{}; // largest pt of charged PFOs
		float m_ppfochargedmax{}; // largest momentum magntiude of charged PFOs

		float m_ptpfomax{};
		float m_ptjmax{};
		float m_pjmax{};
		float m_cosjmax{};
		
		int m_nJets{};
		int m_nIsoLeps{};
		int m_nIsoElectrons{};
		int m_nIsoMuons{};
		int m_nIsoTaus{};
		int m_pairedLepType{};
		int m_npfos{};
		std::vector<int> m_lep_types{};

		float m_yMinus{};
		float m_yPlus{};
		
		// four momenta of leptons + jets and all flavor tags
		std::vector<ReconstructedParticle*> m_jets{};
		std::vector<ROOT::Math::PxPyPzEVector> m_jets4v{};
		std::vector<float> m_jetsMasses{}; 
		std::vector<std::vector<float>> m_jetTags{};

		// pure mass chi2
		std::vector<unsigned short> m_zhh_jet_matching{};
		float m_zhh_mz{};
		float m_zhh_mh1{};
		float m_zhh_mh2{};
		float m_zhh_mhh{};
		float m_zhh_chi2{};
		float m_zhh_p1st{};
		float m_zhh_cosTh1st{};

		typedef std::pair<unsigned short, double> JetTaggingPair;

		// returns a vector of pairs (jet idx, tag value) sorted ASCENDING by btags given a collection
		static std::vector<std::pair<int, float>> sortedTagging(LCCollection* collection, std::string pid_algorithm, std::string pid_parameter_b);

		// returns a vector of pairs (jet idx, tag value) sorted ASCENDING by btags
		static std::vector<std::pair<int, float>> sortedTagging(std::vector<float> tags_by_jet_order);

		static bool jetTaggingComparator ( const JetTaggingPair& l, const JetTaggingPair& r) { return l.second > r.second || std::isnan(r.second); };
		std::vector<JetTaggingPair> m_bTagsSorted{}; // (jet index, btag1value) sorted DESC; first highest, last lowest
		std::vector<JetTaggingPair> m_bTagsSorted2{}; // (jet index, btag2value) sorted DESC; first highest, last lowest
		std::vector<JetTaggingPair> m_cTagsSorted{}; // (jet index, ctag1value) sorted DESC; first highest, last lowest
		std::vector<JetTaggingPair> m_cTagsSorted2{}; // (jet index, ctag1value) sorted DESC; first highest, last lowest
		std::vector<double> m_bTagValues{};
		std::vector<double> m_cTagValues{};

		float m_cosbmax{};

		float m_bmax1{};
		float m_bmax2{};
		float m_bmax3{};
		float m_bmax4{};

		float m_cmax1{};
		float m_cmax2{};
		float m_cmax3{};
		float m_cmax4{};

		std::vector<double> m_bTagValues2{};
		std::vector<double> m_cTagValues2{};

		float m_bmax12{};
		float m_bmax22{};
		float m_bmax32{};
		float m_bmax42{};

		float m_cmax12{};
		float m_cmax22{};
		float m_cmax32{};
		float m_cmax42{};

		// jet momenta and energies
		float m_jet1_q{};
		float m_jet1_qdyn{}; // dynamic jet charge, see https://arxiv.org/pdf/2101.04304

		float m_jet2_q{};
		float m_jet2_qdyn{};

		float m_jet3_q{};
		float m_jet3_qdyn{};

		float m_jet4_q{};
		float m_jet4_qdyn{};

		void setJetCharges();

		// jet matching from kinfit
		std::vector<int> m_JMK_ZHH{};
		std::vector<int> m_JMK_ZZH{};
		std::vector<int> m_JMK_best{};

		float m_fitprob_ZHH{};
        float m_fitprob_ZZH{};
		float m_fitchi2_ZHH{};
        float m_fitchi2_ZZH{};

		std::vector<ROOT::Math::PxPyPzEVector> m_jets4cKinFit_4v{};
		std::vector<ROOT::Math::PxPyPzEVector> m_leps4cKinFit_4v{};

		std::vector<float> m_fit4C_masses{};
		float m_fit4C_mz{};
		float m_fit4C_mh1{};
		float m_fit4C_mh2{};

		//short m_JMK_ZHH_perm_idx{};
		//short m_JMK_ZZH_perm_idx{};

		#ifdef CALC_ME
		// matrix element code
		double m_lcme_jmk_zhh_log{};
		double m_lcme_jmk_zzh_log{};

		// log of the mean over all permutations, and over the given polarization
		double m_lcme_zhh_log{};
		double m_lcme_zzh_log{};

		vector<float> m_lcme_weights{};
		vector<double> m_lcme_zhh_raw{};
		vector<double> m_lcme_zzh_raw{};

		lcme::LCMEZHH *m_lcmezhh{}; // ZHH MEM calculator instance
		lcme::LCMEZZH *m_lcmezzh{}; // ZZH MEM calculator instance

		// the following is energy dependant; if ECM changes, this may need to be updated!!!
		std::map<int, int> m_pdg_to_lcme_mode = {
			{  1,  9 },
			{  2,  7 },
			{  3, 10 },
			{  4,  8 },
			{  5, 11 },
			{ 11,  4 },
			{ 12,  1 },
			{ 13,  5 },
			{ 14,  2 },
			{ 15,  6 },
			{ 16,  3 }
		};

		// assumptions:
		// - from_z1 + from_z2 come from Z, relates to z_decay_pdg; can be any of (ll, vv, qq)
		// - jet3 + jet4 come from either H, or Z (if H, then simply Hdijet=jet3+jet4), relates to z_or_h_decay_pdg
		// - dijet: a Higgs present in both ZHH and ZZH
		// flavor of jet3, jet4 
		void calculateMatrixElements(
			int z1_decay_pdg, // Z=dijet1
			int dj2_decay_pdg, // dijet2
			TLorentzVector from_z1, TLorentzVector from_z2,
			TLorentzVector jet1, TLorentzVector jet2,
			TLorentzVector jet3, TLorentzVector jet4,
			bool permute_from_z);

		void calculateMatrixElements(
			int z1_decay_pdg, // Z=dijet1
			int dj2_decay_pdg, // dijet2
			TLorentzVector from_z1, TLorentzVector from_z2,
			TLorentzVector jet1, TLorentzVector jet2,
			TLorentzVector jet3, TLorentzVector jet4,
			bool permute_from_z,
			unsigned short nperms,
			std::vector<float> weights
			);
		#endif

		// TrueJet information
		short m_useTrueJet{};
		short m_trueJetN{};
		std::vector<ROOT::Math::PxPyPzEVector> m_trueJetMomenta{};
		std::vector<ROOT::Math::PxPyPzEVector> m_trueISRMomenta{};
		std::vector<int> m_trueJetTypes{};
		std::vector<int> m_trueJetPDGs{};

		short m_trueLeptonN{};
		std::vector<ROOT::Math::PxPyPzEVector> m_trueLeptonMomenta{};
		std::vector<int> m_trueLeptonPDGs{};

		std::vector<int> m_trueJetICNTypes{};
		std::vector<int> m_trueJetICNPDGs{};
		std::vector<int> m_trueJetICNTrueJetsIndices{};
		std::vector<int> m_reco2TrueJetIndex{};
		std::vector<int> m_true2RecoJetIndex{};
		std::vector<int> m_trueJetHiggsICNPairs{};

		float getMatchingByAngularSpace(
			vector<EVENT::ReconstructedParticle*> recoJets,
			vector<int> &reco2truejetindex,
			vector<int> &true2recojetindex,
			vector<int> &trueHadronicJetIndices,
			vector<int> &trueLeptonIndices,
			vector<int> &trueISRIndices );

		float getMatchingByAngularSpace(
			vector<EVENT::ReconstructedParticle*> recoJets,
			vector<EVENT::MCParticle*> quarkMCParticles,
			vector<int> &reco2MCPindex,
			vector<int> &true2MCPindex );

		bool m_trueRecoJetsMapped{}; // only true if number of true jets is equal to reco jets and they could be mapped by angular overlap 

};

#endif
