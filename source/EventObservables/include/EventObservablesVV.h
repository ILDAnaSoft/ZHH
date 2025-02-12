#ifndef EventObservablesVV_h
#define EventObservablesVV_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

class EventObservablesVV : public EventObservablesBase {
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

	protected:
		std::string m_JMP{};
		std::string m_JMSP{};


};



#endif