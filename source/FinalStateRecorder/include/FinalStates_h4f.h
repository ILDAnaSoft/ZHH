#ifndef FinalStates_ffffh_h
#define FinalStates_ffffh_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffffh: public p6 {
    protected:
        vector<int> m_z_decay_filter;

        const unsigned short F1_IDX = 8;
        const unsigned short F2_IDX = 9;
        const unsigned short F3_IDX = 10;
        const unsigned short F4_IDX = 11;
        const unsigned short HIGGS_IDX = 12;

    public:
        // Set process ID and event category
        ffffh( string process_name, int process_id, int event_category, vector<int> z_decay_filter ):
            p6( process_name, process_id, event_category, 4, 1, vector<int> {6,7} ),
            m_z_decay_filter { z_decay_filter } {};

        vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) {
            assert(!resolve_higgs);

            return vector<int>{ F1_IDX, F2_IDX, F3_IDX, F4_IDX, HIGGS_IDX};
        }

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs) {
            vector<MCParticle*> fs_particles;

            // Get Z-decayed fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F1_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F2_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F3_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F4_IDX));

            // Get Higgs boson
            MCParticle* h = (MCParticle*)mcp_collection->getElementAt(HIGGS_IDX);

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

            assert_true(fs_particles.size() == 5, RESOLVER_ERRORS::UNEXPECTED_SIZE);

            assert_true(
                vec_contains(m_z_decay_filter, abs(fs_particles[0]->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(fs_particles[1]->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(fs_particles[2]->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(fs_particles[3]->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

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


class e1e1qqh : public ffffh {
    public: e1e1qqh(): ffffh( "e1e1qqh", PROCESS_ID::e1e1qqh, EVENT_CATEGORY_TRUE::llqqH, vector{11,1,2,3,4,5,6} ) {}; };

class e2e2qqh : public ffffh {
    public: e2e2qqh(): ffffh( "e2e2qqh", PROCESS_ID::e2e2qqh, EVENT_CATEGORY_TRUE::llqqH, vector{13,1,2,3,4,5,6} ) {}; };

class e3e3qqh : public ffffh {
    public: e3e3qqh(): ffffh( "e3e3qqh", PROCESS_ID::e3e3qqh, EVENT_CATEGORY_TRUE::llqqH, vector{15,1,2,3,4,5,6} ) {}; };

class qqqqh : public ffffh {
    public: qqqqh(): ffffh( "qqqqh", PROCESS_ID::qqqqh, EVENT_CATEGORY_TRUE::qqqqH, vector{1,2,3,4,5,6} ) {}; };

class n1n1qqh : public ffffh {
    public: n1n1qqh(): ffffh( "n1n1qqh", PROCESS_ID::n1n1qqh, EVENT_CATEGORY_TRUE::vvqqH, vector{12,1,2,3,4,5,6} ) {}; };

class n23n23qqh : public ffffh {
    public: n23n23qqh(): ffffh( "n23n23qqh", PROCESS_ID::n23n23qqh, EVENT_CATEGORY_TRUE::vvqqH, vector{14,16,1,2,3,4,5,6} ) {}; };

#endif
