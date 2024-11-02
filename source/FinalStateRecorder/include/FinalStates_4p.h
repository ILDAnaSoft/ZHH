#ifndef FinalStates_p4_h
#define FinalStates_p4_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class p4: public FinalStateResolver {
    protected:
        vector<int> m_final_state_filter;

    public:
        // Set process ID and event category
        p4( string process_name, int process_id, int event_category, vector<int> decay_filter ):
            FinalStateResolver( process_name, process_id, event_category, 4, 0, vector<int> {4,5} ),
            m_final_state_filter {decay_filter} {};

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_higgs;

            vector<MCParticle*> fs_particles;

            // Get fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(6 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(7 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(8 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(9 ));

            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            // Get fermions
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection);

            assert_true(
                vec_contains(m_final_state_filter, abs(fs_particles[0]->getPDG())) &&
                vec_contains(m_final_state_filter, abs(fs_particles[1]->getPDG())) &&
                vec_contains(m_final_state_filter, abs(fs_particles[2]->getPDG())) &&
                vec_contains(m_final_state_filter, abs(fs_particles[3]->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                fs_particles[2]->getPDG(),
                fs_particles[3]->getPDG(),
            };
        };

        int get_event_category(std::map<int, int> m_final_state_counts) {
            if (m_final_state_counts[PDG::b] == 2) {
                if (m_final_state_counts[PDG::e] == 2)
                    return EVENT_CATEGORY_TRUE::eebb;
                else if (m_final_state_counts[PDG::µ] == 2)
                    return EVENT_CATEGORY_TRUE::µµbb;
                else if (m_final_state_counts[PDG::𝜏] == 2)
                    return EVENT_CATEGORY_TRUE::𝜏𝜏bb;
                else if (m_final_state_counts[PDG::t] == 2)
                    return EVENT_CATEGORY_TRUE::ttbb;
                else if (
                    m_final_state_counts[PDG::ve] +
                    m_final_state_counts[PDG::vµ] +
                    m_final_state_counts[PDG::v𝜏] == 2)
                    return EVENT_CATEGORY_TRUE::vvbb;

            } else if (m_final_state_counts[PDG::b] == 4) {
                return EVENT_CATEGORY_TRUE::bbbb;
            }

            return m_event_category;
        };

};

class llll_zz : public p4 {
    public: llll_zz(): p4( "4f_zz_l", PROCESS_ID::f4_zz_l, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class llll_szee_lmee : public p4 { // 4f_lowmee_singleZee_leptonic
    public: llll_szee_lmee(): p4( "4f_lowmee_sze_l", PROCESS_ID::f4_szee_lmee, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class llll_szsw_lmee : public p4 { // 4f_lowmee_singleZsingleWMix_lept
    public: llll_szsw_lmee(): p4( "4f_lowmee_szeorsw_l", PROCESS_ID::f4_szsw_lmee, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class qqqq_zz : public p4 {
    public: qqqq_zz(): p4( "4f_zz_h", PROCESS_ID::f4_zz_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

class llqq_zz : public p4 {
    public: llqq_zz(): p4( "4f_zz_sl", PROCESS_ID::f4_zz_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,12,13,14,15,16,1,2,3,4,5,6} ) {}; };

class llll_ww : public p4 {
    public: llll_ww(): p4( "4f_ww_l", PROCESS_ID::f4_ww_l, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class qqqq_ww : public p4 {
    public: qqqq_ww(): p4( "4f_ww_h", PROCESS_ID::f4_ww_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

class llqq_ww : public p4 {
    public: llqq_ww(): p4( "4f_ww_sl", PROCESS_ID::f4_ww_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,12,13,14,15,16,1,2,3,4,5,6} ) {}; };

class llll_zzorww : public p4 {
    public: llll_zzorww(): p4( "4f_zzorww_l", PROCESS_ID::f4_zzorww_l, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class qqqq_zzorww : public p4 {
    public: qqqq_zzorww(): p4( "4f_zzorww_h", PROCESS_ID::f4_zzorww_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

class llll_sw : public p4 {
    public: llll_sw(): p4( "4f_sw_l", PROCESS_ID::f4_sw_l, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class llqq_sw : public p4 {
    public: llqq_sw(): p4( "4f_sw_sl", PROCESS_ID::f4_sw_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,12,13,14,15,16,1,2,3,4,5,6} ) {}; };

class llll_sze : public p4 {
    public: llll_sze(): p4( "4f_sze_l", PROCESS_ID::f4_sze_l, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class llqq_sze : public p4 {
    public: llqq_sze(): p4( "4f_sze_sl", PROCESS_ID::f4_sze_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,12,13,14,15,16,1,2,3,4,5,6} ) {}; };

class llvv_sznu : public p4 {
    public: llvv_sznu(): p4( "4f_sznu_l", PROCESS_ID::f4_sznu_l, EVENT_CATEGORY_TRUE::llvv, vector{11,12,13,14,15,16} ) {}; };

class vvqq_sznu : public p4 {
    public: vvqq_sznu(): p4( "4f_sznu_sl", PROCESS_ID::f4_sznu_l, EVENT_CATEGORY_TRUE::vvqq, vector{12,14,16,1,2,3,4,5,6} ) {}; };

class llvv_szeorsw : public p4 {
    public: llvv_szeorsw(): p4( "4f_szeorsw_l", PROCESS_ID::f4_szeorsw_l, EVENT_CATEGORY_TRUE::llvv, vector{11,12,13,14,15,16} ) {}; };


#endif
