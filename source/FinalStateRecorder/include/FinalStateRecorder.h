#ifndef FinalStateRecorder_h
#define FinalStateRecorder_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include "TLorentzVector.h"
#include "nlohmann/json.hpp"

class TFile;
class TTree;

using namespace lcio ;
using namespace marlin ;
using jsonf = nlohmann::json;

enum ERROR_CODES: unsigned int {
	UNINITIALIZED = 9999,
	OK = 0,
	COLLECTION_NOT_FOUND = 1,
	PROCESS_NOT_FOUND = 2
};

// If the final state is a ZHH, the channel is given by the decay channel of the Z boson (else NONE)
enum ZHH_CHANNEL: unsigned int {
	NONE = 0,
	LEPTONIC = 1,
	NEUTRINO = 2,
	HADRONIC = 3
};

std::map<std::string, std::vector<int>> const FinalStateMap {
	// First digit: number of MCParticles at the end of which to infer the daughter PDGs
    // hh / sig: ZHH
    { "e1e1hh", { 2, 8, 9, 10, 11 }}, // e- e+ h h
    { "e2e2hh", { 2, 8, 9, 10, 11 }}, // mu- mu+ h h
    { "e3e3hh", { 2, 8, 9, 10, 11 }}, // tau- tau+
    { "n1n1hh", { 2, 8, 9, 10, 11 }}, // nue anti-nue h h
    { "n23n23hh", { 2, 8, 9, 10, 11 }}, // nu(mu/tau) anti-nu(mu/tau) hh
    { "qqhh", { 2, 8, 9, 10, 11 }},
    
    // hh / bkg: ZZH
    { "e1e1qqh", { 1, 8, 9, 10, 11, 12 }}, // e- e+ q q h
    { "e2e2qqh", { 1, 8, 9, 10, 11, 12 }}, // mu- mu+ q q h
    { "e3e3qqh", { 1, 8, 9, 10, 11, 12 }}, // tau- tau+ q q h
    { "n1n1qqh", { 1, 8, 9, 10, 11, 12 }}, // nue anti-nue q q h
    { "n23n23qqh", { 1, 8, 9, 10, 11, 12 }}, // nu(mu/tau) anti-nu(mu/tau) q q h    
    
    { "qqqqh", { 1, 8, 9, 10, 11, 12 }},
    
    // 2f_Z_hadronic
    { "z_h0", { 0, 9, 10 }} // z(8) f f | processName: z_h0 
};


class FinalStateRecorder : public Processor
{
	public:

		virtual Processor*  newProcessor()
		{
			return new FinalStateRecorder;
		}
		FinalStateRecorder();
		virtual ~FinalStateRecorder() = default;
		FinalStateRecorder(const FinalStateRecorder&) = delete;
		FinalStateRecorder& operator=(const FinalStateRecorder&) = delete;
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
		std::string m_mcParticleCollection{};
		std::string m_outputJsonFile{};
		std::string m_outputRootFile{};

		int m_nRun;
		int m_nEvt;

		int m_errorCode = ERROR_CODES::UNINITIALIZED;
		std::vector<int> m_final_states{};
		std::string m_final_state_string{};
		std::string m_process{};
		int m_zhh_channel{};
		int m_n_higgs{};
		
		

		

		// Output ROOT file
		TFile *m_pTFile{};
		TTree *m_pTTree{};

		// Output JSON file
		jsonf m_jsonFile{};

};

#endif
