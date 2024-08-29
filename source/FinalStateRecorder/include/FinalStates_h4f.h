#ifndef FinalStates_ffffh_h
#define FinalStates_ffffh_h 1

#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

using namespace std;

class ffffh: public p6 {
    protected:
        vector<int> m_z_decay_filter;

    public:
        // Set process ID and event category
        ffffh( string process_name, int process_id, int event_category, vector<int> z_decay_filter ):
            p6( process_name, process_id, event_category, 4, 1 ),
            m_z_decay_filter { z_decay_filter } {};

        vector<int> m_resolve(LCCollection *mcp_collection) {
            // Get Z-decayed fermions
            MCParticle* part1 = (MCParticle*)mcp_collection->getElementAt(8);
            MCParticle* part2 = (MCParticle*)mcp_collection->getElementAt(9);
            MCParticle* part3 = (MCParticle*)mcp_collection->getElementAt(10);
            MCParticle* part4 = (MCParticle*)mcp_collection->getElementAt(11);

            assert_true(
                vec_contains(m_z_decay_filter, abs(part1->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(part2->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(part3->getPDG())) &&
                vec_contains(m_z_decay_filter, abs(part4->getPDG())), RESOLVER_ERRORS::UNALLOWED_VALUES);

            // Get H-decayed fermions
            MCParticle* h1 = (MCParticle*)mcp_collection->getElementAt(12);

            assert_true(h1->getPDG() == 25, RESOLVER_ERRORS::HIGGS_NOT_FOUND);

            vector<int> d1 = pdgs_of_daughter_particles(h1);

            assert_true(d1.size() == 2, RESOLVER_ERRORS::UNEXPECTED_CHILDREN);            

            return vector<int>{
                part1->getPDG(),
                part2->getPDG(),
                part3->getPDG(),
                part4->getPDG(),
                d1[0],
                d1[1]
            };
        };

};


class e1e1qqh : public ffffh {
    public: e1e1qqh(): ffffh( "e1e1qqh", PROCESS_ID::e1e1qqh, EVENT_CATEGORY_TRUE::llqqH, vector{11,1,2,3,4,5,6} ) {}; };

class e2e2qqh : public ffffh {
    public: e2e2qqh(): ffffh( "e2e2qqh", PROCESS_ID::e2e2qqh, EVENT_CATEGORY_TRUE::llqqH, vector{13,1,2,3,4,5,6} ) {}; };

class e3e3qqh : public ffffh {
    public: e3e3qqh(): ffffh( "e3e3qqh", PROCESS_ID::e3e3qqh, EVENT_CATEGORY_TRUE::llqqH, vector{15,1,2,3,4,5,6} ) {}; };

class qqqqh : public ffffh {
    public: qqqqh(): ffffh( "qqqqh", PROCESS_ID::qqqqh, EVENT_CATEGORY_TRUE::qqqqH, vector{1,2,3,4,5,6} ) {}; };

class n1n1qqh : public ffffh {
    public: n1n1qqh(): ffffh( "n1n1qqh", PROCESS_ID::n1n1qqh, EVENT_CATEGORY_TRUE::vvqqH, vector{12,1,2,3,4,5,6} ) {}; };

class n23n23qqh : public ffffh {
    public: n23n23qqh(): ffffh( "n23n23qqh", PROCESS_ID::n23n23qqh, EVENT_CATEGORY_TRUE::vvqqH, vector{14,16,1,2,3,4,5,6} ) {}; };

#endif
