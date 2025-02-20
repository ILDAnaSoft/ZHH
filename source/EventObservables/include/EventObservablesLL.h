#ifndef EventObservablesLL_h
#define EventObservablesLL_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

class EventObservablesLL : public EventObservablesBase {
	public:
		virtual Processor*  newProcessor() {
			return new EventObservablesLL();
		}
		EventObservablesLL();
		virtual ~EventObservablesLL() = default;
		EventObservablesLL(const EventObservablesLL&) = delete;
		EventObservablesLL& operator=(const EventObservablesLL&) = delete;

		// channel specific properties
		void prepareChannelTree();
		void clearChannelValues();
		void updateChannelValues(EVENT::LCEvent *pLCEvent);
     
	 	TTree *getTTree() { return m_pTTree; };
        TTree *m_pTTree = new TTree("EventObservablesLL", "EventObservablesLL");

		int m_nAskedJets() { return 4; };
		int m_nAskedIsoLeps() { return 2; };
		
		std::string m_jetMatchingParameter() { return m_JMP; };
		std::string m_jetMatchingSourceParameter() { return m_JMSP; };

		bool m_use_matrix_elements() { return true; };

		std::string m_yMinusParameter () { return "y34"; };
		std::string m_yPlusParameter () { return "y45"; };

	protected:
		// meta parameters
		std::string m_input2JetCollection{};

		std::string m_JMP{};
		std::string m_JMSP{};

		// data members
		int m_npfosmin4j{};
		int m_npfosmax4j{};

		// isolated lepton momenta and energies
		float m_px31{};
		float m_py31{};
		float m_pz31{};
		float m_e31{};

		float m_px32{};
		float m_py32{};
		float m_pz32{};
		float m_e32{};

		float m_plmin{};
		float m_plmax{};
		float m_mvalepminus{};
		float m_mvalepplus{};
		float m_mzll{};
		// float m_m_diff_z{};
		float m_mzll_pre_pairing{};
		int m_paired_lep_type{};

		// 2 jet
		float m_ptjmax2{};
		float m_pjmax2{};

		float m_cosJ1_2Jets{};
		float m_cosJ2_2Jets{};
        float m_cosJ12_2Jets{};
        float m_cosJ1Z_2Jets{};
        float m_cosJ2Z_2Jets{};

		// 4 jet
		float m_mbmax12{};
		float m_mbmax34{};

};



#endif