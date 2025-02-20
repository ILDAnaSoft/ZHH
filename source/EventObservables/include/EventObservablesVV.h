#ifndef EventObservablesVV_h
#define EventObservablesVV_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

class EventObservablesVV : public EventObservablesBase, public EventObservablesFromZZ {
	public:
		virtual Processor*  newProcessor() {
			return new EventObservablesVV();
		}
		EventObservablesVV();
		virtual ~EventObservablesVV() = default;
		EventObservablesVV(const EventObservablesVV&) = delete;
		EventObservablesVV& operator=(const EventObservablesVV&) = delete;

		// channel specific properties
		void prepareChannelTree();
		void clearChannelValues();
		void updateChannelValues(EVENT::LCEvent *pLCEvent);
     
	 	TTree *getTTree() { return m_pTTree; };
        TTree *m_pTTree = new TTree("EventObservablesVV", "EventObservablesVV");

		int m_nAskedJets() { return 4; };
		int m_nAskedIsoLeps() { return 0; };
		
		std::string m_jetMatchingParameter() { return m_JMP; };
		std::string m_jetMatchingSourceParameter() { return m_JMSP; };

		bool m_use_matrix_elements() { return false; };

		std::string m_yMinusParameter () { return "y34"; };
		std::string m_yPlusParameter () { return "y45"; };

	protected:
		// meta parameters
		std::string m_input5JetCollection{};
		std::string m_input6JetCollection{};

		std::string m_JMP{};
		std::string m_JMSP{};

		// data members
		std::vector<ReconstructedParticle*> m_5jets;
		//std::vector<ReconstructedParticle*> m_6jets;
		
		float m_ptjmax6{};
		float m_pjmax6{};

		// ttbar 5j
		float m_npfosmin5j{};
		float m_npfosmax5j{};
		float m_ptjmax5{};
		float m_pjmax5{};

		float m_yMinus5j{};
		float m_yPlus5j{};
		
		float m_massWtt5j{};
		float m_massT1tt5j{};
		float m_massT2tt5j{};

		// ttbar 4j
		float m_massWtt4j{};
		float m_massT1tt4j{};
		float m_massT2tt4j{};
};



#endif