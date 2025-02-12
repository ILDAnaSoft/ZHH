#ifndef EventObservablesQQ_h
#define EventObservablesQQ_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

class EventObservablesQQ : public EventObservablesBase {
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
		
		std::string m_jetMatchingParameter() { return m_JMP; };
		std::string m_jetMatchingSourceParameter() { return m_JMSP; };

	protected:
		std::string m_JMP{};
		std::string m_JMSP{};


};



#endif