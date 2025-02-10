#ifndef EventObservablesQQ_h
#define EventObservablesQQ_h 1

#include <TFile.h>
#include <TTree.h>
#include "EventObservablesBase.h"
#include "marlin/Processor.h"

class EventObservablesQQ : public Processor , public EventObservablesBase {
	public:
		virtual Processor*  newProcessor() {
			return new EventObservablesQQ;
		}
		EventObservablesQQ() ;
		virtual ~EventObservablesQQ() = default;
		EventObservablesQQ(const EventObservablesQQ&) = delete;
		EventObservablesQQ& operator=(const EventObservablesQQ&) = delete;
     
        TTree *m_pTTree = new TTree("EventObservablesQQ", "EventObservablesQQ");
};



#endif