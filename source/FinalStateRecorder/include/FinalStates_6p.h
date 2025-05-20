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
        p6( string process_name, int process_id, int event_category ):
            FinalStateResolver( process_name, process_id, event_category, 6, 0, vector<int> {4,5} ),
            first_event_check(false) {};
            
        // for the new (Whizard 2,3) samples, first_event_check is not necessary
        p6( string process_name, int process_id, int event_category, int n_fermions, int n_higgs, vector<int> isr_particles, bool skip_first_event_check = true ):
            FinalStateResolver( process_name, process_id, event_category, n_fermions, n_higgs, isr_particles ),
            first_event_check(skip_first_event_check) {};

        // on_first_event takes care of generation with Whizard 2 and 3: if the first
        // (two) particle(s) have gen=4, beam particles are included, and all positions
        // must be shifted by two; the ZHH and ZZH samples are new enough (Whizard2/3)
        // that this is not an issue, so the correction is not done in hh2f and h4f

        // see M Berggren https://agenda.linearcollider.org/event/7371/contributions/37870/attachments/30856/46172/berggren-lcws-morioka-2016-sw.pdf
        // 1 for stable particles
        // 2 particle ( meson or baryon ) decayed in generator
        // 3 documentation line
        // 4 incoming (beam) particles
        // 5 outgoing partons ( hard process )
        // remarks: 
        // in case of generated final state leptons they will be copied once with genstat
        // ISR photons will not have genstat 5
        void on_first_event(LCCollection *mcp_collection) {
            if (((MCParticle*)mcp_collection->getElementAt(1))->getGeneratorStatus() == 4) {
                shift_pos = 2;
                
                m_isr_indices[0] += shift_pos;
                m_isr_indices[1] += shift_pos;
            }
        };
        bool first_event_check{};
        int shift_pos = 0;

        vector<MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) {
            (void) resolve_higgs;

            vector<MCParticle*> fs_particles;

            // Get fermions
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 6 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 7 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 8 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 9 ));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 10));
            fs_particles.push_back((MCParticle*)mcp_collection->getElementAt(shift_pos + 11));

            return fs_particles;
        }

        vector<int> resolve(LCCollection *mcp_collection) {
            if (!first_event_check) {
                on_first_event(mcp_collection);
                first_event_check = true;
            }

            vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection);

            assert_true(
                abs(fs_particles[0]->getPDG()) < 17 &&
                abs(fs_particles[1]->getPDG()) < 17 &&
                abs(fs_particles[2]->getPDG()) < 17 &&
                abs(fs_particles[3]->getPDG()) < 17 &&
                abs(fs_particles[4]->getPDG()) < 17 &&
                abs(fs_particles[5]->getPDG()) < 17, RESOLVER_ERRORS::UNALLOWED_VALUES);

            return vector<int>{
                fs_particles[0]->getPDG(),
                fs_particles[1]->getPDG(),
                fs_particles[2]->getPDG(),
                fs_particles[3]->getPDG(),
                fs_particles[4]->getPDG(),
                fs_particles[5]->getPDG()
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
                        m_final_state_counts[PDG::ve] == 1) {
                            if (m_final_state_counts[PDG::c] == 1 &&
                                m_final_state_counts[PDG::s] == 1)
                                return EVENT_CATEGORY_TRUE::evbbcs;
                            else if (m_final_state_counts[PDG::u] == 1 &&
                                    m_final_state_counts[PDG::d] == 1)
                                return EVENT_CATEGORY_TRUE::evbbud;
                            else
                                return EVENT_CATEGORY_TRUE::evbbqq;
                        }
                    else if (m_final_state_counts[PDG::µ] == 1 &&
                             m_final_state_counts[PDG::vµ] == 1) {
                            if (m_final_state_counts[PDG::c] == 1 &&
                                    m_final_state_counts[PDG::s] == 1)
                                    return EVENT_CATEGORY_TRUE::µvbbcs;
                                else if (m_final_state_counts[PDG::u] == 1 &&
                                        m_final_state_counts[PDG::d] == 1)
                                    return EVENT_CATEGORY_TRUE::µvbbud;
                                else
                                    return EVENT_CATEGORY_TRUE::µvbbqq;
                        }
                    else if (m_final_state_counts[PDG::𝜏] == 1 &&
                             m_final_state_counts[PDG::v𝜏] == 1) {
                                if (m_final_state_counts[PDG::c] == 1 &&
                                    m_final_state_counts[PDG::s] == 1)
                                    return EVENT_CATEGORY_TRUE::𝜏vbbcs;
                                else if (m_final_state_counts[PDG::u] == 1 &&
                                        m_final_state_counts[PDG::d] == 1)
                                    return EVENT_CATEGORY_TRUE::𝜏vbbud;
                                else
                                    return EVENT_CATEGORY_TRUE::𝜏vbbqq;
                            }
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
                else if (charged_leps == 2 && neutrinos == 0)
                    return EVENT_CATEGORY_TRUE::llbbbb;
                else if (non_b == 2)
                    return EVENT_CATEGORY_TRUE::qqbbbb;
            } else if (m_final_state_counts[PDG::b] == 6) {
                return EVENT_CATEGORY_TRUE::bbbbbb;
            }

            return m_event_category;
        };

};

