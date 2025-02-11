#ifndef EventObservablesBase_h
#define EventObservablesBase_h 1

#include "nlohmann/json.hpp"
#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/ReconstructedParticle.h"
#include "lcio.h"
#include <string>
#include <vector>
#include "TLorentzVector.h"
#include <EventCategory.h>
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include <tuple>
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2I.h"
#include "inv_mass.h"

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;
using namespace lcme ;

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
// TODO: implement, using EventCategory

class EventObservablesBase: public Processor
{
	public:
		EventObservablesBase(const std::string& name);
		virtual ~EventObservablesBase() = default;
		EventObservablesBase(const EventObservablesBase&) = delete;
		EventObservablesBase& operator=(const EventObservablesBase&) = delete;

		virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();
		
	protected:
		// common properties for all channels
		void prepareBaseTree();
		void clearBaseValues();
		void updateBaseValues(EVENT::LCEvent *pLCEvent);

		// channel specific properties; must be implemented by inheriting classes
		virtual void prepareChannelTree() = 0;
		virtual void clearChannelValues() = 0;
		virtual void updateChannelValues(EVENT::LCEvent *pLCEvent) = 0;
		virtual TTree* getTTree() = 0;

		std::vector<int> pairJetsByMass(std::vector<EVENT::ReconstructedParticle*> jets, IMPL::LCCollectionVec* higgsCandidates);

		// names of input collections
		std::string m_inputIsolatedleptonCollection{};
		std::string m_inputLepPairCollection{};
		std::string m_inputJetCollection{};
		std::string m_inputPfoCollection{};
		std::string m_outputFile{};
		std::string m_whichPreselection{};
		std::string m_cutDefinitionsJSONFile{};
		std::string m_PIDAlgorithmBTag{};
		bool m_write_ttree{};

		// outputs
		TFile *m_pTFile{};

		// PreSelection cut values and inputs
		int m_nAskedJets{};
        int m_nAskedIsoLeps{};
		float m_maxdileptonmassdiff{};
		float m_maxdijetmassdiff{};
		float m_mindijetmass{};
		float m_maxdijetmass{};
		float m_minmissingPT{};
		float m_maxmissingPT{};
		float m_maxthrust{};
		float m_minblikeliness{};
		int m_minnbjets{};
		float m_maxEvis{};
		float m_minHHmass{};

		float m_ECM{};

		// meta information and observables
		int m_nRun;
		int m_nEvt;
		int m_errorCode;

		float m_todo{};

		int m_ndijets{};

		float m_Evis{};
		float m_missingMass{};
		float m_mh1{};
		float m_mh2{};
		float m_mhh{};
		float m_pz{};
		float m_ph1{};
		float m_ph2{};
		float m_cosz{};
		float m_cosh1{};
		float m_cosh2{};
		float m_yminus{}; //?
		float m_yplus{}; //?

		int m_nhbb{};
		int m_nJets{};

		float m_bmax1{};
		float m_bmax2{};
		float m_bmax3{};
		float m_bmax4{};

		float m_cmax1{};
		float m_cmax2{};
		float m_cmax3{};
		float m_cmax4{};



		int m_nIsoLeps{};
		std::vector<int> m_lepTypes{};
		int m_lepTypesPaired{};
		float m_missingPT{};
		
		float m_missingE{};
		
		float m_thrust{};
		float m_dileptonMassPrePairing{};
		float m_dileptonMass{};
		float m_dileptonMassDiff{};
		float m_chi2min{};
		
		int m_isPassed{};

		// Event values for improving/investigating preselection efficiency
		std::vector<int> m_dijetPairing{};
		std::vector<float> m_dijetMass{};
		std::vector<float> m_dijetMassDiff{};
		std::vector<double> m_bTagValues{};
		std::vector<double> m_cTagValues{};
		
		float m_dihiggsMass{};
		std::vector<int>  m_preselsPassedVec{};

		size_t m_preselsPassedAll{};
		int m_preselsPassedConsec{};
		int m_nbjets{};
		std::vector<float> m_blikelihoodness{};

};

#endif
