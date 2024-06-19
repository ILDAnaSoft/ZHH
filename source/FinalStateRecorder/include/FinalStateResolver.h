#ifndef FinalStateResolver_h
#define FinalStateResolver_h 1

#include "marlin/Processor.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "vector"
#include "string"

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
        std::string m_process_name;
        int m_process_id;
        int m_event_category;

        // Helper functions
        int pdg_of_particle(EVENT::LCObject* particle);
        std::vector<int> pdgs_of_nth_hadronic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_leptonic_decay(LCCollection *mcp_collection, int n);
        std::vector<int> pdgs_of_nth_semilept_decay(LCCollection *mcp_collection, int n);

    public:
        FinalStateResolver(std::string process_name, int process_id, int event_category);
        ~FinalStateResolver();
        
        virtual std::vector<int> resolve_event(LCCollection *mcp_collection);

};

#endif
