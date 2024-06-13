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

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
enum EVENT_CATEGORY_ZHH: unsigned int {
	NONE = 0,
	OTHER = 1,
	LEPTONIC = 11,
	NEUTRINO = 21,
	HADRONIC = 31
};

enum EVENT_CATEGORY_TRUE: unsigned int {
	OTHER = 0,
	
	// LEPTONIC
	OTHER_LL = 10,
	llHH = 11, // llbbbb (ZHH signal)

	eebb = 12,
	μμbb = 13,
	𝜏𝜏bb = 14,
	llbbbb = 15,
	llqqH = 16,
	ll = 17,

	// NEUTRINO
	OTHER_VV = 20,
	vvHH = 21, // vvbbbb (ZHH signal)

	vvbb = 22,
	vvbbbb = 23,
	vvqqH = 24,
	vv = 25,

	// HADRONIC
	OTHER_QQ = 30,
	qqHH = 31, // qqbbbb (ZHH signal)

	qqqqH = 32,
	qqbbbb = 33,
	bbbb = 34,
	ttZ = 35,
	ttbb = 36,
	qq = 37,
	
	// ttbar -> lvbbqq [t->Wb, W->lv/qq, b->bb]
	// so far not accounted: ttbar -> llvvbb (two leptonically decaying W bosons)
	// reason: https://tikz.net/sm_decay_piechart/
	// W -> qqbar 67%; W -> lv 33%
	// => 2xW -> qqbar 67% * 67% = 44.89% (two hadronic decays)
	// => 2xW -> lv 33% * 33% = 10.89% (two leptonic decays)
	// rest: 44.22% (one hadronic, one leptonic decay)
	OTHER_TTBAR = 40,
	evbbqq = 41,
	μvbbqq = 42,
	𝜏vbbqq = 43,

	// tt/WWZ -> bbqqqq
	// for tt: tt -> bbqqqq : 2x [t->Wb; W->qq]
	// for WWZ: WWZ -> bbqqqq : 2x [W->qq; Z->bb]
	OTHER_FULL_HADRONIC = 50,
	bbcssc = 51,
	bbuddu = 52,
	bbcsdu = 53	
};

// Must match ordering in m_final_state_counts of FinalStateRecorder
std::vector<unsigned int> PDG_NUMBERING {
	1, // d
	2, // u
	3, // s
	4, // c
	5, // b
	6, // t
	11, // e
	12, // ve
	13, // μ
	14, // vμ
	15, // τ
	16, // vτ
	22, // γ
	23, // Z
	24, // W
	25 // h
};

std::map<std::string, std::vector<int>> const FinalStateMap {
	// First digit: number of Higgs bosons (their daughter PDGs are inferred)
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
		int m_nEvtSum;

		int m_errorCode = ERROR_CODES::UNINITIALIZED;
		std::vector<int> m_final_states{};
		std::vector<int> m_final_states_h_decay{};
		std::map<std::string, int> m_final_state_counts { // this must match the order in PDG_NUMBERING
			{ "d", 0 },
			{ "u", 0 },
			{ "s", 0 },
			{ "c", 0 },
			{ "b", 0 },
			{ "t", 0 },
			{ "e", 0 },
			{ "ve", 0 },
			{ "μ", 0 },
			{ "vμ", 0 },
			{ "τ", 0 },
			{ "vτ", 0 },
			{ "γ", 0 },
			{ "Z", 0 },
			{ "W", 0 },
			{ "h", 0 }
		};
		std::string m_final_state_string{};
		
		std::string m_process{};
		int m_event_category{};
		int m_n_higgs{};	


		// Output ROOT file
		TFile *m_pTFile{};
		TTree *m_pTTree{};

		// Output JSON file
		jsonf m_jsonFile{};

};

#endif
