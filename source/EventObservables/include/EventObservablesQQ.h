#ifndef EventObservablesQQ_h
#define EventObservablesQQ_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

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

		int m_nAskedJets() { return 6; };
		int m_nAskedIsoLeps() { return 0; };

		virtual bool m_use_matrix_elements() { return true; };

		std::string m_yMinusParameter () { return "y56"; };
		std::string m_yPlusParameter () { return "y67"; };

		// helper functions
		std::vector<float> chargeSumOfConstituents(std::vector<ReconstructedParticle*> jets);
		void calculateSimpleZHHChi2();

	protected:
		// meta parameters
		std::string m_input4JetCollection{};
		std::string m_zhhKinfitJetCollection{};

		// overriden parent functions
		void setJetCharges();

		// data members
		ReconstructedParticleVec m_4jets{};

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