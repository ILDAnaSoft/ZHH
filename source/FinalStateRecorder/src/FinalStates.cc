#include "FinalStateResolver.h"
#include "common.h"

class e1e1hh : public FinalStateResolver {
    public:
        // Set process ID and event category
        e1e1hh(): FinalStateResolver( PROCESS_ID::e1e1hh, EVENT_CATEGORY_TRUE::llHH ) {};

        std::vector<int> m_resolve(LCCollection *mcp_collection) {
            return std::vector<int>{
                mcp_collection->getElementAt(8),
                mcp_collection.getElementAt(9).getPDG()
            };
        };
};

