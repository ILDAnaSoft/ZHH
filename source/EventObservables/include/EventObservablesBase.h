#ifndef EventObservablesBase_h
#define EventObservablesBase_h 1

#include "nlohmann/json.hpp"
#include "marlin/Processor.h"
#include "marlin/VerbosityLevels.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/ReconstructedParticle.h"
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
#include "inv_mass.h"
#include "v4.h"

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;
using namespace lcme ;

TLorentzVector v4old(ReconstructedParticle* p);
TLorentzVector v4old(LCObject* lcobj);

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
// TODO: implement, using EventCategory

class EventObservablesBase: public Processor
{
	public:
		EventObservablesBase(const std::string& name);
		virtual ~EventObservablesBase() = default;
		EventObservablesBase(const EventObservablesBase&) = delete;
		EventObservablesBase& operator=(const EventObservablesBase&) = delete;

		virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();

		// helper functions
		ReconstructedParticleVec getElements(LCCollection *collection, std::vector<int> elements);
		std::pair<int, int> nPFOsMinMax(LCCollection *collection); // smallest and largest number of PFOs of jets
		float leadingMomentum(ReconstructedParticleVec jets);
		
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
		virtual std::string m_jetMatchingParameter() = 0; // e.g. best_perm_ll
		virtual std::string m_jetMatchingSourceParameter() = 0; // e.g. best_perm_ll_from_kinfit ; 1 for "from mass chi2", 2 for "from kinfit"
		virtual std::string m_yMinusParameter() = 0;
		virtual std::string m_yPlusParameter() = 0;

		virtual bool m_use_matrix_elements() = 0;

		// names of input collections
		std::string m_inputIsolatedleptonCollection{};
		std::string m_inputLepPairCollection{};
		std::string m_inputJetCollection{};
		std::string m_inputPfoCollection{};
		std::string m_outputFile{};
		std::string m_whichPreselection{};
		std::string m_cutDefinitionsJSONFile{};

		// flavortag
		std::string m_JetTaggingPIDAlgorithm{};
		std::string m_JetTaggingPIDParameterB{};
		std::string m_JetTaggingPIDParameterC{};

		std::string m_JetTaggingPIDAlgorithm2{};
		std::string m_JetTaggingPIDParameterB2{};
		std::string m_JetTaggingPIDParameterC2{};

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

		// matrix elements; here given as log of the mean over 4 permutations, and over ghe given polarization
		double m_lcme_zhh_log{};
		double m_lcme_zzh_log{};

		lcme::LCMEZHH *m_lcmezhh{}; // ZHH MEM calculator instance
		lcme::LCMEZZH *m_lcmezzh{}; // ZZH MEM calculator instance

		// dijet_targets: a list of PDGs to constitute the jets (in this order)
		// used in the 6jet case to reduce permutation space from (1-6)=720 to (1-4)=24
		std::tuple<std::vector<unsigned short>, std::vector<float>, float> pairJetsByMass(std::vector<ReconstructedParticle*> jets, std::vector<unsigned short> dijet_targets);

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

		// meta information and observables
		int m_nRun;
		int m_nEvt;
		int m_errorCode;

		float m_todo{};

		float m_Evis{};
		float m_missingMass{};
		float m_missingPT{};
		float m_missingE{};
		float m_thrust{}; // principal thrust value
		float m_thrustMajor{};
		float m_thrustMinor{};
		float m_thrustAxisCos{}; // cos theta of principle thrust axis

		float m_ptpfomax{};
		float m_ptjmax{};
		
		int m_nJets{};
		int m_nIsoLeps{};
		int m_npfos{};
		std::vector<int> m_lep_types{};

		//float m_mh1{};
		//float m_mh2{};
		//float m_mhh{};
		float m_pz{};
		float m_ph1{};
		float m_ph2{};
		float m_cosz{};
		float m_cosh1{};
		float m_cosh2{};
		float m_yMinus{};
		float m_yPlus{};
		
		std::vector<ReconstructedParticle*> m_jets;

		typedef std::pair<unsigned short, double> JetTaggingPair;
		static bool jetTaggingComparator ( const JetTaggingPair& l, const JetTaggingPair& r) { return l.first < r.first; };
		std::vector<JetTaggingPair> m_bTagsSorted{}; // (jet index, btag1value) sorted; first highest, last lowest
		std::vector<double> m_bTagValues{};
		std::vector<double> m_cTagValues{};

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
		float m_pxj1{};
		float m_pyj1{};
		float m_pzj1{};
		float m_ej1{};

		float m_pxj2{};
		float m_pyj2{};
		float m_pzj2{};
		float m_ej2{};

		float m_pxj3{};
		float m_pyj3{};
		float m_pzj3{};
		float m_ej3{};

		float m_pxj4{};
		float m_pyj4{};
		float m_pzj4{};
		float m_ej4{};

		void setJetMomenta();

		// jet matching
		std::vector<int> m_jet_matching{};
		int m_jet_matching_source{}; // 1 for "from mass chi2", 2 for "from kinfit"
		bool m_using_kinfit{}; // 1 if m_jet_matching_source == 2
		bool m_using_mass_chi2{}; // 1 if m_jet_matching_source == 1

		/*  old variables for preselection (to be done in post)
		most are now calculated by the Kinfit processors
		float m_dileptonMassPrePairing{};
		float m_dileptonMass{};
		float m_dileptonMassDiff{};
		float m_chi2min{};
		int m_ndijets{};
		
		std::vector<int> m_dijetPairing{};
		std::vector<float> m_dijetMass{};
		std::vector<float> m_dijetMassDiff{};
		float m_dihiggsMass{};

		std::vector<int>  m_preselsPassedVec{};
		size_t m_preselsPassedAll{};
		int m_preselsPassedConsec{};
		int m_nbjets{};
		int m_isPassed{};
		*/

		std::string m_jetMatchingParamName{};

};

#endif
