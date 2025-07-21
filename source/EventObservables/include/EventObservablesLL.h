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
		std::string m_inputIsoElectrons{};
		std::string m_inputIsoMuons{};
		std::string m_inputIsoTaus{};
		std::vector<string> m_2JetTaggingPIDParameters{};

		std::string m_JMP{};
		std::string m_JMSP{};

		// data members
		int m_npfosmin4j{};
		int m_npfosmax4j{};

		// isolated lepton momenta and energies
		std::vector<ROOT::Math::PxPyPzEVector> m_leps4v{};
		int m_typel1{};
		int m_typel2{};

		float m_plmin{};
		float m_plmax{};
		float m_mvalepminus{};
		float m_mvalepplus{};
		float m_mzll{};
		// float m_m_diff_z{};
		float m_mzll_pre_pairing{};

		// 2 jet
		std::vector<std::vector<float>> m_2jetTags{};
		std::vector<ROOT::Math::PxPyPzEVector> m_2jets4v{};
		float m_2jet1_m{};
		float m_2jet2_m{};
		float m_2jet_m_inv{};

		float m_ptjmin2{};
		float m_pjmin2{};

		float m_ptjmax2{};
		float m_pjmax2{};

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