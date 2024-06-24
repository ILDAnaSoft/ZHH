#ifndef FinalStates_ffff_h
#define FinalStates_ffff_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffff: public FinalStateResolver {
    protected:
        vector<int> m_z_decay_filter;

    public:
        // Set process ID and event category
        ffff( string process_name, int process_id, int event_category, vector<int> z_decay_filter ): FinalStateResolver( process_name, process_id, event_category ) {
            m_z_decay_filter = z_decay_filter;
        };

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get Z-decayed fermions
            MCParticle* f1 = (MCParticle*)mcp_collection->getElementAt(6);
            MCParticle* f2 = (MCParticle*)mcp_collection->getElementAt(7);
            MCParticle* f3 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* f4 = (MCParticle*)mcp_collection->getElementAt(9);

            assert_true(
                vec_contains(m_z_decay_filter, abs(f1->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(f2->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(f3->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(f4->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                f1->getPDG(),
                f2->getPDG(),
                f3->getPDG(),
                f4->getPDG(),
            };
        };

};

class llll_zz : public ffff {
    public: llll_zz(): ffff( "4f_zz_l", PROCESS_ID::f4_zz_l, EVENT_CATEGORY_TRUE::llll, vector{11,13,15} ) {}; };

class qqqq_zz : public ffff {
    public: qqqq_zz(): ffff( "4f_zz_h", PROCESS_ID::f4_zz_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5} ) {}; };

class llqq_zz : public ffff {
    public: llqq_zz(): ffff( "4f_zz_sl", PROCESS_ID::f4_zz_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,13,15,1,2,3,4,5} ) {}; };

class llll_ww : public ffff {
    public: llll_ww(): ffff( "4f_ww_l", PROCESS_ID::f4_ww_l, EVENT_CATEGORY_TRUE::llll, vector{11,13,15} ) {}; };

class qqqq_ww : public ffff {
    public: qqqq_ww(): ffff( "4f_ww_h", PROCESS_ID::f4_ww_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5} ) {}; };

class llqq_ww : public ffff {
    public: llqq_ww(): ffff( "4f_ww_sl", PROCESS_ID::f4_ww_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,13,15,1,2,3,4,5} ) {}; };

class llll_zzorww : public ffff {
    public: llll_zzorww(): ffff( "4f_zzorww_l", PROCESS_ID::f4_zzorww_l, EVENT_CATEGORY_TRUE::llll, vector{11,13,15} ) {}; };

class qqqq_zzorww : public ffff {
    public: qqqq_zzorww(): ffff( "4f_zzorww_h", PROCESS_ID::f4_zzorww_h, EVENT_CATEGORY_TRUE::qqqq, vector{1,2,3,4,5} ) {}; };

class llll_sw : public ffff {
    public: llll_sw(): ffff( "4f_sw_l", PROCESS_ID::f4_sw_l, EVENT_CATEGORY_TRUE::llll, vector{11,13,15} ) {}; };

class llqq_sw : public ffff {
    public: llqq_sw(): ffff( "4f_sw_sl", PROCESS_ID::f4_sw_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,13,15,1,2,3,4,5} ) {}; };

class llll_sze : public ffff {
    public: llll_sze(): ffff( "4f_sze_l", PROCESS_ID::f4_sze_l, EVENT_CATEGORY_TRUE::llll, vector{11,13,15} ) {}; };

class llqq_sze : public ffff {
    public: llqq_sze(): ffff( "4f_sze_sl", PROCESS_ID::f4_sze_sl, EVENT_CATEGORY_TRUE::llqq, vector{11,13,15,1,2,3,4,5} ) {}; };

class llvv_sznu : public ffff {
    public: llvv_sznu(): ffff( "4f_sznu_l", PROCESS_ID::f4_sznu_l, EVENT_CATEGORY_TRUE::llvv, vector{11,13,15,12,14,16} ) {}; };

class vvqq_sznu : public ffff {
    public: vvqq_sznu(): ffff( "4f_sznu_sl", PROCESS_ID::f4_sznu_l, EVENT_CATEGORY_TRUE::vvqq, vector{12,14,16,1,2,3,4,5} ) {}; };

class llvv_szeorsw : public ffff {
    public: llvv_szeorsw(): ffff( "4f_szeorsw_l", PROCESS_ID::f4_szeorsw_l, EVENT_CATEGORY_TRUE::llvv, vector{13,15,17,12,14,16,} ) {}; };


#endif
