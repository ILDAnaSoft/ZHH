#ifndef FinalStates_p5_h
#define FinalStates_p5_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class p5: public FinalStateResolver {
    protected:
        unsigned short F1_IDX = 6;
        unsigned short F2_IDX = 7;
        unsigned short F3_IDX = 8;
        unsigned short F4_IDX = 9;
        unsigned short F5_IDX = 10;

    public:
        // Set process ID and event category
        p5( string process_name, int process_id, int event_category): FinalStateResolver( process_name, process_id, event_category, 5, 0, vector<int>{5} ) {};
        p5( string process_name, int process_id, int event_category, int n_fermions, int n_higgs ): FinalStateResolver( process_name, process_id, event_category, n_fermions, n_higgs, vector<int>{5} ) {};

        vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) {
            return vector<int>{ F1_IDX, F2_IDX, F3_IDX, F4_IDX, F5_IDX };
        }

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_higgs;

            vector<MCParticle*> fs_particles;

            // Get fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F1_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F2_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F3_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F4_IDX));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(F5_IDX));

            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection);

            assert_true(
                abs(fs_particles[0]->getPDG()) < 17 &&
                abs(fs_particles[1]->getPDG()) < 17 &&
                abs(fs_particles[2]->getPDG()) < 17 &&
                abs(fs_particles[3]->getPDG()) < 17 &&
                abs(fs_particles[4]->getPDG()) < 17, RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                fs_particles[2]->getPDG(),
                fs_particles[3]->getPDG(),
                fs_particles[4]->getPDG(),
            };
        };

        int get_event_category(std::map<int, int> m_final_state_counts) {
            (void)m_final_state_counts;
            
            return m_event_category;
        };

};

