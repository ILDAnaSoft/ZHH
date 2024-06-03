#ifndef FinalStateRecorder_h
#define FinalStateRecorder_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include <string>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include "TLorentzVector.h"
class TFile;
class TTree;

using namespace lcio ;
using namespace marlin ;

enum ERROR_CODES: unsigned int {
	OK = 0,
	COLLECTION_NOT_FOUND = 1,
	PROCESS_NOT_FOUND = 2
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
    { "ff", { 0, 9, 10 }} // z(8) f f | processName: z_h0 
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
		std::string m_outputFile{};

		int m_nRun;
		int m_nEvt;
		std::string m_process{};
		std::vector<int> m_final_states;

		int m_errorCode;

		TFile *m_pTFile{};        
		TTree *m_pTTree{};

};

#endif
