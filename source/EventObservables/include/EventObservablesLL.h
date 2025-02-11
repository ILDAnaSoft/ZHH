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
};



#endif