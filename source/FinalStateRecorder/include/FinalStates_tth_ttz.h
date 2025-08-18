#ifndef FinalStates_ptth_h
#define FinalStates_ptth_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ptth_ttz: public FinalStateResolver {
    protected:
        vector<int> m_final_state_filter;

        // either Higgs or Z
        unsigned short ON_SHELL_BOSON_IDX = 10;

        unsigned short TOP1_IDX = 8;
        unsigned short TOP2_IDX = 9;

    public:
        // Set process ID and event category
        // Try to always resolve the top quark decay
        ptth_ttz( string process_name, int process_id, int event_category, bool is_tth, vector<int> decay_filter ):
            FinalStateResolver( process_name, process_id, event_category, 6, is_tth ? 1 : 0, vector<int> {6, 7} ),
            m_final_state_filter { decay_filter } {};
            
        vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) mcp_collection;

            vector<int> result;
            vector<MCParticle*> mcparticles;

            for (int i = 0; i < mcp_collection->getNumberOfElements(); i++) {
                MCParticle* mcpart = (MCParticle*)mcp_collection->getElementAt(i);

                if (m_n_higgs > 0 && resolve_higgs && mcpart->getPDG() == 25) {
                    mcparticles.push_back(mcpart->getDaughters()[0]);
                    mcparticles.push_back(mcpart->getDaughters()[1]);
                } else
                    mcparticles.push_back(mcpart);
            }
            
            for (MCParticle* mcpart: resolve_fs_particles(mcp_collection, false)) {
                int idx = find(mcparticles.begin(), mcparticles.end(), mcpart) - mcparticles.begin();
                assert(idx < mcparticles.size());

                result.push_back(idx);
            }

            return result;
        }

        void resolve_top_decay(MCParticle* input, vector<MCParticle*> &dec_particles) {
            vector<MCParticle*> daughters = input->getDaughters();

            for (MCParticle* daughter: daughters) {
                if (abs(daughter->getPDG()) == 24) {
                    vector<MCParticle*> Wdaughters = daughter->getDaughters();

                    for (MCParticle* Wdaughter: Wdaughters) {
                        if (vec_contains(m_final_state_filter, abs(Wdaughter->getPDG()))) {
                            dec_particles.push_back(Wdaughter);
                        }
                    }
                } else if (vec_contains(m_final_state_filter, abs(daughter->getPDG()))) {
                    dec_particles.push_back(daughter);
                }
            }
        }

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            vector<MCParticle*> fs_particles;
            vector<MCParticle*> top1_dec_particles;
            vector<MCParticle*> top2_dec_particles;

            resolve_top_decay((MCParticle*)mcp_collection->getElementAt(TOP1_IDX), top1_dec_particles);
            resolve_top_decay((MCParticle*)mcp_collection->getElementAt(TOP2_IDX), top2_dec_particles);

            fs_particles.insert(fs_particles.end(), top1_dec_particles.begin(), top1_dec_particles.end());
            fs_particles.insert(fs_particles.end(), top2_dec_particles.begin(), top2_dec_particles.end());

            MCParticle* boson = (MCParticle*)mcp_collection->getElementAt(ON_SHELL_BOSON_IDX);

            if (m_n_higgs > 0 && resolve_higgs) {
                assert(boson->getPDG() == 25 && boson->getDaughters().size() == 2);

                fs_particles.push_back((MCParticle*)boson->getDaughters()[0]);
                fs_particles.push_back((MCParticle*)boson->getDaughters()[1]);
            } else
                fs_particles.push_back(boson);

            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection);

            vector<int> ids = resolve_fs_particle_indices(mcp_collection);

            assert_true(fs_particles.back()->getPDG() == (m_n_higgs > 0 ? 25 : 23), RESOLVER_ERRORS::UNALLOWED_VALUES);
            assert_true(fs_particles.size() == 7, RESOLVER_ERRORS::UNEXPECTED_CHILDREN);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                fs_particles[2]->getPDG(),
                fs_particles[3]->getPDG(),
                fs_particles[4]->getPDG(),
                fs_particles[5]->getPDG(),
                fs_particles[6]->getPDG()
            };
        };
};

class ptth : public ptth_ttz {
    public: ptth(): ptth_ttz( "tth", PROCESS_ID::f8_tth, EVENT_CATEGORY_TRUE::ttH, true, vector{1,2,3,4,5,11,12,13,14,15,16} ) {}; };

class pttz : public ptth_ttz {
    public: pttz(): ptth_ttz( "ttz", PROCESS_ID::f8_ttz, EVENT_CATEGORY_TRUE::ttZ, false, vector{1,2,3,4,5,11,12,13,14,15,16} ) {}; };
    
#endif
