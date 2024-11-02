#ifndef TruthRecoComparison_h
#define TruthRecoComparison_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <vector>
#include "FinalStateResolver.h"
#include "TLorentzVector.h"
#include "UTIL/LCRelationNavigator.h"

class TFile;
class TTree;

using namespace lcio;
using namespace marlin;

struct ERROR_CODES {
	enum Values: int {
		UNINITIALIZED = -1,
		OK = 0,
		COLLECTION_NOT_FOUND = 7001,
		UNKNOWN_ERROR = 7002
	};
};


class TruthRecoComparison : public Processor
{
	private:
		float m_beam_pol1{};
		
		TLorentzVector m_mcp_mom_tot{0., 0., 0., 0.};
		TLorentzVector m_mcp_mom_detected{0., 0., 0., 0.};
		TLorentzVector m_mcp_mom_undetected{0., 0., 0., 0.};
		TLorentzVector m_pfo_mom_tot{0.,0.,0.,0.};

		float m_mcp_E_tot{};
		float m_mcp_px_tot{};
		float m_mcp_py_tot{};
		float m_mcp_pz_tot{};

		float m_mcp_E_detected{};
		float m_mcp_px_detected{};
		float m_mcp_py_detected{};
		float m_mcp_pz_detected{};

		float m_mcp_E_undetected{};
		float m_mcp_px_undetected{};
		float m_mcp_py_undetected{};
		float m_mcp_pz_undetected{};

		float m_true_Et{};
		float m_reco_Et{};

		float m_true_pt_miss{};
		float m_reco_pt_miss{};

		float m_true_m_miss{};
		float m_reco_m_miss{};

		float m_true_E_miss{};
		float m_reco_E_miss{};

	public:
		virtual Processor*  newProcessor()
		{
			return new TruthRecoComparison;
		}
		TruthRecoComparison();
		virtual ~TruthRecoComparison() = default;
		TruthRecoComparison(const TruthRecoComparison&) = delete;
		TruthRecoComparison& operator=(const TruthRecoComparison&) = delete;

		virtual void init();
		virtual void clear();
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
		std::string m_inputPfoCollection{};
		std::string m_recoMCTruthLink{};
		std::string m_mcTruthRecoLink{};
		std::string m_mcParticleCollection{};
		std::string m_outputRootFile{};

		float m_ECM{};

		int m_n_run;
		int m_n_evt = 0;
		int m_n_evt_sum = 0;
		int m_error_code;

		// Output ROOT file
		TFile *m_pTFile{};
		TTree *m_pTTree{};

		// Truth kinematics
		EVENT::ReconstructedParticle* TruthRecoComparison::getLinkedPFO( EVENT::MCParticle *mcParticle , LCRelationNavigator RecoMCParticleNav , LCRelationNavigator MCParticleRecoNav , bool getChargedPFO , bool getNeutralPFO , float &weightPFOtoMCP , float &weightMCPtoPFO );
		void TruthRecoComparison::updateKinematics();
};

#endif
