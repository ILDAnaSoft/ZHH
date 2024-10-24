#ifndef ChargedPFOCorrection_h
#define ChargedPFOCorrection_h 1
#include <marlin/Processor.h>
#include <marlin/Global.h>
#include "UTIL/LCRelationNavigator.h"
#include "UTIL/LCIterator.h"
#include "UTIL/Operators.h"
#include <UTIL/BitField64.h>
#include "UTIL/LCTrackerConf.h"
#include <UTIL/ILDConf.h>
#include <UTIL/BitSet32.h>
#include "EVENT/LCStrVec.h"
#include <EVENT/MCParticle.h>
#include <EVENT/Track.h>
#include "IMPL/LCCollectionVec.h"
#include <IMPL/ReconstructedParticleImpl.h>
#include "DDMarlinCED.h"
#include "lcio.h"
#include <string>
#include <vector>
#include <math.h>
#include <set>
#include <vector>
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TMatrixD.h"
#include "TStyle.h"
#include "TCanvas.h"

class TFile;
class TDirectory;
class TH1F;
class TH1I;
class TH2I;
class TH2F;
class TTree;
class TF1;

using namespace lcio ;
using namespace marlin ;

class ChargedPFOCorrection : public Processor
{
	public:

		virtual Processor*  newProcessor()
		{
			return new ChargedPFOCorrection;
		}
		ChargedPFOCorrection();
		virtual ~ChargedPFOCorrection() = default;
		ChargedPFOCorrection(const ChargedPFOCorrection&) = delete;
		ChargedPFOCorrection& operator=(const ChargedPFOCorrection&) = delete;
		virtual void init();
		virtual void Clear();
		virtual void processRunHeader();
		virtual void processEvent( EVENT::LCEvent *pLCEvent );
		int getTruthTrkID( EVENT::LCEvent *pLCEvent , EVENT::Track* inputTrk );
		int getTrackIndex( EVENT::LCCollection *TrackCollection , EVENT::Track* inputTrk );
		TLorentzVector getTrackFourMomentum( EVENT::Track* inputTrk , double trackMass );
		std::vector<float> getChargedPFOCovMat( EVENT::Track* inputTrk , float trackMass );
		virtual void check( EVENT::LCEvent *pLCEvent );
		virtual void end();

	private:

		typedef std::vector<int>		IntVector;
		typedef std::vector<double>		DoubleVector;
		typedef std::vector<float>		FloatVector;

		std::string				m_inputPfoCollection{};
		std::string				m_MarlinTrkTracks{};
		std::string				m_MarlinTrkTracksKAON{};
		std::string				m_MarlinTrkTracksPROTON{};
		std::string				m_TrackMCTruthLinkCollection{};
		std::string				m_MCTruthTrackLinkCollection{};
		std::string				m_outputPfoCollection{};

		bool					m_updatePFOs = true;
		bool					m_updatePFOwithOneTrack = true;
		bool					m_updatePFOwithTwoTrack = true;
		bool					m_updatePFOwithMoreTrack = true;
		bool					m_updatePFOwithPionTrack = false;
		int					m_mainTrackSelectionScenario = 1;

		float					m_Bfield;
		double					c;
		double					mm2m;
		double					eV2GeV;
		double					eB;
		float					m_MinWeightTrackMCTruthLink;
		float					m_MinWeightMCTruthTrackLink;
		float					m_pion_mass;
		float					m_proton_mass;
		float					m_kaon_mass;
};

#endif