class p5_ae_eeevv : public p5 {
    public: p5_ae_eeevv(): p5( "ae_eeevv", PROCESS_ID::p5_ae_eeevv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eyyyy : public p5 {
    public: p5_ea_eyyyy(): p5( "ea_eyyyy", PROCESS_ID::p5_ea_eyyyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_elevv : public p5 {
    public: p5_ae_elevv(): p5( "ae_elevv", PROCESS_ID::p5_ae_elevv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eyyyy : public p5 {
    public: p5_ae_eyyyy(): p5( "ae_eyyyy", PROCESS_ID::p5_ae_eyyyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_exxxx : public p5 {
    public: p5_ea_exxxx(): p5( "ea_exxxx", PROCESS_ID::p5_ea_exxxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_evvxx : public p5 {
    public: p5_ae_evvxx(): p5( "ae_evvxx", PROCESS_ID::p5_ae_evvxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eeeyy : public p5 {
    public: p5_ae_eeeyy(): p5( "ae_eeeyy", PROCESS_ID::p5_ae_eeeyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eevxy : public p5 {
    public: p5_ae_eevxy(): p5( "ae_eevxy", PROCESS_ID::p5_ae_eevxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_lvvyy : public p5 {
    public: p5_ae_lvvyy(): p5( "ae_lvvyy", PROCESS_ID::p5_ae_lvvyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eeevv : public p5 {
    public: p5_ea_eeevv(): p5( "ea_eeevv", PROCESS_ID::p5_ea_eeevv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_ellxx : public p5 {
    public: p5_ea_ellxx(): p5( "ea_ellxx", PROCESS_ID::p5_ea_ellxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eeeee : public p5 {
    public: p5_ae_eeeee(): p5( "ae_eeeee", PROCESS_ID::p5_ae_eeeee, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_elvxy : public p5 {
    public: p5_ea_elvxy(): p5( "ea_elvxy", PROCESS_ID::p5_ea_elvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_evvyy : public p5 {
    public: p5_ea_evvyy(): p5( "ea_evvyy", PROCESS_ID::p5_ea_evvyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_evlxy : public p5 {
    public: p5_ea_evlxy(): p5( "ea_evlxy", PROCESS_ID::p5_ea_evlxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_ellvv : public p5 {
    public: p5_ae_ellvv(): p5( "ae_ellvv", PROCESS_ID::p5_ae_ellvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_vxyyy : public p5 {
    public: p5_ea_vxyyy(): p5( "ea_vxyyy", PROCESS_ID::p5_ea_vxyyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eeexx : public p5 {
    public: p5_ea_eeexx(): p5( "ea_eeexx", PROCESS_ID::p5_ea_eeexx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_ellll : public p5 {
    public: p5_ae_ellll(): p5( "ae_ellll", PROCESS_ID::p5_ae_ellll, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eeell : public p5 {
    public: p5_ae_eeell(): p5( "ae_eeell", PROCESS_ID::p5_ae_eeell, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_ellxx : public p5 {
    public: p5_ae_ellxx(): p5( "ae_ellxx", PROCESS_ID::p5_ae_ellxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_elvxy : public p5 {
    public: p5_ae_elvxy(): p5( "ae_elvxy", PROCESS_ID::p5_ae_elvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_vxxxy : public p5 {
    public: p5_ae_vxxxy(): p5( "ae_vxxxy", PROCESS_ID::p5_ae_vxxxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_exxxx : public p5 {
    public: p5_ae_exxxx(): p5( "ae_exxxx", PROCESS_ID::p5_ae_exxxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eelvv : public p5 {
    public: p5_ea_eelvv(): p5( "ea_eelvv", PROCESS_ID::p5_ea_eelvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eeeee : public p5 {
    public: p5_ea_eeeee(): p5( "ea_eeeee", PROCESS_ID::p5_ea_eeeee, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_evvvv : public p5 {
    public: p5_ae_evvvv(): p5( "ae_evvvv", PROCESS_ID::p5_ae_evvvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_lvvyy : public p5 {
    public: p5_ea_lvvyy(): p5( "ea_lvvyy", PROCESS_ID::p5_ea_lvvyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_evvyy : public p5 {
    public: p5_ae_evvyy(): p5( "ae_evvyy", PROCESS_ID::p5_ae_evvyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_exxyy : public p5 {
    public: p5_ea_exxyy(): p5( "ea_exxyy", PROCESS_ID::p5_ea_exxyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eevxy : public p5 {
    public: p5_ea_eevxy(): p5( "ea_eevxy", PROCESS_ID::p5_ea_eevxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eeeyy : public p5 {
    public: p5_ea_eeeyy(): p5( "ea_eeeyy", PROCESS_ID::p5_ea_eeeyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_ellyy : public p5 {
    public: p5_ea_ellyy(): p5( "ea_ellyy", PROCESS_ID::p5_ea_ellyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_vxxxy : public p5 {
    public: p5_ea_vxxxy(): p5( "ea_vxxxy", PROCESS_ID::p5_ea_vxxxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_vvvxy : public p5 {
    public: p5_ae_vvvxy(): p5( "ae_vvvxy", PROCESS_ID::p5_ae_vvvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_vvvxy : public p5 {
    public: p5_ea_vvvxy(): p5( "ea_vvvxy", PROCESS_ID::p5_ea_vvvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_ellyy : public p5 {
    public: p5_ae_ellyy(): p5( "ae_ellyy", PROCESS_ID::p5_ae_ellyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_evvvv : public p5 {
    public: p5_ea_evvvv(): p5( "ea_evvvv", PROCESS_ID::p5_ea_evvvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_exxyy : public p5 {
    public: p5_ae_exxyy(): p5( "ae_exxyy", PROCESS_ID::p5_ae_exxyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_evlxy : public p5 {
    public: p5_ae_evlxy(): p5( "ae_evlxy", PROCESS_ID::p5_ae_evlxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_vxyyy : public p5 {
    public: p5_ae_vxyyy(): p5( "ae_vxyyy", PROCESS_ID::p5_ae_vxyyy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_lllvv : public p5 {
    public: p5_ae_lllvv(): p5( "ae_lllvv", PROCESS_ID::p5_ae_lllvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eelvv : public p5 {
    public: p5_ae_eelvv(): p5( "ae_eelvv", PROCESS_ID::p5_ae_eelvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_lvvxx : public p5 {
    public: p5_ae_lvvxx(): p5( "ae_lvvxx", PROCESS_ID::p5_ae_lvvxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_ellvv : public p5 {
    public: p5_ea_ellvv(): p5( "ea_ellvv", PROCESS_ID::p5_ea_ellvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_evexy : public p5 {
    public: p5_ae_evexy(): p5( "ae_evexy", PROCESS_ID::p5_ae_evexy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_ellll : public p5 {
    public: p5_ea_ellll(): p5( "ea_ellll", PROCESS_ID::p5_ea_ellll, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_elevv : public p5 {
    public: p5_ea_elevv(): p5( "ea_elevv", PROCESS_ID::p5_ea_elevv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_eeell : public p5 {
    public: p5_ea_eeell(): p5( "ea_eeell", PROCESS_ID::p5_ea_eeell, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_lvvvv : public p5 {
    public: p5_ae_lvvvv(): p5( "ae_lvvvv", PROCESS_ID::p5_ae_lvvvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_evexy : public p5 {
    public: p5_ea_evexy(): p5( "ea_evexy", PROCESS_ID::p5_ea_evexy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_eeexx : public p5 {
    public: p5_ae_eeexx(): p5( "ae_eeexx", PROCESS_ID::p5_ae_eeexx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_lllvv : public p5 {
    public: p5_ea_lllvv(): p5( "ea_lllvv", PROCESS_ID::p5_ea_lllvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_evvxx : public p5 {
    public: p5_ea_evvxx(): p5( "ea_evvxx", PROCESS_ID::p5_ea_evvxx, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_llvxy : public p5 {
    public: p5_ea_llvxy(): p5( "ea_llvxy", PROCESS_ID::p5_ea_llvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ae_llvxy : public p5 {
    public: p5_ae_llvxy(): p5( "ae_llvxy", PROCESS_ID::p5_ae_llvxy, EVENT_CATEGORY_TRUE::p5_any ) {}; };

class p5_ea_lvvvv : public p5 {
    public: p5_ea_lvvvv(): p5( "ea_lvvvv", PROCESS_ID::p5_ea_lvvvv, EVENT_CATEGORY_TRUE::p5_any ) {}; };


#endif
