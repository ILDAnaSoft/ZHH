#ifndef FinalStates_fffff_h
#define FinalStates_fffff_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class fffff: public FinalStateResolver {
    public:
        // Set process ID and event category
        fffff( string process_name, int process_id, int event_category): FinalStateResolver( process_name, process_id, event_category, 5, 0 ) {};

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get 5 fermions
            MCParticle* f1 = (MCParticle*)mcp_collection->getElementAt(6);
            MCParticle* f2 = (MCParticle*)mcp_collection->getElementAt(7);
            MCParticle* f3 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* f4 = (MCParticle*)mcp_collection->getElementAt(9);
            MCParticle* f5 = (MCParticle*)mcp_collection->getElementAt(10);

            assert_true(
                abs(f1->getPDG()) < 17 &&
                abs(f2->getPDG()) < 17 &&
                abs(f3->getPDG()) < 17 &&
                abs(f4->getPDG()) < 17 &&
                abs(f5->getPDG()) < 17, RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                f1->getPDG(),
                f2->getPDG(),
                f3->getPDG(),
                f4->getPDG(),
                f5->getPDG(),
            };
        };

};

class f5_ae_eeevv : public fffff {
    public: f5_ae_eeevv(): fffff( "ae_eeevv", PROCESS_ID::f5_ae_eeevv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eyyyy : public fffff {
    public: f5_ea_eyyyy(): fffff( "ea_eyyyy", PROCESS_ID::f5_ea_eyyyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_elevv : public fffff {
    public: f5_ae_elevv(): fffff( "ae_elevv", PROCESS_ID::f5_ae_elevv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eyyyy : public fffff {
    public: f5_ae_eyyyy(): fffff( "ae_eyyyy", PROCESS_ID::f5_ae_eyyyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_exxxx : public fffff {
    public: f5_ea_exxxx(): fffff( "ea_exxxx", PROCESS_ID::f5_ea_exxxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_evvxx : public fffff {
    public: f5_ae_evvxx(): fffff( "ae_evvxx", PROCESS_ID::f5_ae_evvxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eeeyy : public fffff {
    public: f5_ae_eeeyy(): fffff( "ae_eeeyy", PROCESS_ID::f5_ae_eeeyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eevxy : public fffff {
    public: f5_ae_eevxy(): fffff( "ae_eevxy", PROCESS_ID::f5_ae_eevxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_lvvyy : public fffff {
    public: f5_ae_lvvyy(): fffff( "ae_lvvyy", PROCESS_ID::f5_ae_lvvyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eeevv : public fffff {
    public: f5_ea_eeevv(): fffff( "ea_eeevv", PROCESS_ID::f5_ea_eeevv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_ellxx : public fffff {
    public: f5_ea_ellxx(): fffff( "ea_ellxx", PROCESS_ID::f5_ea_ellxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eeeee : public fffff {
    public: f5_ae_eeeee(): fffff( "ae_eeeee", PROCESS_ID::f5_ae_eeeee, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_elvxy : public fffff {
    public: f5_ea_elvxy(): fffff( "ea_elvxy", PROCESS_ID::f5_ea_elvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_evvyy : public fffff {
    public: f5_ea_evvyy(): fffff( "ea_evvyy", PROCESS_ID::f5_ea_evvyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_evlxy : public fffff {
    public: f5_ea_evlxy(): fffff( "ea_evlxy", PROCESS_ID::f5_ea_evlxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_ellvv : public fffff {
    public: f5_ae_ellvv(): fffff( "ae_ellvv", PROCESS_ID::f5_ae_ellvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_vxyyy : public fffff {
    public: f5_ea_vxyyy(): fffff( "ea_vxyyy", PROCESS_ID::f5_ea_vxyyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eeexx : public fffff {
    public: f5_ea_eeexx(): fffff( "ea_eeexx", PROCESS_ID::f5_ea_eeexx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_ellll : public fffff {
    public: f5_ae_ellll(): fffff( "ae_ellll", PROCESS_ID::f5_ae_ellll, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eeell : public fffff {
    public: f5_ae_eeell(): fffff( "ae_eeell", PROCESS_ID::f5_ae_eeell, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_ellxx : public fffff {
    public: f5_ae_ellxx(): fffff( "ae_ellxx", PROCESS_ID::f5_ae_ellxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_elvxy : public fffff {
    public: f5_ae_elvxy(): fffff( "ae_elvxy", PROCESS_ID::f5_ae_elvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_vxxxy : public fffff {
    public: f5_ae_vxxxy(): fffff( "ae_vxxxy", PROCESS_ID::f5_ae_vxxxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_exxxx : public fffff {
    public: f5_ae_exxxx(): fffff( "ae_exxxx", PROCESS_ID::f5_ae_exxxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eelvv : public fffff {
    public: f5_ea_eelvv(): fffff( "ea_eelvv", PROCESS_ID::f5_ea_eelvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eeeee : public fffff {
    public: f5_ea_eeeee(): fffff( "ea_eeeee", PROCESS_ID::f5_ea_eeeee, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_evvvv : public fffff {
    public: f5_ae_evvvv(): fffff( "ae_evvvv", PROCESS_ID::f5_ae_evvvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_lvvyy : public fffff {
    public: f5_ea_lvvyy(): fffff( "ea_lvvyy", PROCESS_ID::f5_ea_lvvyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_evvyy : public fffff {
    public: f5_ae_evvyy(): fffff( "ae_evvyy", PROCESS_ID::f5_ae_evvyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_exxyy : public fffff {
    public: f5_ea_exxyy(): fffff( "ea_exxyy", PROCESS_ID::f5_ea_exxyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eevxy : public fffff {
    public: f5_ea_eevxy(): fffff( "ea_eevxy", PROCESS_ID::f5_ea_eevxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eeeyy : public fffff {
    public: f5_ea_eeeyy(): fffff( "ea_eeeyy", PROCESS_ID::f5_ea_eeeyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_ellyy : public fffff {
    public: f5_ea_ellyy(): fffff( "ea_ellyy", PROCESS_ID::f5_ea_ellyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_vxxxy : public fffff {
    public: f5_ea_vxxxy(): fffff( "ea_vxxxy", PROCESS_ID::f5_ea_vxxxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_vvvxy : public fffff {
    public: f5_ae_vvvxy(): fffff( "ae_vvvxy", PROCESS_ID::f5_ae_vvvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_vvvxy : public fffff {
    public: f5_ea_vvvxy(): fffff( "ea_vvvxy", PROCESS_ID::f5_ea_vvvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_ellyy : public fffff {
    public: f5_ae_ellyy(): fffff( "ae_ellyy", PROCESS_ID::f5_ae_ellyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_evvvv : public fffff {
    public: f5_ea_evvvv(): fffff( "ea_evvvv", PROCESS_ID::f5_ea_evvvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_exxyy : public fffff {
    public: f5_ae_exxyy(): fffff( "ae_exxyy", PROCESS_ID::f5_ae_exxyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_evlxy : public fffff {
    public: f5_ae_evlxy(): fffff( "ae_evlxy", PROCESS_ID::f5_ae_evlxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_vxyyy : public fffff {
    public: f5_ae_vxyyy(): fffff( "ae_vxyyy", PROCESS_ID::f5_ae_vxyyy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_lllvv : public fffff {
    public: f5_ae_lllvv(): fffff( "ae_lllvv", PROCESS_ID::f5_ae_lllvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eelvv : public fffff {
    public: f5_ae_eelvv(): fffff( "ae_eelvv", PROCESS_ID::f5_ae_eelvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_lvvxx : public fffff {
    public: f5_ae_lvvxx(): fffff( "ae_lvvxx", PROCESS_ID::f5_ae_lvvxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_ellvv : public fffff {
    public: f5_ea_ellvv(): fffff( "ea_ellvv", PROCESS_ID::f5_ea_ellvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_evexy : public fffff {
    public: f5_ae_evexy(): fffff( "ae_evexy", PROCESS_ID::f5_ae_evexy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_ellll : public fffff {
    public: f5_ea_ellll(): fffff( "ea_ellll", PROCESS_ID::f5_ea_ellll, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_elevv : public fffff {
    public: f5_ea_elevv(): fffff( "ea_elevv", PROCESS_ID::f5_ea_elevv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_eeell : public fffff {
    public: f5_ea_eeell(): fffff( "ea_eeell", PROCESS_ID::f5_ea_eeell, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_lvvvv : public fffff {
    public: f5_ae_lvvvv(): fffff( "ae_lvvvv", PROCESS_ID::f5_ae_lvvvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_evexy : public fffff {
    public: f5_ea_evexy(): fffff( "ea_evexy", PROCESS_ID::f5_ea_evexy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_eeexx : public fffff {
    public: f5_ae_eeexx(): fffff( "ae_eeexx", PROCESS_ID::f5_ae_eeexx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_lllvv : public fffff {
    public: f5_ea_lllvv(): fffff( "ea_lllvv", PROCESS_ID::f5_ea_lllvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_evvxx : public fffff {
    public: f5_ea_evvxx(): fffff( "ea_evvxx", PROCESS_ID::f5_ea_evvxx, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_llvxy : public fffff {
    public: f5_ea_llvxy(): fffff( "ea_llvxy", PROCESS_ID::f5_ea_llvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ae_llvxy : public fffff {
    public: f5_ae_llvxy(): fffff( "ae_llvxy", PROCESS_ID::f5_ae_llvxy, EVENT_CATEGORY_TRUE::f5_any ) {}; };

class f5_ea_lvvvv : public fffff {
    public: f5_ea_lvvvv(): fffff( "ea_lvvvv", PROCESS_ID::f5_ea_lvvvv, EVENT_CATEGORY_TRUE::f5_any ) {}; };


#endif
