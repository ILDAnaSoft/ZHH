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

        unsigned short F1_IDX = 6;
        unsigned short F2_IDX = 7;
        unsigned short F3_IDX = 8;
        unsigned short F4_IDX = 9;

    public:
        // Set process ID and event category
        p4( string process_name, int process_id, int event_category, vector<int> decay_filter ):
            FinalStateResolver( process_name, process_id, event_category, 4, 0, vector<int> {4,5} ),
            m_final_state_filter {decay_filter},
            m_first_event_check(false) {};
        
        p4( string process_name, int process_id, int event_category, int n_fermions, int n_higgs, vector<int> isr_particles, vector<int> decay_filter ):
            FinalStateResolver( process_name, process_id, event_category, n_fermions, n_higgs, isr_particles ),
            m_final_state_filter {decay_filter},
            m_first_event_check(false) {};

        vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_higgs;

            m_nZorHorW = 0;

            // only required for test production (f4_llbb_sl0 and f4_eebb_sl0) AND new mc-2025 4f prod
            // see workflows/resources/whizard_template/whizard.base.sin
            if (m_process_id == PROCESS_ID::f4_llbb_sl0 || m_process_id == PROCESS_ID::f4_eebb_sl0 ||
                m_process_id == PROCESS_ID::f4_zz_l0 || m_process_id == PROCESS_ID::f4_zz_h0 ||
                m_process_id == PROCESS_ID::f4_zz_sl0 || m_process_id == PROCESS_ID::f4_sze_l0 ||
		        m_process_id == PROCESS_ID::f4_sze_sl0 || m_process_id == PROCESS_ID::f4_sznu_l0 ||
                m_process_id == PROCESS_ID::f4_sznu_sl0 || m_process_id == PROCESS_ID::f4_ww_l0 ||
                m_process_id == PROCESS_ID::f4_ww_h0 || m_process_id == PROCESS_ID::f4_ww_sl0 ||
                m_process_id == PROCESS_ID::f4_sw_l0 || m_process_id == PROCESS_ID::f4_sw_sl0 ||
                m_process_id == PROCESS_ID::f4_zzorww_l0 || m_process_id == PROCESS_ID::f4_zzorww_h0 ||
                m_process_id == PROCESS_ID::f4_szeorsw_l0 || m_process_id == PROCESS_ID::f4_zznu_l0 ||
                m_process_id == PROCESS_ID::f4_zznu_sl0) {

                int mcp_cand1_pdg = abs(((MCParticle*)mcp_collection->getElementAt( m_shift_pos + 6 ))->getPDG());
                int mcp_cand2_pdg = abs(((MCParticle*)mcp_collection->getElementAt( m_shift_pos + 7 ))->getPDG());

                if (mcp_cand1_pdg == 23 ||  mcp_cand1_pdg == 24 || mcp_cand1_pdg == 25)
                    m_nZorHorW++;

                if (mcp_cand2_pdg == 23 ||  mcp_cand2_pdg == 24 || mcp_cand2_pdg == 25)
                    m_nZorHorW++;

                F1_IDX = m_shift_pos + 6 + m_nZorHorW;
                F2_IDX = m_shift_pos + 7 + m_nZorHorW;
                F3_IDX = m_shift_pos + 8 + m_nZorHorW;
                F4_IDX = m_shift_pos + 9 + m_nZorHorW;

                //std::cerr << "nZorHorW = " << m_nZorHorW << " | IDX(PDG): " << (m_shift_pos + 6) << " (PDG=" << mcp_cand1_pdg << "); " << (m_shift_pos + 7) << "(PDG=" << mcp_cand2_pdg << ")" << std::endl; 
            }
            
            return vector<int>{ F1_IDX, F2_IDX, F3_IDX, F4_IDX };
        }

        // for compatability with new samples; see FinalStates_p6.h for explanation
        void on_first_event(LCCollection *mcp_collection) {
            if (((MCParticle*)mcp_collection->getElementAt(1))->getGeneratorStatus() == 4) {
                m_shift_pos = 2;
                
                m_isr_indices[0] += m_shift_pos;
                m_isr_indices[1] += m_shift_pos;

                F1_IDX += m_shift_pos;
                F2_IDX += m_shift_pos;
                F3_IDX += m_shift_pos;
                F4_IDX += m_shift_pos;
            }
        };
        bool m_first_event_check{};
        int m_shift_pos = 0;
        int m_nZorHorW = 0;

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_fs_particle_indices(mcp_collection, resolve_higgs);

            return vector<MCParticle*>{ 
                (MCParticle*)mcp_collection->getElementAt( F1_IDX ),
                (MCParticle*)mcp_collection->getElementAt( F2_IDX ),
                (MCParticle*)mcp_collection->getElementAt( F3_IDX ),
                (MCParticle*)mcp_collection->getElementAt( F4_IDX )
            };
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            if (!m_first_event_check) {
                on_first_event(mcp_collection);
                m_first_event_check = true;
            }

            // Get fermions
            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection);

            //std::cerr << "Added " << fs_particles[0]->getPDG() << " " << fs_particles[1]->getPDG() << " " << fs_particles[2]->getPDG() << " " << fs_particles[3]->getPDG() << std::endl;

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
                else if (m_final_state_counts[PDG::μ] == 2)
                    return EVENT_CATEGORY_TRUE::μμbb;
                else if (m_final_state_counts[PDG::τ] == 2)
                    return EVENT_CATEGORY_TRUE::ττbb;
                else if (m_final_state_counts[PDG::t] == 2)
                    return EVENT_CATEGORY_TRUE::ttbb;
                else if (
                    m_final_state_counts[PDG::ve] +
                    m_final_state_counts[PDG::vμ] +
                    m_final_state_counts[PDG::vτ] == 2)
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
    public: vvqq_sznu(): p4( "4f_sznu_sl", PROCESS_ID::f4_sznu_sl, EVENT_CATEGORY_TRUE::vvqq, vector{12,14,16,1,2,3,4,5,6} ) {}; };

