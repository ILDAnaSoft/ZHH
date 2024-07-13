#ifndef FinalStates_ffffff_h
#define FinalStates_ffffff_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class f6: public FinalStateResolver {
    public:
        // Set process ID and event category
        f6( string process_name, int process_id, int event_category ): FinalStateResolver( process_name, process_id, event_category, 6, 0 ) {};

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get Z-decayed fermions
            MCParticle* f1 = (MCParticle*)mcp_collection->getElementAt(6);
            MCParticle* f2 = (MCParticle*)mcp_collection->getElementAt(7);
            MCParticle* f3 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* f4 = (MCParticle*)mcp_collection->getElementAt(9);
            MCParticle* f5 = (MCParticle*)mcp_collection->getElementAt(10);
            MCParticle* f6 = (MCParticle*)mcp_collection->getElementAt(11);

            assert_true(
                abs(f1->getPDG()) < 17 &&
                abs(f2->getPDG()) < 17 &&
                abs(f3->getPDG()) < 17 &&
                abs(f4->getPDG()) < 17 &&
                abs(f5->getPDG()) < 17 &&
                abs(f6->getPDG()) < 17, RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                f1->getPDG(),
                f2->getPDG(),
                f3->getPDG(),
                f4->getPDG(),
                f5->getPDG(),
                f6->getPDG()
            };
        };

};

