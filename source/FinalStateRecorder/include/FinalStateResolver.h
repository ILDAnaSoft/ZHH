#ifndef FinalStateResolver_h
#define FinalStateResolver_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "vector"

using namespace lcio ;
using namespace marlin ;

struct RESOLVER_ERRORS {
    enum Values: unsigned int {
        OK = 0,
        UNKNOWN_ERROR = 1,
        UNALLOWED_VALUES = 2
    };
};

class FinalStateResolver {
    protected:
        int m_process;
        int m_event_category;

        /* data */
    public:
        FinalStateResolver(int process_id, int event_category);
        ~FinalStateResolver();
        
        virtual std::vector<int> resolve_event(LCCollection *mcp_collection);
        int pdg_of_particle(EVENT::LCObject* particle);
};

#endif
