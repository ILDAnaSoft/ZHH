#ifndef FinalStates_ffh_h
#define FinalStates_ffh_h 1

#include "FinalStates_4p.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffh: public p4 {
    public:
        // Set process ID and event category
        ffh( string process_name, int process_id, int event_category, vector<int> z_decay_filter ):
            p4( process_name, process_id, event_category, 2, 1, vector<int> {6,7}, z_decay_filter ) {};

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs) {
            vector<MCParticle*> fs_particles;

            // Get Z-decayed fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(8));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(9));

            // Get Higgs boson
            MCParticle* h = (MCParticle*)mcp_collection->getElementAt(10);

            if (resolve_higgs) {
                // Get H-decayed fermions
                fs_particles.push_back(h->getDaughters()[0]);
                fs_particles.push_back(h->getDaughters()[1]);
            } else {
                fs_particles.push_back(h);
            }
        
            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection, false);

            assert_true(fs_particles.size() == 3, RESOLVER_ERRORS::UNEXPECTED_SIZE);

            assert_true(
                vec_contains(m_final_state_filter, abs(fs_particles[0]->getPDG())) &&
                vec_contains(m_final_state_filter, abs(fs_particles[1]->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            // Get H-decayed fermions
            assert_true(fs_particles[4]->getPDG() == 25, RESOLVER_ERRORS::HIGGS_NOT_FOUND);

            vector<int> d1 = pdgs_of_daughter_particles(fs_particles[4]);

            assert_true(d1.size() == 2, RESOLVER_ERRORS::UNEXPECTED_CHILDREN);

            m_n_b_from_higgs = count(d1.begin(), d1.end(), 5);
            m_n_c_from_higgs = count(d1.begin(), d1.end(), 4);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                fs_particles[2]->getPDG(),
                fs_particles[3]->getPDG(),
                d1[0],
                d1[1]
            };
        };

};


class e1e1h : public ffh {
    public: e1e1h(): ffh( "e1e1h", PROCESS_ID::e1e1h, EVENT_CATEGORY_TRUE::llH, vector{11} ) {}; };

class e2e2h : public ffh {
    public: e2e2h(): ffh( "e2e2h", PROCESS_ID::e2e2h, EVENT_CATEGORY_TRUE::llH, vector{13} ) {}; };

class e3e3h : public ffh {
    public: e3e3h(): ffh( "e3e3h", PROCESS_ID::e3e3h, EVENT_CATEGORY_TRUE::llH, vector{15} ) {}; };

class qqh : public ffh {
    public: qqh(): ffh( "qqh", PROCESS_ID::qqh, EVENT_CATEGORY_TRUE::qqH, vector{1,2,3,4,5,6} ) {}; };

class n1n1h : public ffh {
    public: n1n1h(): ffh( "n1n1h", PROCESS_ID::n1n1h, EVENT_CATEGORY_TRUE::vvH, vector{12} ) {}; };

class n23n23h : public ffh {
    public: n23n23h(): ffh( "n23n23h", PROCESS_ID::n23n23h, EVENT_CATEGORY_TRUE::vvH, vector{14,16} ) {}; };

#endif
