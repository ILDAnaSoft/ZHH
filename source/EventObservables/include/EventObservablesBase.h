#ifndef EventObservablesBase_h
#define EventObservablesBase_h 1

#include "nlohmann/json.hpp"
#include "marlin/Processor.h"
#include "marlin/VerbosityLevels.h"
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
		virtual int m_nAskedJets() = 0;
		virtual int m_nAskedIsoLeps() = 0;
		virtual std::string m_jetMatchingParameter() = 0; // e.g. best_perm_ll
		virtual std::string m_jetMatchingSourceParameter() = 0; // e.g. best_perm_ll_from_kinfit ; 1 for "from mass chi2", 2 for "from kinfit"

		// names of input collections
		std::string m_inputIsolatedleptonCollection{};
		std::string m_inputLepPairCollection{};
		std::string m_inputJetCollection{};
		std::string m_inputPfoCollection{};
		std::string m_outputFile{};
		std::string m_whichPreselection{};
		std::string m_cutDefinitionsJSONFile{};
		std::string m_JetTaggingPIDAlgorithm{};
		std::string m_JetTaggingPIDParameterB{};
		std::string m_JetTaggingPIDParameterC{};
		bool m_write_ttree{};

		// outputs
		TFile *m_pTFile{};

		// PreSelection cut values and inputs
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

		float m_missingPT{};
		float m_missingE{};
		float m_thrust{};

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

		// jet momenta and energies
		float m_px11{};
		float m_py11{};
		float m_pz11{};
		float m_e11{};

		float m_px12{};
		float m_py12{};
		float m_pz12{};
		float m_e12{};

		float m_px21{};
		float m_py21{};
		float m_pz21{};
		float m_e21{};

		float m_px22{};
		float m_py22{};
		float m_pz22{};
		float m_e22{};

		// jet matching
		std::vector<int> m_jet_matching{};
		int m_jet_matching_source{}; // 1 for "from mass chi2", 2 for "from kinfit"


		int m_nIsoLeps{};
		std::vector<int> m_lep_types{};
		

		/*  old variables for preselection (to be done in post)
		most are now calculated by the Kinfit processors
		float m_dileptonMassPrePairing{};
		float m_dileptonMass{};
		float m_dileptonMassDiff{};
		float m_chi2min{};
		int m_ndijets{};
		
		std::vector<int> m_dijetPairing{};
		std::vector<float> m_dijetMass{};
		std::vector<float> m_dijetMassDiff{};
		float m_dihiggsMass{};

		std::vector<int>  m_preselsPassedVec{};
		size_t m_preselsPassedAll{};
		int m_preselsPassedConsec{};
		int m_nbjets{};
		int m_isPassed{};
		*/

		std::vector<double> m_bTagValues{};
		std::vector<double> m_cTagValues{};
		std::string m_jetMatchingParamName{};

};

#endif
