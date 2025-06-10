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

		// helper functions
		void calculateSimpleZHHChi2();

	protected:
		// meta parameters
		std::string m_input2JetCollection{};

		std::string m_JMP{};
		std::string m_JMSP{};

		// data members
		int m_npfosmin4j{};
		int m_npfosmax4j{};

		// isolated lepton momenta and energies
		float m_pxl1{};
		float m_pyl1{};
		float m_pzl1{};
		float m_el1{};

		float m_pxl2{};
		float m_pyl2{};
		float m_pzl2{};
		float m_el2{};

		float m_plmin{};
		float m_plmax{};
		float m_mvalepminus{};
		float m_mvalepplus{};
		float m_mzll{};
		// float m_m_diff_z{};
		float m_mzll_pre_pairing{};
		int m_paired_lep_type{};

		// 2 jet
		float m_pxj1_2Jets{};
		float m_pyj1_2Jets{};
		float m_pzj1_2Jets{};
		float m_ej1_2Jets{};

		float m_pxj2_2Jets{};
		float m_pyj2_2Jets{};
		float m_pzj2_2Jets{};
		float m_ej2_2Jets{};

		float m_ptjmin2{};
		float m_pjmin2{};

		float m_ptjmax2{};
		float m_pjmax2{};

		float m_m_inv_2Jets{};

		float m_cosJ1_2Jets{};
		float m_cosJ2_2Jets{};
        float m_cosJ12_2Jets{};
        float m_cosJ1Z_2Jets{};
        float m_cosJ2Z_2Jets{};
		float m_cosJZMax_2Jets{};

		float m_yMinus2{};
		float m_yPlus2{};

		std::vector<double> m_bTagValues_2Jets{};
		std::vector<double> m_bTagValues_2Jets2{};

		float m_bmax1_2Jets{};
		float m_bmax2_2Jets{};

		float m_bmax12_2Jets{};
		float m_bmax22_2Jets{};

		// 4 jet
		float m_mbmax12{};
		float m_mbmax34{};

};



#endif