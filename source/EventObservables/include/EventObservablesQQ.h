#ifndef EventObservablesQQ_h
#define EventObservablesQQ_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

// additional discriminating features of the qqHH(bbbb) analysis; all float so they can be written/cleared as one block
struct QQExtraFeatures {
	// ZHH hypothesis from jet masses only: Z->qq and H->bb dijets taken from all 45 pairings (chi2 incl. Z mass)
	float zhh6_chi2{}, zhh6_mz{}, zhh6_mh1{}, zhh6_mh2{}, zhh6_mhh{};
	float zhh6_recoil_hh{}, zhh6_recoil_z{};
	float zhh6_cosz{}, zhh6_coshh{}, zhh6_cos_hh{}, zhh6_cos_zjj{}, zhh6_cosh1star{}, zhh6_cosh2star{};
	// ZZH hypothesis (qqqqH background)
	float zzh6_chi2{}, zzh6_mh{}, zzh6_mz1{}, zzh6_mz2{};
	// WW/ZZ->4q hypothesis from the 4 jet clustering
	float ww4_chi2{}, ww4_mw1{}, ww4_mw2{};
	// flavor tagging
	float sumBTags{}, btag4sum{}, btag4prod{}, ctagz{};
	// missing momentum
	float pzmiss{}, cosmiss{};
	// event shape
	float sphericity{}, aplanarity{}, fw2{};
	// jet topology
	float cosjj_max{}, mjj_min{};
	// post-fit kinematics of the ZHH kinematic fit (dijet masses are fixed by the constraints and therefore not stored)
	float kfzhh_mhh{}, kfzhh_cosz{}, kfzhh_cos_hh{};
};

class EventObservablesQQ : public EventObservablesBase, public EventObservablesFromZZ {
	public:
		virtual Processor*  newProcessor() {
			return new EventObservablesQQ();
		}
		EventObservablesQQ();
		virtual ~EventObservablesQQ() = default;
		EventObservablesQQ(const EventObservablesQQ&) = delete;
		EventObservablesQQ& operator=(const EventObservablesQQ&) = delete;

		// channel specific properties
		void prepareChannelTree();
		void clearChannelValues();
		void updateChannelValues(EVENT::LCEvent *pLCEvent);
     
	 	TTree *getTTree() { return m_pTTree; };
        TTree *m_pTTree = new TTree("EventObservablesQQ", "EventObservablesQQ");

		unsigned int m_nAskedJets() { return 6; };
		unsigned int m_nAskedIsoLeps() { return 0; };

		virtual bool m_use_matrix_elements() { return true; };
		bool m_channelRequiresKinfit() { return false; };

		std::string m_yMinusParameter () { return "y56"; };
		std::string m_yPlusParameter () { return "y67"; };

		// helper functions
		std::vector<float> chargeSumOfConstituents(std::vector<ReconstructedParticle*> jets);
		void calculateSimpleZHHChi2();
		void calculateExtraFeatures(EVENT::LCEvent *pLCEvent);

	protected:
		// meta parameters
		std::string m_input4JetCollection{};
		std::string m_zhhKinfitJetCollection{};

		// overriden parent functions

		// data members
		ReconstructedParticleVec m_4jets{};

		QQExtraFeatures m_x{};
		int m_nbjets05{};

		int m_npfosmin6j{};
		int m_npfosmax6j{};

		float m_cosjmax4{};
		float m_pjmax4{};
		float m_ptjmax4{};

		float m_jet5_q{};
		float m_jet5_qdyn{};

		float m_jet6_q{};
		float m_jet6_qdyn{};

		float m_bmax5{};
		float m_bmax6{};
		float m_bTagZ{};

		// ttbar
		std::vector<float> m_tt_target_masses{};
		std::vector<float> m_tt_target_resolutions{};

		float m_tt_mw1{};
		float m_tt_mw2{};
		float m_tt_mt1{};
		float m_tt_mt2{};
		float m_tt_chi2{};		
};



#endif