#ifndef ExpandJetProcessor_h
#define ExpandJetProcessor_h 1

#include <string>
#include <vector>
#include <algorithm>

#include <marlin/Processor.h>
#include <lcio.h>

#include <EVENT/ReconstructedParticle.h>

using namespace lcio;
using namespace marlin;

class ExpandJetProcessor : public Processor{
    public:
        virtual Processor* newProcessor() { return new ExpandJetProcessor; }
        ExpandJetProcessor();
        virtual void init();
        virtual void processRunHeader( LCRunHeader* run );
        virtual void processEvent( LCEvent* evt );
        virtual void check( LCEvent* evt );
        virtual void end();

    protected:
        std::string m_inputJetCollection{};
        std::string m_outputCollection{};
        std::string m_inputPFOCollection{};

    private:
        bool m_PIDAlgoSync{};
        std::vector<std::string> m_PIDAlgorithmsToKeep{};
};
#endif