#ifndef FinalStates_p6_h
#define FinalStates_p6_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class p6: public FinalStateResolver {
    public:
        // Set process ID and event category
        p6( string process_name, int process_id, int event_category ): FinalStateResolver( process_name, process_id, event_category, 6, 0 ) {};
        p6( string process_name, int process_id, int event_category, int n_fermions, int n_higgs ): FinalStateResolver( process_name, process_id, event_category, n_fermions, n_higgs ) {};

        vector<int> m_resolve(LCCollection *mcp_collection) {
            MCParticle* part1 = (MCParticle*)mcp_collection->getElementAt(6);
            MCParticle* part2 = (MCParticle*)mcp_collection->getElementAt(7);
            MCParticle* part3 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* part4 = (MCParticle*)mcp_collection->getElementAt(9);
            MCParticle* part5 = (MCParticle*)mcp_collection->getElementAt(10);
            MCParticle* part6 = (MCParticle*)mcp_collection->getElementAt(11);

            assert_true(
                abs(part1->getPDG()) < 17 &&
                abs(part2->getPDG()) < 17 &&
                abs(part3->getPDG()) < 17 &&
                abs(part4->getPDG()) < 17 &&
                abs(part5->getPDG()) < 17 &&
                abs(part6->getPDG()) < 17, RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                part1->getPDG(),
                part2->getPDG(),
                part3->getPDG(),
                part4->getPDG(),
                part5->getPDG(),
                part6->getPDG()
            };
        };

        virtual int get_event_category(std::map<int, int> m_final_state_counts) {
            int non_b =
                m_final_state_counts[PDG::u] +
                m_final_state_counts[PDG::d] +
                m_final_state_counts[PDG::c] +
                m_final_state_counts[PDG::s] +
                m_final_state_counts[PDG::t];

            int charged_leps =
                m_final_state_counts[PDG::e] +
                m_final_state_counts[PDG::µ] +
                m_final_state_counts[PDG::𝜏];

            int neutrinos =
                m_final_state_counts[PDG::ve] +
                m_final_state_counts[PDG::vµ] +
                m_final_state_counts[PDG::v𝜏];

            // bb
            if (m_final_state_counts[PDG::b] == 2) {
                // qq
                if (non_b == 2) {
                    if (m_final_state_counts[PDG::e] == 1 &&
                        m_final_state_counts[PDG::ve] == 1)
                        return EVENT_CATEGORY_TRUE::evbbqq;
                    else if (m_final_state_counts[PDG::µ] == 1 &&
                             m_final_state_counts[PDG::vµ] == 1)
                        return EVENT_CATEGORY_TRUE::µvbbqq;
                    else if (m_final_state_counts[PDG::𝜏] == 1 &&
                             m_final_state_counts[PDG::v𝜏] == 1)
                        return EVENT_CATEGORY_TRUE::𝜏vbbqq;
                } else if (non_b == 4) {
                    if (m_final_state_counts[PDG::s] == 2 && m_final_state_counts[PDG::c] == 2)
                        return EVENT_CATEGORY_TRUE::bbcssc;
                    else if (
                        m_final_state_counts[PDG::c] == 1 && 
                        m_final_state_counts[PDG::s] == 1 &&
                        m_final_state_counts[PDG::d] == 1 &&
                        m_final_state_counts[PDG::u] == 1)
                        return EVENT_CATEGORY_TRUE::bbcsdu;
                    else if (m_final_state_counts[PDG::u] == 2 && m_final_state_counts[PDG::d] == 2)
                        return EVENT_CATEGORY_TRUE::bbuddu;
                    else
                        return EVENT_CATEGORY_TRUE::bbqqqq;
                }

            } else if (m_final_state_counts[PDG::b] == 4) {
                if (neutrinos == 2)
                    return EVENT_CATEGORY_TRUE::vvbbbb;
                else if (charged_leps + neutrinos == 2)
                    return EVENT_CATEGORY_TRUE::llbbbb;
                else if (non_b == 2)
                    return EVENT_CATEGORY_TRUE::qqbbbb;
            } else if (m_final_state_counts[PDG::b] == 6) {
                return EVENT_CATEGORY_TRUE::bbbbbb;
            }

            return m_event_category;
        };

};

