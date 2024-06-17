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
        std::vector<int> m_allowed_values;

        /* data */
    public:
        FinalStateResolver(int process, std::vector<int> allowed_values);
        ~FinalStateResolver();
        
        virtual std::vector<int> m_resolve(EVENT::LCEvent *pLCEvent);
        
        std::vector<int> resolveEvent(EVENT::LCEvent *pLCEvent);
};

#endif