class f6_ttbar_yycyyc : public f6 {
    public: f6_ttbar_yycyyc(): f6( "yycyyc", PROCESS_ID::f6_ttbar_yycyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyvlyx : public f6 {
    public: f6_ttbar_yyvlyx(): f6( "yyvlyx", PROCESS_ID::f6_ttbar_yyvlyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyxylv : public f6 {
    public: f6_ttbar_yyxylv(): f6( "yyxylv", PROCESS_ID::f6_ttbar_yyxylv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyuyyu : public f6 {
    public: f6_ttbar_yyuyyu(): f6( "yyuyyu", PROCESS_ID::f6_ttbar_yyuyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyuyyc : public f6 {
    public: f6_ttbar_yyuyyc(): f6( "yyuyyc", PROCESS_ID::f6_ttbar_yyuyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyxyev : public f6 {
    public: f6_ttbar_yyxyev(): f6( "yyxyev", PROCESS_ID::f6_ttbar_yyxyev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyvllv : public f6 {
    public: f6_ttbar_yyvllv(): f6( "yyvllv", PROCESS_ID::f6_ttbar_yyvllv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyvelv : public f6 {
    public: f6_ttbar_yyvelv(): f6( "yyvelv", PROCESS_ID::f6_ttbar_yyvelv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yycyyu : public f6 {
    public: f6_ttbar_yycyyu(): f6( "yycyyu", PROCESS_ID::f6_ttbar_yycyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyveyx : public f6 {
    public: f6_ttbar_yyveyx(): f6( "yyveyx", PROCESS_ID::f6_ttbar_yyveyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyvlev : public f6 {
    public: f6_ttbar_yyvlev(): f6( "yyvlev", PROCESS_ID::f6_ttbar_yyvlev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_ttbar_yyveev : public f6 {
    public: f6_ttbar_yyveev(): f6( "yyveev", PROCESS_ID::f6_ttbar_yyveev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class f6_yyyyZ_yyyyee : public f6 {
    public: f6_yyyyZ_yyyyee(): f6( "yyyyee", PROCESS_ID::f6_yyyyZ_yyyyee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eeeexx : public f6 {
    public: f6_yyyyZ_eeeexx(): f6( "eeeexx", PROCESS_ID::f6_yyyyZ_eeeexx, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eeeell : public f6 {
    public: f6_yyyyZ_eeeell(): f6( "eeeell", PROCESS_ID::f6_yyyyZ_eeeell, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eeeeyy : public f6 {
    public: f6_yyyyZ_eeeeyy(): f6( "eeeeyy", PROCESS_ID::f6_yyyyZ_eeeeyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eellyy : public f6 {
    public: f6_yyyyZ_eellyy(): f6( "eellyy", PROCESS_ID::f6_yyyyZ_eellyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_yyyyyy : public f6 {
    public: f6_yyyyZ_yyyyyy(): f6( "yyyyyy", PROCESS_ID::f6_yyyyZ_yyyyyy, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_llllee : public f6 {
    public: f6_yyyyZ_llllee(): f6( "llllee", PROCESS_ID::f6_yyyyZ_llllee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_yyyyll : public f6 {
    public: f6_yyyyZ_yyyyll(): f6( "yyyyll", PROCESS_ID::f6_yyyyZ_yyyyll, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_yyyyvv : public f6 {
    public: f6_yyyyZ_yyyyvv(): f6( "yyyyvv", PROCESS_ID::f6_yyyyZ_yyyyvv, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eellxx : public f6 {
    public: f6_yyyyZ_eellxx(): f6( "eellxx", PROCESS_ID::f6_yyyyZ_eellxx, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_yyyyZ_eeeeee : public f6 {
    public: f6_yyyyZ_eeeeee(): f6( "eeeeee", PROCESS_ID::f6_yyyyZ_eeeeee, EVENT_CATEGORY_TRUE::f6_yyyyZ ) {}; };

class f6_vvWW_vvxyyx : public f6 {
    public: f6_vvWW_vvxyyx(): f6( "vvxyyx", PROCESS_ID::f6_vvWW_vvxyyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvxylv : public f6 {
    public: f6_vvWW_vvxylv(): f6( "vvxylv", PROCESS_ID::f6_vvWW_vvxylv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvveev : public f6 {
    public: f6_vvWW_vvveev(): f6( "vvveev", PROCESS_ID::f6_vvWW_vvveev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvveyx : public f6 {
    public: f6_vvWW_vvveyx(): f6( "vvveyx", PROCESS_ID::f6_vvWW_vvveyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvvlyx : public f6 {
    public: f6_vvWW_vvvlyx(): f6( "vvvlyx", PROCESS_ID::f6_vvWW_vvvlyx, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvvllv : public f6 {
    public: f6_vvWW_vvvllv(): f6( "vvvllv", PROCESS_ID::f6_vvWW_vvvllv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvxyev : public f6 {
    public: f6_vvWW_vvxyev(): f6( "vvxyev", PROCESS_ID::f6_vvWW_vvxyev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvvlev : public f6 {
    public: f6_vvWW_vvvlev(): f6( "vvvlev", PROCESS_ID::f6_vvWW_vvvlev, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_vvWW_vvvelv : public f6 {
    public: f6_vvWW_vvvelv(): f6( "vvvelv", PROCESS_ID::f6_vvWW_vvvelv, EVENT_CATEGORY_TRUE::vvWW ) {}; };

class f6_eeWW_eeveev : public f6 {
    public: f6_eeWW_eeveev(): f6( "eeveev", PROCESS_ID::f6_eeWW_eeveev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eexyyx : public f6 {
    public: f6_eeWW_eexyyx(): f6( "eexyyx", PROCESS_ID::f6_eeWW_eexyyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eevlev : public f6 {
    public: f6_eeWW_eevlev(): f6( "eevlev", PROCESS_ID::f6_eeWW_eevlev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eexyev : public f6 {
    public: f6_eeWW_eexyev(): f6( "eexyev", PROCESS_ID::f6_eeWW_eexyev, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eeveyx : public f6 {
    public: f6_eeWW_eeveyx(): f6( "eeveyx", PROCESS_ID::f6_eeWW_eeveyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eevllv : public f6 {
    public: f6_eeWW_eevllv(): f6( "eevllv", PROCESS_ID::f6_eeWW_eevllv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eevlyx : public f6 {
    public: f6_eeWW_eevlyx(): f6( "eevlyx", PROCESS_ID::f6_eeWW_eevlyx, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eexylv : public f6 {
    public: f6_eeWW_eexylv(): f6( "eexylv", PROCESS_ID::f6_eeWW_eexylv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_eeWW_eevelv : public f6 {
    public: f6_eeWW_eevelv(): f6( "eevelv", PROCESS_ID::f6_eeWW_eevelv, EVENT_CATEGORY_TRUE::eeWW ) {}; };

class f6_xxWW_xxveyx : public f6 {
    public: f6_xxWW_xxveyx(): f6( "xxveyx", PROCESS_ID::f6_xxWW_xxveyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxxyyx : public f6 {
    public: f6_xxWW_xxxyyx(): f6( "xxxyyx", PROCESS_ID::f6_xxWW_xxxyyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxxylv : public f6 {
    public: f6_xxWW_xxxylv(): f6( "xxxylv", PROCESS_ID::f6_xxWW_xxxylv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxvlyx : public f6 {
    public: f6_xxWW_xxvlyx(): f6( "xxvlyx", PROCESS_ID::f6_xxWW_xxvlyx, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxveev : public f6 {
    public: f6_xxWW_xxveev(): f6( "xxveev", PROCESS_ID::f6_xxWW_xxveev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxvelv : public f6 {
    public: f6_xxWW_xxvelv(): f6( "xxvelv", PROCESS_ID::f6_xxWW_xxvelv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxxyev : public f6 {
    public: f6_xxWW_xxxyev(): f6( "xxxyev", PROCESS_ID::f6_xxWW_xxxyev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxvllv : public f6 {
    public: f6_xxWW_xxvllv(): f6( "xxvllv", PROCESS_ID::f6_xxWW_xxvllv, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxWW_xxvlev : public f6 {
    public: f6_xxWW_xxvlev(): f6( "xxvlev", PROCESS_ID::f6_xxWW_xxvlev, EVENT_CATEGORY_TRUE::f6_xxWW ) {}; };

class f6_xxxxZ_xxxxee : public f6 {
    public: f6_xxxxZ_xxxxee(): f6( "xxxxee", PROCESS_ID::f6_xxxxZ_xxxxee, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_xxxxZ_vvvvyy : public f6 {
    public: f6_xxxxZ_vvvvyy(): f6( "vvvvyy", PROCESS_ID::f6_xxxxZ_vvvvyy, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_xxxxZ_xxxxvv : public f6 {
    public: f6_xxxxZ_xxxxvv(): f6( "xxxxvv", PROCESS_ID::f6_xxxxZ_xxxxvv, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_xxxxZ_xxxxxx : public f6 {
    public: f6_xxxxZ_xxxxxx(): f6( "xxxxxx", PROCESS_ID::f6_xxxxZ_xxxxxx, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_xxxxZ_xxxxll : public f6 {
    public: f6_xxxxZ_xxxxll(): f6( "xxxxll", PROCESS_ID::f6_xxxxZ_xxxxll, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_xxxxZ_vvvvxx : public f6 {
    public: f6_xxxxZ_vvvvxx(): f6( "vvvvxx", PROCESS_ID::f6_xxxxZ_vvvvxx, EVENT_CATEGORY_TRUE::f6_xxxxZ ) {}; };

class f6_llWW_llxylv : public f6 {
    public: f6_llWW_llxylv(): f6( "llxylv", PROCESS_ID::f6_llWW_llxylv, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llveyx : public f6 {
    public: f6_llWW_llveyx(): f6( "llveyx", PROCESS_ID::f6_llWW_llveyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llvlev : public f6 {
    public: f6_llWW_llvlev(): f6( "llvlev", PROCESS_ID::f6_llWW_llvlev, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llvelv : public f6 {
    public: f6_llWW_llvelv(): f6( "llvelv", PROCESS_ID::f6_llWW_llvelv, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llvlyx : public f6 {
    public: f6_llWW_llvlyx(): f6( "llvlyx", PROCESS_ID::f6_llWW_llvlyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llxyev : public f6 {
    public: f6_llWW_llxyev(): f6( "llxyev", PROCESS_ID::f6_llWW_llxyev, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llxyyx : public f6 {
    public: f6_llWW_llxyyx(): f6( "llxyyx", PROCESS_ID::f6_llWW_llxyyx, EVENT_CATEGORY_TRUE::llWW ) {}; };

class f6_llWW_llvllv : public f6 {
    public: f6_llWW_llvllv(): f6( "llvllv", PROCESS_ID::f6_llWW_llvllv, EVENT_CATEGORY_TRUE::llWW ) {}; };



#endif
