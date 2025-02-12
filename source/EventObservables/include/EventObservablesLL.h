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

	protected:
		std::string m_JMP{};
		std::string m_JMSP{};

		// isolated lepton momenta and energies
		float m_px31{};
		float m_py31{};
		float m_pz31{};
		float m_e31{};

		float m_px32{};
		float m_py32{};
		float m_pz32{};
		float m_e32{};

		// dilepton mass; first two calculated by Kinfit processor
		// float m_mzll{};
		// float m_m_diff_z{};
		float m_mzll_pre_pairing{};
		int m_paired_lep_type{};
		
};



#endif