class llvv_szeorsw : public p4 {
    public: llvv_szeorsw(): p4( "4f_szeorsw_l", PROCESS_ID::f4_szeorsw_l, EVENT_CATEGORY_TRUE::llvv, vector{11,12,13,14,15,16} ) {}; };

class llbb_sl0 : public p4 {
    public: llbb_sl0(): p4( "llbb_sl0", PROCESS_ID::f4_llbb_sl0, EVENT_CATEGORY_TRUE::llqq, vector{13,15,5} ) {}; };

class eebb_sl0 : public p4 {
    public: eebb_sl0(): p4( "eebb_sl0", PROCESS_ID::f4_eebb_sl0, EVENT_CATEGORY_TRUE::llqq, vector{11,5} ) {}; };

    
// pure leptonic
class zz_l0 : public p4 {
    public: zz_l0(): p4( "zz_l0", PROCESS_ID::f4_zz_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class sw_l0 : public p4 {
    public: sw_l0(): p4( "sw_l0", PROCESS_ID::f4_sw_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class sze_l0 : public p4 {
    public: sze_l0(): p4( "sze_l0", PROCESS_ID::f4_sze_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class szeorsw_l0 : public p4 {
    public: szeorsw_l0(): p4( "szeorsw_l0", PROCESS_ID::f4_szeorsw_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class sznu_l0 : public p4 {
    public: sznu_l0(): p4( "sznu_l0", PROCESS_ID::f4_sznu_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class ww_l0 : public p4 {
    public: ww_l0(): p4( "ww_l0", PROCESS_ID::f4_ww_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class zznu_l0 : public p4 {
    public: zznu_l0(): p4( "zznu_l0", PROCESS_ID::f4_zznu_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

class zzorww_l0 : public p4 {
    public: zzorww_l0(): p4( "zzorww_l0", PROCESS_ID::f4_zzorww_l0, EVENT_CATEGORY_TRUE::llll, vector{11,12,13,14,15,16} ) {}; };

// pure hadronic
class ww_h0 : public p4 {
    public: ww_h0(): p4( "ww_h0", PROCESS_ID::f4_ww_h0, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

class zz_h0 : public p4 {
    public: zz_h0(): p4( "zz_h0", PROCESS_ID::f4_zz_h0, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

class zzorww_h0 : public p4 {
    public: zzorww_h0(): p4( "zzorww_h0", PROCESS_ID::f4_zzorww_h0, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5,6} ) {}; };

// semi leptonic
class sw_sl0 : public p4 {
    public: sw_sl0(): p4( "sw_sl0", PROCESS_ID::f4_sw_sl0, EVENT_CATEGORY_TRUE::lvqq, vector{1,2,3,4,5,6,11,12,13,14,15,16} ) {}; };

class sze_sl0 : public p4 {
    public: sze_sl0(): p4( "sze_sl0", PROCESS_ID::f4_sze_sl0, EVENT_CATEGORY_TRUE::llqq, vector{1,2,3,4,5,6,11} ) {}; };

class sznu_sl0 : public p4 {
    public: sznu_sl0(): p4( "sznu_sl0", PROCESS_ID::f4_sznu_sl0, EVENT_CATEGORY_TRUE::vvqq, vector{1,2,3,4,5,6,12,14,16} ) {}; };

class ww_sl0 : public p4 {
    public: ww_sl0(): p4( "ww_sl0", PROCESS_ID::f4_ww_sl0, EVENT_CATEGORY_TRUE::lvqq, vector{1,2,3,4,5,6,11,12,13,14,15,16} ) {}; };

class zz_sl0 : public p4 {
    public: zz_sl0(): p4( "zz_sl0", PROCESS_ID::f4_zz_sl0, EVENT_CATEGORY_TRUE::llqq, vector{1,2,3,4,5,6,11,13,15} ) {}; };

class zznu_sl0 : public p4 {
    public: zznu_sl0(): p4( "zznu_sl0", PROCESS_ID::f4_zznu_sl0, EVENT_CATEGORY_TRUE::vvqq, vector{1,2,3,4,5,6,12,14,16} ) {}; };

#endif
