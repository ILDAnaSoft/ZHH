#ifndef MergeCollections_h
#define MergeCollections_h 1

#include <string>
#include <vector>
#include <algorithm>

#include <marlin/Processor.h>
#include <lcio.h>

#include <EVENT/ReconstructedParticle.h>

using namespace lcio;
using namespace marlin;

class MergeCollections : public Processor{
    public:
        virtual Processor* newProcessor() { return new MergeCollections; }
        MergeCollections();
        
        virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();

    protected:
        std::string m_sourceCollection1{};
        std::string m_sourceCollection2{};
        std::string m_sourceCollection3{};
        std::vector<std::string> m_sourceCollections{};

        std::string m_outputCollection{};
};
#endif