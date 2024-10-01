#ifndef PreSelection_h
#define PreSelection_h 1

#include "nlohmann/json.hpp"
#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include "TLorentzVector.h"
class TFile;
class TH1F;
class TH1I;
class TH2I;
class TTree;

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
enum EVENT_CATEGORY_ZHH: unsigned int {
	NONE = 0,
	OTHER = 1,
	LEPTONIC = 11,
	NEUTRINO = 21,
	HADRONIC = 31
};

class PreSelection : public Processor
{
	public:

		virtual Processor*  newProcessor()
		{
			return new PreSelection;
		}
		PreSelection();
		virtual ~PreSelection() = default;
		PreSelection(const PreSelection&) = delete;
		PreSelection& operator=(const PreSelection&) = delete;
		virtual void init();
		virtual void Clear();
		virtual void processRunHeader( LCRunHeader*  /*run*/);
		virtual void processEvent( EVENT::LCEvent *pLCEvent );
		virtual void check();
		virtual void end();
		
 protected:
		
		/**
		 * Add the expected output collections
		 */
		
		/** Input collection name.
		 */
		std::string m_inputIsolatedleptonCollection{};
		std::string m_inputLepPairCollection{};
		std::string m_inputJetCollection{};
		std::string m_inputPfoCollection{};
		std::string m_PreSelectionCollection{};
		std::string m_HiggsCollection{};
		std::string m_outputFile{};
		std::string m_whichPreselection{};
		std::string m_isPassedCollection{};
		std::string m_cutDefinitionsJSONFile{};

		int m_nAskedJets{};
        int m_nAskedIsoLeps{};
		float m_ECM{};

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

		int m_nRun;
		int m_nEvt;
		int m_errorCode;

		int m_nJets{};
		int m_nIsoLeps{};
		std::vector<int> m_lepTypes{};
		float m_missingPT{};
		float m_Evis{};
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
		
		float m_dihiggsMass{};
		std::vector<int>  m_preselsPassedVec{};

		int m_preselsPassedAll{};
		int m_preselsPassedConsec{};
		int m_nbjets{};

		TFile *m_pTFile{};        
		TTree *m_pTTree{};

};

#endif
