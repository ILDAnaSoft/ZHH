#include "FinalStateResolver.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/MCParticle.h>
#include "common.h"

class e1e1hh : public FinalStateResolver {
    public:
        // Set process ID and event category
        e1e1hh(): FinalStateResolver( "e1e1hh", PROCESS_ID::e1e1hh, EVENT_CATEGORY_TRUE::llHH ) {};

        std::vector<int> m_resolve(LCCollection *mcp_collection) {
            return std::vector<int>{
                pdg_of_particle(mcp_collection->getElementAt(0)),
                pdg_of_particle(mcp_collection->getElementAt(1)),
            };
        };
};

