#ifndef FinalStates_ffhh_h
#define FinalStates_ffhh_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffhh: public FinalStateResolver {
    protected:
        vector<int> m_z_decay_filter;

    public:
        // Set process ID and event category
        ffhh( string process_name, int process_id, int event_category, vector<int> z_decay_filter ): FinalStateResolver( process_name, process_id, event_category, 2, 2 ) {
            m_z_decay_filter = z_decay_filter;
        };

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get Z-decayed fermions
            MCParticle* f1 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* f2 = (MCParticle*)mcp_collection->getElementAt(9);

            assert_true(
                vec_contains(m_z_decay_filter, abs(f1->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(f2->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            // Get H-decayed fermions
            MCParticle* h1 = (MCParticle*)mcp_collection->getElementAt(10);
            MCParticle* h2 = (MCParticle*)mcp_collection->getElementAt(11);

            assert_true(h1->getPDG() == 25 && h2->getPDG() == 25, RESOLVER_ERRORS::HIGGS_NOT_FOUND);

            vector<int> d1 = pdgs_of_daughter_particles(h1);
            vector<int> d2 = pdgs_of_daughter_particles(h2);

            assert_true(d1.size() == 2 && d2.size() == 2, RESOLVER_ERRORS::UNEXPECTED_CHILDREN);            

            return vector<int>{
                f1->getPDG(),
                f2->getPDG(),
                d1[0],
                d1[1],
                d2[0],
                d2[1]
            };
        };

};


class e1e1hh : public ffhh {
    public: e1e1hh(): ffhh( "e1e1hh", PROCESS_ID::e1e1hh, EVENT_CATEGORY_TRUE::llHH, vector{11} ) {}; };

class e2e2hh : public ffhh {
    public: e2e2hh(): ffhh( "e2e2hh", PROCESS_ID::e2e2hh, EVENT_CATEGORY_TRUE::llHH, vector{13} ) {}; };

class e3e3hh : public ffhh {
    public: e3e3hh(): ffhh( "e3e3hh", PROCESS_ID::e3e3hh, EVENT_CATEGORY_TRUE::llHH, vector{15} ) {}; };

class qqhh : public ffhh {
    public: qqhh(): ffhh( "qqhh", PROCESS_ID::qqhh, EVENT_CATEGORY_TRUE::qqHH, vector{1,2,3,4,5,6} ) {}; };

class n1n1hh : public ffhh {
    public: n1n1hh(): ffhh( "n1n1hh", PROCESS_ID::n1n1hh, EVENT_CATEGORY_TRUE::vvHH, vector{12} ) {}; };

class n23n23hh : public ffhh {
    public: n23n23hh(): ffhh( "n23n23hh", PROCESS_ID::n23n23hh, EVENT_CATEGORY_TRUE::vvHH, vector{14, 16} ) {}; };


#endif



