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

		// PRELIMINARY!!!
		bool m_use_matrix_elements() { return true; };

	protected:
		std::string m_JMP{};
		std::string m_JMSP{};

		float m_px31{};
		float m_py31{};
		float m_pz31{};
		float m_e31{};

		float m_px32{};
		float m_py32{};
		float m_pz32{};
		float m_e32{};

		// matrix element related
		float m_lcme_jet_matching_chi2{};
		float m_lcme_jet_matching_mz{};
		float m_lcme_jet_matching_mh1{};
		float m_lcme_jet_matching_mh2{};
		
};



#endif