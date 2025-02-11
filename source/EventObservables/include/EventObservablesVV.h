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
};



#endif