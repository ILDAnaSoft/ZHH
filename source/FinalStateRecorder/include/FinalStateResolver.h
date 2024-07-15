#ifndef FinalStateResolver_h
#define FinalStateResolver_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/MCParticle.h"
#include "lcio.h"
#include "vector"
#include "string"

using namespace lcio;
using namespace marlin;

struct RESOLVER_ERRORS {
    enum Values: unsigned int {
        OK = 0,
        UNKNOWN_ERROR = 5001,
        UNALLOWED_VALUES = 5002,
        UNEXPECTED_CHILDREN = 5020,
        HIGGS_NOT_FOUND = 5030,
    };
};

class FinalStateResolver {
    protected:
        std::string m_process_name;
        int m_process_id;
        int m_event_category;
        int m_n_fermions;
        int m_n_higgs;

        void assert_true(bool check) {
            if (!check) {
                throw RESOLVER_ERRORS::UNKNOWN_ERROR;
            }; };
        void assert_true(bool check, int err) {
            if (!check) {
                throw err;
            }; };

        // Helper functions
        int pdg_of_particle(EVENT::LCObject* particle);
        std::vector<int> pdgs_of_daughter_particles(EVENT::LCObject* particle);
        std::vector<int> pdgs_of_daughter_particles(EVENT::MCParticle* particle);

        std::vector<int> pdgs_of_nth_hadronic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_leptonic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_semilept_decay(LCCollection *mcp_collection, int n);

    public:
        FinalStateResolver(std::string process_name, int process_id, int event_category, int n_fermions, int n_higgs);
        virtual ~FinalStateResolver();

        std::string get_process_name() { return m_process_name; };
        int get_process_id() { return m_process_id; };
        int get_event_category(std::map<int, int> m_final_state_counts) { return m_event_category; };
        int get_n_fermions() { return m_n_fermions; };
        int get_n_higgs() { return m_n_higgs; };
        
        virtual std::vector<int> m_resolve(LCCollection *mcp_collection) = 0;
        //virtual int m_get_category(LCCollection *mcp_collection) = 0;

};

#endif
