#ifndef MergePIDProcessor_h
#define MergePIDProcessor_h 1

#include <string>
#include <vector>
#include <algorithm>

#include <marlin/Processor.h>
#include <lcio.h>

#include <EVENT/ReconstructedParticle.h>

using namespace lcio;
using namespace marlin;

class MergePIDProcessor : public Processor{
    public:
        virtual Processor* newProcessor() { return new MergePIDProcessor; }
        MergePIDProcessor();
        virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();

    protected:
        std::string m_sourceCollection{};
        std::string m_targetCollection{};
        std::string m_outputCollection{};

    private:
        std::vector<std::string> m_PIDAlgorithmsToKeep{};
};
#endif