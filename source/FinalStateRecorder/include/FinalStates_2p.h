#ifndef FinalStates_ff_h
#define FinalStates_ff_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class p2: public FinalStateResolver {
    protected:
        vector<int> m_final_state_filter{};
        unsigned short F1_IDX = 6;
        unsigned short F2_IDX = 7;

    public:
        // Set process ID and event category
        p2( string process_name, int process_id, int event_category, vector<int> final_state_filter ): FinalStateResolver( process_name, process_id, event_category, 2, 0, vector<int> {4,5} ) {
            m_final_state_filter = final_state_filter;
        };

        vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) {
            return vector<int>{ F1_IDX, F2_IDX };
        }

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_higgs;

            vector<MCParticle*> fs_particles;

            // Get fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F1_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F2_IDX));

            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection, false);

            assert_true(
                vec_contains(m_final_state_filter, abs(fs_particles[0]->getPDG())) &&
                vec_contains(m_final_state_filter, abs(fs_particles[1]->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
            };
        };

        int get_event_category(std::map<int, int> m_final_state_counts) {
            (void)m_final_state_counts;
            
            return m_event_category;
        };
};


class ll : public p2 {
    public: ll(): p2( "2f_z_l", PROCESS_ID::f2_z_l, EVENT_CATEGORY_TRUE::ll, vector{11,13,15} ) {}; };

class qq : public p2 {
    public: qq(): p2( "2f_z_h", PROCESS_ID::f2_z_h, EVENT_CATEGORY_TRUE::qq, vector{1,2,3,4,5,6} ) {}; };

class vv : public p2 {
    public: vv(): p2( "2f_z_nung", PROCESS_ID::f2_z_nung, EVENT_CATEGORY_TRUE::vv, vector{12,14,16} ) {}; };

class ee1 : public p2 {
    public: ee1(): p2( "2f_z_bhabhag", PROCESS_ID::f2_z_bhabhag, EVENT_CATEGORY_TRUE::ll, vector{11} ) {}; };

class ee2 : public p2 {
    public: ee2(): p2( "2f_z_bhabhang", PROCESS_ID::f2_z_bhabhagg, EVENT_CATEGORY_TRUE::ll, vector{11} ) {}; };

#endif
