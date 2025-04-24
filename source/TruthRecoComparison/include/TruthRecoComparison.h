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
#include <map>
#include "Math/Vector4D.h"
#include "UTIL/LCRelationNavigator.h"
#include "EVENT/ReconstructedParticle.h"
#include "EVENT/MCParticle.h"

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
		ROOT::Math::PxPyPzEVector m_mcp_mom_tot{0., 0., 0., 0.};
		ROOT::Math::PxPyPzEVector m_mcp_mom_detected{0., 0., 0., 0.};
		ROOT::Math::PxPyPzEVector m_mcp_mom_undetected{0., 0., 0., 0.};
		ROOT::Math::PxPyPzEVector m_pfo_mom_tot{0.,0.,0.,0.};

		float m_true_Et{};
		float m_reco_Et{};

		float m_true_pt_miss{};
		float m_reco_pt_miss{};

		float m_true_m_miss{};
		float m_reco_m_miss{};

		float m_true_E_miss{};
		float m_reco_E_miss{};

		float m_true_E_vis{};
		float m_reco_E_vis{};

		std::vector<float> m_reco_jet_pt{};
		std::vector<float> m_reco_jet_E{};

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
		std::string m_inputJetCollection{};
		std::string m_recoMCTruthLink{};
		std::string m_mcTruthRecoLink{};
		std::string m_mcParticleCollection{};
		std::string m_outputRootFile{};

		float m_ECM{};

		int m_n_run;
		int m_n_evt = 0;
		int m_error_code;

		// Output ROOT file
		TFile *m_pTFile{};
		TTree *m_pTTree = new TTree("TruthRecoComparison", "TruthRecoComparison");;

		// Truth kinematics
		EVENT::ReconstructedParticle* getLinkedPFO( EVENT::MCParticle *mcParticle , LCRelationNavigator RecoMCParticleNav , LCRelationNavigator MCParticleRecoNav , bool getChargedPFO , bool getNeutralPFO , float &weightPFOtoMCP , float &weightMCPtoPFO );
		void updateKinematics();

		// charged Kaons K+, charged Pions, Protons, neutral pions, Neutrons, Electron, Muons
		std::vector<int> m_species_abs_pdgs = {321, 211, 2212, 111, 2112, 11, 13};
		std::vector<std::string> m_species_labels = { "charged Kaons K+", "charged Pions", "Protons", "neutral Pions", "Neutrons", "Electron", "Muons" }; 
		std::vector<std::string> m_species_names = { "ch_kaon", "ch_pion", "proton", "neut_pion", "neutron", "electron", "muon" }; 

		// These have to match each other
		std::vector<std::string> m_species_features = { "E", "theta", "phi", "pt" };
		void extract_true_particle_features(std::vector<EVENT::MCParticle*> mcparticles, std::vector<std::vector<float>> &feature_vec);
		void extract_reco_particle_features(std::vector<EVENT::ReconstructedParticle*> reco_particles, std::vector<std::vector<float>> &feature_vec);

		// Features are stored in [true, reco] alternating order for each feature
		// Even indices are truth, odd are reco
		std::map<int, std::vector<std::vector<float>>> m_species {};

		std::map<int, std::vector<MCParticle*>> m_species_particles_true {};
		std::map<int, std::vector<ReconstructedParticle*>> m_species_particles_reco {};
};

#endif
