#ifndef FinalStates_ffhh_h
#define FinalStates_ffhh_h 1

#include "FinalStates_6p.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffhh: public p6 {
    protected:
        vector<int> m_z_decay_filter;

    public:
        // Set process ID and event category
        ffhh( string process_name, int process_id, int event_category, vector<int> z_decay_filter ):
            p6( process_name, process_id, event_category, 2, 2, vector<int> {6,7} ),
            m_z_decay_filter { z_decay_filter } {};

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs) {
            vector<MCParticle*> fs_particles;

            // Get Z-decayed fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(8));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(9));

            // Get Higgs bosons
            MCParticle* h1 = (MCParticle*)mcp_collection->getElementAt(10);
            MCParticle* h2 = (MCParticle*)mcp_collection->getElementAt(11);

            if (resolve_higgs) {
                // Get H-decayed fermions
                fs_particles.push_back(h1->getDaughters()[0]);
                fs_particles.push_back(h1->getDaughters()[1]);

                fs_particles.push_back(h2->getDaughters()[0]);
                fs_particles.push_back(h2->getDaughters()[1]);
            } else {
                fs_particles.push_back(h1);
                fs_particles.push_back(h2);
            }
        
            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection, false);

            assert_true(fs_particles.size() == 4, RESOLVER_ERRORS::UNEXPECTED_SIZE);

            assert_true(
                vec_contains(m_z_decay_filter, abs(fs_particles[0]->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(fs_particles[1]->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            assert_true(fs_particles[2]->getPDG() == 25 && fs_particles[3]->getPDG() == 25, RESOLVER_ERRORS::HIGGS_NOT_FOUND);

            vector<int> d1 = pdgs_of_daughter_particles(fs_particles[2]);
            vector<int> d2 = pdgs_of_daughter_particles(fs_particles[3]);

            assert_true(d1.size() == 2 && d2.size() == 2, RESOLVER_ERRORS::UNEXPECTED_CHILDREN);   

            m_n_b_from_higgs = count(d1.begin(), d1.end(), 5) + count(d2.begin(), d2.end(), 5);        
            m_n_c_from_higgs = count(d1.begin(), d1.end(), 4) + count(d2.begin(), d2.end(), 4);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                d1[0],
                d1[1],
                d2[0],
                d2[1]
            };
        };

        virtual int get_event_category(std::map<int, int> m_final_state_counts) {
            // Takes priority over p6::get_event_category
            (void) m_final_state_counts;
            
            return m_event_category;
        }
};


class e1e1hh : public ffhh {
    public: e1e1hh(): ffhh( "e1e1hh", PROCESS_ID::e1e1hh, EVENT_CATEGORY_TRUE::llHH, vector{11} ) {}; };

class e2e2hh : public ffhh {
    public: e2e2hh(): ffhh( "e2e2hh", PROCESS_ID::e2e2hh, EVENT_CATEGORY_TRUE::llHH, vector{13} ) {}; };

class e3e3hh : public ffhh {
    public: e3e3hh(): ffhh( "e3e3hh", PROCESS_ID::e3e3hh, EVENT_CATEGORY_TRUE::llHH, vector{15} ) {}; };

class qqhh : public ffhh {
    public: qqhh(): ffhh( "qqhh", PROCESS_ID::qqhh, EVENT_CATEGORY_TRUE::qqHH, vector{1,2,3,4,5,6} ) {}; };

class n1n1hh : public ffhh {
    public: n1n1hh(): ffhh( "n1n1hh", PROCESS_ID::n1n1hh, EVENT_CATEGORY_TRUE::vvHH, vector{12} ) {}; };

class n23n23hh : public ffhh {
    public: n23n23hh(): ffhh( "n23n23hh", PROCESS_ID::n23n23hh, EVENT_CATEGORY_TRUE::vvHH, vector{14, 16} ) {}; };


#endif