class p6_yycyyc : public p6 {
    public: p6_yycyyc(): p6( "yycyyc", PROCESS_ID::f6_yycyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyvlyx : public p6 {
    public: p6_yyvlyx(): p6( "yyvlyx", PROCESS_ID::f6_yyvlyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyxylv : public p6 {
    public: p6_yyxylv(): p6( "yyxylv", PROCESS_ID::f6_yyxylv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyuyyu : public p6 {
    public: p6_yyuyyu(): p6( "yyuyyu", PROCESS_ID::f6_yyuyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyuyyc : public p6 {
    public: p6_yyuyyc(): p6( "yyuyyc", PROCESS_ID::f6_yyuyyc, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyxyev : public p6 {
    public: p6_yyxyev(): p6( "yyxyev", PROCESS_ID::f6_yyxyev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyvllv : public p6 {
    public: p6_yyvllv(): p6( "yyvllv", PROCESS_ID::f6_yyvllv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyvelv : public p6 {
    public: p6_yyvelv(): p6( "yyvelv", PROCESS_ID::f6_yyvelv, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yycyyu : public p6 {
    public: p6_yycyyu(): p6( "yycyyu", PROCESS_ID::f6_yycyyu, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyveyx : public p6 {
    public: p6_yyveyx(): p6( "yyveyx", PROCESS_ID::f6_yyveyx, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyvlev : public p6 {
    public: p6_yyvlev(): p6( "yyvlev", PROCESS_ID::f6_yyvlev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

class p6_yyveev : public p6 {
    public: p6_yyveev(): p6( "yyveev", PROCESS_ID::f6_yyveev, EVENT_CATEGORY_TRUE::OTHER_TTBAR ) {}; };

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

// NEW mc-2025 inclusive productions
// 2l4q
class p6_inclusive_eeeexx : public p6 {
    public: p6_inclusive_eeeexx(): p6( "P6f_eeeexx", PROCESS_ID::f6_eeeexx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eeeeyy : public p6 {
    public: p6_inclusive_eeeeyy(): p6( "P6f_eeeeyy", PROCESS_ID::f6_eeeeyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eeevxy : public p6 {
    public: p6_inclusive_eeevxy(): p6( "P6f_eeevxy", PROCESS_ID::f6_eeevxy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eelvxy : public p6 {
    public: p6_inclusive_eelvxy(): p6( "P6f_eelvxy", PROCESS_ID::f6_eelvxy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eeveyx : public p6 {
    public: p6_inclusive_eeveyx(): p6( "P6f_eeveyx", PROCESS_ID::f6_eeveyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eevlyx : public p6 {
    public: p6_inclusive_eevlyx(): p6( "P6f_eevlyx", PROCESS_ID::f6_eevlyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eexxxx : public p6 {
    public: p6_inclusive_eexxxx(): p6( "P6f_eexxxx", PROCESS_ID::f6_eexxxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eexyyx : public p6 {
    public: p6_inclusive_eexyyx(): p6( "P6f_eexyyx", PROCESS_ID::f6_eexyyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_eeyyyy : public p6 {
    public: p6_inclusive_eeyyyy(): p6( "P6f_eeyyyy", PROCESS_ID::f6_eeyyyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_lleexx : public p6 {
    public: p6_inclusive_lleexx(): p6( "P6f_lleexx", PROCESS_ID::f6_lleexx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_lleeyy : public p6 {
    public: p6_inclusive_lleeyy(): p6( "P6f_lleeyy", PROCESS_ID::f6_lleeyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llevxy : public p6 {
    public: p6_inclusive_llevxy(): p6( "P6f_llevxy", PROCESS_ID::f6_llevxy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llllll : public p6 {
    public: p6_inclusive_llllll(): p6( "P6f_llllll", PROCESS_ID::f6_llllll, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llllxx : public p6 {
    public: p6_inclusive_llllxx(): p6( "P6f_llllxx", PROCESS_ID::f6_llllxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llllyy : public p6 {
    public: p6_inclusive_llllyy(): p6( "P6f_llllyy", PROCESS_ID::f6_llllyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llveyx : public p6 {
    public: p6_inclusive_llveyx(): p6( "P6f_llveyx", PROCESS_ID::f6_llveyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llvllv : public p6 {
    public: p6_inclusive_llvllv(): p6( "P6f_llvllv", PROCESS_ID::f6_llvllv, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llxxxx : public p6 {
    public: p6_inclusive_llxxxx(): p6( "P6f_llxxxx", PROCESS_ID::f6_llxxxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llxyyx : public p6 {
    public: p6_inclusive_llxyyx(): p6( "P6f_llxyyx", PROCESS_ID::f6_llxyyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_llyyyy : public p6 {
    public: p6_inclusive_llyyyy(): p6( "P6f_llyyyy", PROCESS_ID::f6_llyyyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_veevxx : public p6 {
    public: p6_inclusive_veevxx(): p6( "P6f_veevxx", PROCESS_ID::f6_veevxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_veevyy : public p6 {
    public: p6_inclusive_veevyy(): p6( "P6f_veevyy", PROCESS_ID::f6_veevyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_velvxx : public p6 {
    public: p6_inclusive_velvxx(): p6( "P6f_velvxx", PROCESS_ID::f6_velvxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_velvyy : public p6 {
    public: p6_inclusive_velvyy(): p6( "P6f_velvyy", PROCESS_ID::f6_velvyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vlevxx : public p6 {
    public: p6_inclusive_vlevxx(): p6( "P6f_vlevxx", PROCESS_ID::f6_vlevxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vlevyy : public p6 {
    public: p6_inclusive_vlevyy(): p6( "P6f_vlevyy", PROCESS_ID::f6_vlevyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vllvxx : public p6 {
    public: p6_inclusive_vllvxx(): p6( "P6f_vllvxx", PROCESS_ID::f6_vllvxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vllvyy : public p6 {
    public: p6_inclusive_vllvyy(): p6( "P6f_vllvyy", PROCESS_ID::f6_vllvyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvevxy : public p6 {
    public: p6_inclusive_vvevxy(): p6( "P6f_vvevxy", PROCESS_ID::f6_vvevxy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvlvxy : public p6 {
    public: p6_inclusive_vvlvxy(): p6( "P6f_vvlvxy", PROCESS_ID::f6_vvlvxy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvveyx : public p6 {
    public: p6_inclusive_vvveyx(): p6( "P6f_vvveyx", PROCESS_ID::f6_vvveyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvvllv : public p6 {
    public: p6_inclusive_vvvllv(): p6( "P6f_vvvllv", PROCESS_ID::f6_vvvllv, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvvlyx : public p6 {
    public: p6_inclusive_vvvlyx(): p6( "P6f_vvvlyx", PROCESS_ID::f6_vvvlyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvvvvv : public p6 {
    public: p6_inclusive_vvvvvv(): p6( "P6f_vvvvvv", PROCESS_ID::f6_vvvvvv, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvvvxx : public p6 {
    public: p6_inclusive_vvvvxx(): p6( "P6f_vvvvxx", PROCESS_ID::f6_vvvvxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvvvyy : public p6 {
    public: p6_inclusive_vvvvyy(): p6( "P6f_vvvvyy", PROCESS_ID::f6_vvvvyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvxxxx : public p6 {
    public: p6_inclusive_vvxxxx(): p6( "P6f_vvxxxx", PROCESS_ID::f6_vvxxxx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvxyyx : public p6 {
    public: p6_inclusive_vvxyyx(): p6( "P6f_vvxyyx", PROCESS_ID::f6_vvxyyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_vvyyyy : public p6 {
    public: p6_inclusive_vvyyyy(): p6( "P6f_vvyyyy", PROCESS_ID::f6_vvyyyy, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_xxveyx : public p6 {
    public: p6_inclusive_xxveyx(): p6( "P6f_xxveyx", PROCESS_ID::f6_xxveyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_xxvlyx : public p6 {
    public: p6_inclusive_xxvlyx(): p6( "P6f_xxvlyx", PROCESS_ID::f6_xxvlyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_xxxyev : public p6 {
    public: p6_inclusive_xxxyev(): p6( "P6f_xxxyev", PROCESS_ID::f6_xxxyev, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_xxxylv : public p6 {
    public: p6_inclusive_xxxylv(): p6( "P6f_xxxylv", PROCESS_ID::f6_xxxylv, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_yyveyx : public p6 {
    public: p6_inclusive_yyveyx(): p6( "P6f_yyveyx", PROCESS_ID::f6_yyveyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_yyvlyx : public p6 {
    public: p6_inclusive_yyvlyx(): p6( "P6f_yyvlyx", PROCESS_ID::f6_yyvlyx, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_yyxyev : public p6 {
    public: p6_inclusive_yyxyev(): p6( "P6f_yyxyev", PROCESS_ID::f6_yyxyev, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };
    
class p6_inclusive_yyxylv : public p6 {
    public: p6_inclusive_yyxylv(): p6( "P6f_yyxylv", PROCESS_ID::f6_yyxylv, EVENT_CATEGORY_TRUE::F6_OTHER ) {}; };

// 6q
class p6_inclusive_xxxxxx : public p6 {
    public: p6_inclusive_xxxxxx(): p6( "P6f_xxxxxx", PROCESS_ID::f6_xxxxxx, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_xxxyyx : public p6 {
    public: p6_inclusive_xxxyyx(): p6( "P6f_xxxyyx", PROCESS_ID::f6_xxxyyx, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_yycyyc : public p6 {
    public: p6_inclusive_yycyyc(): p6( "P6f_yycyyc", PROCESS_ID::f6_yycyyc, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_yycyyu : public p6 {
    public: p6_inclusive_yycyyu(): p6( "P6f_yycyyu", PROCESS_ID::f6_yycyyu, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_yyuyyc : public p6 {
    public: p6_inclusive_yyuyyc(): p6( "P6f_yyuyyc", PROCESS_ID::f6_yyuyyc, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_yyuyyu : public p6 {
    public: p6_inclusive_yyuyyu(): p6( "P6f_yyuyyu", PROCESS_ID::f6_yyuyyu, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
class p6_inclusive_yyyyyy : public p6 {
    public: p6_inclusive_yyyyyy(): p6( "P6f_yyyyyy", PROCESS_ID::f6_yyyyyy, EVENT_CATEGORY_TRUE::F6_OTHER ) { }; };
    
// FLAVOR TAG

class p6_ftag_uuuuuu : public p6 {
    public: p6_ftag_uuuuuu(): p6( "uuuuuu", PROCESS_ID::f6_ftag_uuuuuu, EVENT_CATEGORY_TRUE::OTHER ) {}; };

class p6_ftag_dddddd : public p6 {
    public: p6_ftag_dddddd(): p6( "dddddd", PROCESS_ID::f6_ftag_dddddd, EVENT_CATEGORY_TRUE::OTHER ) {}; };

class p6_ftag_ssssss : public p6 {
    public: p6_ftag_ssssss(): p6( "ssssss", PROCESS_ID::f6_ftag_ssssss, EVENT_CATEGORY_TRUE::OTHER ) {}; };

class p6_ftag_cccccc : public p6 {
    public: p6_ftag_cccccc(): p6( "cccccc", PROCESS_ID::f6_ftag_cccccc, EVENT_CATEGORY_TRUE::OTHER ) {}; };

class p6_ftag_bbbbbb : public p6 {
    public: p6_ftag_bbbbbb(): p6( "bbbbbb", PROCESS_ID::f6_ftag_bbbbbb, EVENT_CATEGORY_TRUE::OTHER ) {}; };






#endif