class p6_ttbar_yycyyc : public p6 {
    public: p6_ttbar_yycyyc(): p6( "yycyyc", PROCESS_ID::f6_ttbar_yycyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyvlyx : public p6 {
    public: p6_ttbar_yyvlyx(): p6( "yyvlyx", PROCESS_ID::f6_ttbar_yyvlyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyxylv : public p6 {
    public: p6_ttbar_yyxylv(): p6( "yyxylv", PROCESS_ID::f6_ttbar_yyxylv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyuyyu : public p6 {
    public: p6_ttbar_yyuyyu(): p6( "yyuyyu", PROCESS_ID::f6_ttbar_yyuyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyuyyc : public p6 {
    public: p6_ttbar_yyuyyc(): p6( "yyuyyc", PROCESS_ID::f6_ttbar_yyuyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyxyev : public p6 {
    public: p6_ttbar_yyxyev(): p6( "yyxyev", PROCESS_ID::f6_ttbar_yyxyev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyvllv : public p6 {
    public: p6_ttbar_yyvllv(): p6( "yyvllv", PROCESS_ID::f6_ttbar_yyvllv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyvelv : public p6 {
    public: p6_ttbar_yyvelv(): p6( "yyvelv", PROCESS_ID::f6_ttbar_yyvelv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yycyyu : public p6 {
    public: p6_ttbar_yycyyu(): p6( "yycyyu", PROCESS_ID::f6_ttbar_yycyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyveyx : public p6 {
    public: p6_ttbar_yyveyx(): p6( "yyveyx", PROCESS_ID::f6_ttbar_yyveyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyvlev : public p6 {
    public: p6_ttbar_yyvlev(): p6( "yyvlev", PROCESS_ID::f6_ttbar_yyvlev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_ttbar_yyveev : public p6 {
    public: p6_ttbar_yyveev(): p6( "yyveev", PROCESS_ID::f6_ttbar_yyveev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyyyZ_yyyyee : public p6 {
    public: p6_yyyyZ_yyyyee(): p6( "yyyyee", PROCESS_ID::f6_yyyyZ_yyyyee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eeeexx : public p6 {
    public: p6_yyyyZ_eeeexx(): p6( "eeeexx", PROCESS_ID::f6_yyyyZ_eeeexx, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eeeell : public p6 {
    public: p6_yyyyZ_eeeell(): p6( "eeeell", PROCESS_ID::f6_yyyyZ_eeeell, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eeeeyy : public p6 {
    public: p6_yyyyZ_eeeeyy(): p6( "eeeeyy", PROCESS_ID::f6_yyyyZ_eeeeyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eellyy : public p6 {
    public: p6_yyyyZ_eellyy(): p6( "eellyy", PROCESS_ID::f6_yyyyZ_eellyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_yyyyyy : public p6 {
    public: p6_yyyyZ_yyyyyy(): p6( "yyyyyy", PROCESS_ID::f6_yyyyZ_yyyyyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_llllee : public p6 {
    public: p6_yyyyZ_llllee(): p6( "llllee", PROCESS_ID::f6_yyyyZ_llllee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_yyyyll : public p6 {
    public: p6_yyyyZ_yyyyll(): p6( "yyyyll", PROCESS_ID::f6_yyyyZ_yyyyll, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_yyyyvv : public p6 {
    public: p6_yyyyZ_yyyyvv(): p6( "yyyyvv", PROCESS_ID::f6_yyyyZ_yyyyvv, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eellxx : public p6 {
    public: p6_yyyyZ_eellxx(): p6( "eellxx", PROCESS_ID::f6_yyyyZ_eellxx, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_yyyyZ_eeeeee : public p6 {
    public: p6_yyyyZ_eeeeee(): p6( "eeeeee", PROCESS_ID::f6_yyyyZ_eeeeee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class p6_vvWW_vvxyyx : public p6 {
    public: p6_vvWW_vvxyyx(): p6( "vvxyyx", PROCESS_ID::f6_vvWW_vvxyyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvxylv : public p6 {
    public: p6_vvWW_vvxylv(): p6( "vvxylv", PROCESS_ID::f6_vvWW_vvxylv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvveev : public p6 {
    public: p6_vvWW_vvveev(): p6( "vvveev", PROCESS_ID::f6_vvWW_vvveev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvveyx : public p6 {
    public: p6_vvWW_vvveyx(): p6( "vvveyx", PROCESS_ID::f6_vvWW_vvveyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvvlyx : public p6 {
    public: p6_vvWW_vvvlyx(): p6( "vvvlyx", PROCESS_ID::f6_vvWW_vvvlyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvvllv : public p6 {
    public: p6_vvWW_vvvllv(): p6( "vvvllv", PROCESS_ID::f6_vvWW_vvvllv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvxyev : public p6 {
    public: p6_vvWW_vvxyev(): p6( "vvxyev", PROCESS_ID::f6_vvWW_vvxyev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvvlev : public p6 {
    public: p6_vvWW_vvvlev(): p6( "vvvlev", PROCESS_ID::f6_vvWW_vvvlev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_vvWW_vvvelv : public p6 {
    public: p6_vvWW_vvvelv(): p6( "vvvelv", PROCESS_ID::f6_vvWW_vvvelv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class p6_eeWW_eeveev : public p6 {
    public: p6_eeWW_eeveev(): p6( "eeveev", PROCESS_ID::f6_eeWW_eeveev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eexyyx : public p6 {
    public: p6_eeWW_eexyyx(): p6( "eexyyx", PROCESS_ID::f6_eeWW_eexyyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eevlev : public p6 {
    public: p6_eeWW_eevlev(): p6( "eevlev", PROCESS_ID::f6_eeWW_eevlev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eexyev : public p6 {
    public: p6_eeWW_eexyev(): p6( "eexyev", PROCESS_ID::f6_eeWW_eexyev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eeveyx : public p6 {
    public: p6_eeWW_eeveyx(): p6( "eeveyx", PROCESS_ID::f6_eeWW_eeveyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eevllv : public p6 {
    public: p6_eeWW_eevllv(): p6( "eevllv", PROCESS_ID::f6_eeWW_eevllv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eevlyx : public p6 {
    public: p6_eeWW_eevlyx(): p6( "eevlyx", PROCESS_ID::f6_eeWW_eevlyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eexylv : public p6 {
    public: p6_eeWW_eexylv(): p6( "eexylv", PROCESS_ID::f6_eeWW_eexylv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_eeWW_eevelv : public p6 {
    public: p6_eeWW_eevelv(): p6( "eevelv", PROCESS_ID::f6_eeWW_eevelv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class p6_xxWW_xxveyx : public p6 {
    public: p6_xxWW_xxveyx(): p6( "xxveyx", PROCESS_ID::f6_xxWW_xxveyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxxyyx : public p6 {
    public: p6_xxWW_xxxyyx(): p6( "xxxyyx", PROCESS_ID::f6_xxWW_xxxyyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxxylv : public p6 {
    public: p6_xxWW_xxxylv(): p6( "xxxylv", PROCESS_ID::f6_xxWW_xxxylv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxvlyx : public p6 {
    public: p6_xxWW_xxvlyx(): p6( "xxvlyx", PROCESS_ID::f6_xxWW_xxvlyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxveev : public p6 {
    public: p6_xxWW_xxveev(): p6( "xxveev", PROCESS_ID::f6_xxWW_xxveev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxvelv : public p6 {
    public: p6_xxWW_xxvelv(): p6( "xxvelv", PROCESS_ID::f6_xxWW_xxvelv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxxyev : public p6 {
    public: p6_xxWW_xxxyev(): p6( "xxxyev", PROCESS_ID::f6_xxWW_xxxyev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxvllv : public p6 {
    public: p6_xxWW_xxvllv(): p6( "xxvllv", PROCESS_ID::f6_xxWW_xxvllv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxWW_xxvlev : public p6 {
    public: p6_xxWW_xxvlev(): p6( "xxvlev", PROCESS_ID::f6_xxWW_xxvlev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class p6_xxxxZ_xxxxee : public p6 {
    public: p6_xxxxZ_xxxxee(): p6( "xxxxee", PROCESS_ID::f6_xxxxZ_xxxxee, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_xxxxZ_vvvvyy : public p6 {
    public: p6_xxxxZ_vvvvyy(): p6( "vvvvyy", PROCESS_ID::f6_xxxxZ_vvvvyy, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_xxxxZ_xxxxvv : public p6 {
    public: p6_xxxxZ_xxxxvv(): p6( "xxxxvv", PROCESS_ID::f6_xxxxZ_xxxxvv, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_xxxxZ_xxxxxx : public p6 {
    public: p6_xxxxZ_xxxxxx(): p6( "xxxxxx", PROCESS_ID::f6_xxxxZ_xxxxxx, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_xxxxZ_xxxxll : public p6 {
    public: p6_xxxxZ_xxxxll(): p6( "xxxxll", PROCESS_ID::f6_xxxxZ_xxxxll, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_xxxxZ_vvvvxx : public p6 {
    public: p6_xxxxZ_vvvvxx(): p6( "vvvvxx", PROCESS_ID::f6_xxxxZ_vvvvxx, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class p6_llWW_llxylv : public p6 {
    public: p6_llWW_llxylv(): p6( "llxylv", PROCESS_ID::f6_llWW_llxylv, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llveyx : public p6 {
    public: p6_llWW_llveyx(): p6( "llveyx", PROCESS_ID::f6_llWW_llveyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llvlev : public p6 {
    public: p6_llWW_llvlev(): p6( "llvlev", PROCESS_ID::f6_llWW_llvlev, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llvelv : public p6 {
    public: p6_llWW_llvelv(): p6( "llvelv", PROCESS_ID::f6_llWW_llvelv, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llvlyx : public p6 {
    public: p6_llWW_llvlyx(): p6( "llvlyx", PROCESS_ID::f6_llWW_llvlyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llxyev : public p6 {
    public: p6_llWW_llxyev(): p6( "llxyev", PROCESS_ID::f6_llWW_llxyev, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llxyyx : public p6 {
    public: p6_llWW_llxyyx(): p6( "llxyyx", PROCESS_ID::f6_llWW_llxyyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class p6_llWW_llvllv : public p6 {
    public: p6_llWW_llvllv(): p6( "llvllv", PROCESS_ID::f6_llWW_llvllv, EVENT_CATEGORY_TRUE::llWW ) {}; };



#endif
