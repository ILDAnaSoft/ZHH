#ifndef ZHHPostRecoMEProcessor_h
#define ZHHPostRecoMEProcessor_h 1

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
using namespace lcme;
class ZHHPostRecoMEProcessor : public Processor
{
	public:

		virtual Processor*  newProcessor()
		{
			return new ZHHPostRecoMEProcessor;
		}
		ZHHPostRecoMEProcessor();
		virtual ~ZHHPostRecoMEProcessor() = default;
		ZHHPostRecoMEProcessor(const ZHHPostRecoMEProcessor&) = delete;
		ZHHPostRecoMEProcessor& operator=(const ZHHPostRecoMEProcessor&) = delete;
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
		std::string m_inputLepPairCollection{};
		std::string m_inputHiggsPairCollection{};
		std::string m_inputPreSelectionCollection{};
		std::string m_inputMCTrueCollection{};
		std::string m_outputFile{};

		int m_ZDecayMode{};
		float m_Hmass{};

		TFile *m_pTFile{};
        TTree *m_pTTree{};
		lcme::LCMEZZH *_zzh; // ZZH MEM calculator instance

		int m_nRun;
        int m_nEvt;

		// Refreshed every run
		int m_passed_preselection{};
		int m_h1_decay_pdg{};
		int m_h2_decay_pdg{};

		float m_true_sigma{};
		float m_true_sigmall{};
		float m_true_sigmalr{};
		float m_true_sigmarl{};
		float m_true_sigmarr{};
		
		float m_reco_sigma{};
		float m_reco_sigmall{};
		float m_reco_sigmalr{};
		float m_reco_sigmarl{};
		float m_reco_sigmarr{};

};

#endif
