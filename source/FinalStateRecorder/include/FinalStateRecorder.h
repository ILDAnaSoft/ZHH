#ifndef FinalStateRecorder_h
#define FinalStateRecorder_h 1

#include "marlin/Processor.h"
#include "lcio.h"
#include <string>
#include "IMPL/LCCollectionVec.h"
#include <fstream>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include "nlohmann/json.hpp"
#include "FinalStateResolver.h"
#include "TLorentzVector.h"
#include <sstream>

class TFile;
class TTree;

using namespace lcio;
using namespace marlin;
using jsonf = nlohmann::json;

struct ERROR_CODES {
	enum Values: int {
		UNINITIALIZED = -1,
		OK = 0,
		COLLECTION_NOT_FOUND = 6001,
		PROCESS_NOT_FOUND = 6002,
		UNKNOWN_ERROR = 6003,
		UNALLOWED_VALUES = 6004,
	};
};


class FinalStateRecorder : public Processor
{
	private:
		void register_process(FinalStateResolver* resolver) { resolvers[resolver->get_process_name()] = resolver;  };
		std::map<std::string, FinalStateResolver*> resolvers{};

		float m_beam_pol1{};
		float m_beam_pol2{};
		float m_cross_section{};
		float m_cross_section_err{};
		float m_event_weight{};
		int m_process_id{};
		std::time_t m_t_start{};
		std::string m_process_name{};

		// filter mechanism
		std::vector<std::pair<int*, int>> construct_filter_lookup(std::vector<std::string> filter);
		std::vector<std::pair<int*, int>> m_filter_lookup{};
		std::vector<std::string> m_filter_quantities{};
		std::vector<int> m_filter_operators{}; // 0,1,2,3,4 for "=", "<", ">", "<=", ">="
		std::vector<std::string> m_filter_operator_names{}; // any of "=", "<", ">", "<=", ">=" for all selected filters
		bool process_filter();
		bool m_passed_filter{};

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
		std::string m_mcParticleCollection{};
		std::string m_outputJsonFile{};
		std::string m_outputRootFile{};
		std::vector<std::string> m_eventFilter{};
		bool m_setReturnValues{};

		int m_n_run;
		int m_n_evt = 0;
		int m_n_evt_sum = 0;
		int m_error_code;
		
		std::vector<int> m_final_states{};
		std::map<int, int> m_final_state_counts {
			{ 1, 0 }, // d
			{ 2, 0 }, // u
			{ 3, 0 }, // s
			{ 4, 0 }, // c
			{ 5, 0 }, // b
			{ 6, 0 }, // t
			{ 11, 0 }, // e
			{ 12, 0 }, // ve
			{ 13, 0 }, // µ
			{ 14, 0 }, // vµ
			{ 15, 0 }, // tau
			{ 16, 0 }, // vtau
			{ 21, 0 }, // g
			{ 22, 0 }, // γ
			{ 23, 0 }, // Z
			{ 24, 0 }, // W
			{ 25, 0 } // h
		};

		int m_process{};
		int m_event_category{};
		int m_event_category_zhh{};
		int m_n_fermion{};
		int m_n_higgs{};
		int m_n_b_from_higgs{};
		int m_n_c_from_higgs{};

		// Output ROOT file
		bool m_write_ttree{};
		TFile *m_pTFile{};
		TTree *m_pTTree = new TTree("FinalStates", "FinalStates");

		// Output JSON file
		jsonf m_jsonFile{};
};

#endif
