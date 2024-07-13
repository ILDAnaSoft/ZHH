#ifndef FinalStates_ff_h
#define FinalStates_ff_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ff: public FinalStateResolver {
    protected:
        vector<int> m_final_state_filter{};

    public:
        // Set process ID and event category
        ff( string process_name, int process_id, int event_category, vector<int> final_state_filter ): FinalStateResolver( process_name, process_id, event_category, 2, 0 ) {
            m_final_state_filter = final_state_filter;
        };

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get final state fermions
            MCParticle* f1 = (MCParticle*)mcp_collection->getElementAt(6);
            MCParticle* f2 = (MCParticle*)mcp_collection->getElementAt(7);

            assert_true(
                vec_contains(m_final_state_filter, abs(f1->getPDG())) &&
                vec_contains(m_final_state_filter, abs(f2->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                f1->getPDG(),
                f2->getPDG(),
            };
        };

};


class ll : public ff {
    public: ll(): ff( "2f_z_l", PROCESS_ID::f2_z_l, EVENT_CATEGORY_TRUE::ll, vector{11,13,15} ) {}; };

class qq : public ff {
    public: qq(): ff( "2f_z_h", PROCESS_ID::f2_z_h, EVENT_CATEGORY_TRUE::qq, vector{1,2,3,4,5} ) {}; };

class vv : public ff {
    public: vv(): ff( "2f_z_nung", PROCESS_ID::f2_z_nung, EVENT_CATEGORY_TRUE::vv, vector{12,14,16} ) {}; };

class ee1 : public ff {
    public: ee1(): ff( "2f_z_bhabhag", PROCESS_ID::f2_z_bhabhag, EVENT_CATEGORY_TRUE::ll, vector{11} ) {}; };

class ee2 : public ff {
    public: ee2(): ff( "2f_z_bhabhang", PROCESS_ID::f2_z_bhabhagg, EVENT_CATEGORY_TRUE::ll, vector{11} ) {}; };

#endif
