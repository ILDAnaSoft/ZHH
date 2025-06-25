#ifndef FinalStateResolver_h
#define FinalStateResolver_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/MCParticle.h"
#include "lcio.h"
#include "vector"
#include "string"

using namespace lcio;
using namespace marlin;

struct RESOLVER_ERRORS {
    enum Values: unsigned int {
        OK = 0,
        UNKNOWN_ERROR = 5001,
        UNALLOWED_VALUES = 5002, // the definition of expected particles does not match for an encounted event. probably the way the event was generated has changed 
        UNEXPECTED_CHILDREN = 5020,
        HIGGS_NOT_FOUND = 5030,
        UNEXPECTED_SIZE = 5040,
        ISR_NOT_FOUND = 5050
    };
};

class FinalStateResolver {
    protected:
        std::string m_process_name;
        int m_process_id;
        int m_event_category;
        int m_n_fermions;
        int m_n_higgs;
        std::vector<int> m_isr_indices;

        int m_n_b_from_higgs = 0;
        int m_n_c_from_higgs = 0;

        void assert_true(bool check) {
            if (!check) {
                throw RESOLVER_ERRORS::UNKNOWN_ERROR;
            }; };
        void assert_true(bool check, int err) {
            if (!check) {
                throw err;
            }; };

        // Helper functions
        int pdg_of_particle(EVENT::LCObject* particle);
        std::vector<int> pdgs_of_daughter_particles(EVENT::LCObject* particle);
        std::vector<int> pdgs_of_daughter_particles(EVENT::MCParticle* particle);

        std::vector<int> pdgs_of_nth_hadronic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_leptonic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_semilept_decay(LCCollection *mcp_collection, int n);

    public:
        FinalStateResolver(std::string process_name, int process_id, int event_category, int n_fermions, int n_higgs, std::vector<int> isr_indices);
        virtual ~FinalStateResolver();

        std::string get_process_name() { return m_process_name; };
        int get_process_id() { return m_process_id; };
        
        int get_n_fermions() { return m_n_fermions; };
        int get_n_higgs() { return m_n_higgs; };
        int get_n_b_from_higgs() { return m_n_b_from_higgs; };
        int get_n_c_from_higgs() { return m_n_c_from_higgs; };

        std::vector<EVENT::MCParticle*> resolve_isr_particles(LCCollection *mcp_collection) {
            std::vector<EVENT::MCParticle*> isr_particles;

            for (size_t i = 0; i < m_isr_indices.size(); i++) {
                MCParticle* isr_photon = (MCParticle*)mcp_collection->getElementAt(m_isr_indices[i]);

                assert_true(isr_photon->getPDG() == 22, RESOLVER_ERRORS::ISR_NOT_FOUND);

                isr_particles.push_back(isr_photon);
            }
            
            return isr_particles;
        };
        
        // CAN be overwritten by deriving class definitions
        virtual int get_event_category(std::map<int, int> m_final_state_counts) {
            (void)m_final_state_counts;
            return m_event_category; };
        
        virtual std::vector<int> resolve_higgs_decays(LCCollection *mcp_collection) {
            if (get_n_higgs()) {
                std::vector<int> result;
                std::vector<MCParticle*> fs_particles = resolve_fs_particles(mcp_collection, false);

                for (size_t i = 0; i < fs_particles.size(); i++) {
					if (fs_particles[i]->getPDG() == 25) {
                        result.push_back(fs_particles[i]->getDaughters()[0]->getPDG());
                        result.push_back(fs_particles[i]->getDaughters()[1]->getPDG());
                    }
                }

                return result;
            } else
                return {};
        };

        // MUST be overwritten by deriving class definitions
        virtual std::vector<int> resolve_fs_particle_indices(LCCollection *mcp_collection, bool resolve_higgs = false) = 0;
        virtual std::vector<EVENT::MCParticle*> resolve_fs_particles(LCCollection *mcp_collection, bool resolve_higgs = false) = 0;
        virtual std::vector<int> resolve(LCCollection *mcp_collection) = 0;
};

#endif
