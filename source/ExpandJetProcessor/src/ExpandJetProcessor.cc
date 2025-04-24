#include "ExpandJetProcessor.h"

#include <EVENT/LCCollection.h>
#include <IMPL/LCCollectionVec.h>

#include <marlin/VerbosityLevels.h>
#include <UTIL/PIDHandler.h>
#include "sync_pids.h"

using namespace lcio;
using namespace marlin;

ExpandJetProcessor aExpandJetProcessor;

ExpandJetProcessor::ExpandJetProcessor()
    :Processor("ExpandJetProcessor"){

    //processor description
    _description = "ExpandJetProcessor takes in a collection of jets and returns a flattened collection of declustered ReconstructedParticle objects ";

    //register steering parameters
    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "InputCollection",
        "Input collection of jets to decluster",
        m_inputJetCollection,
        std::string("PFOsMinusOverlayJets") );

    registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "OutputCollection",
        "Output collection of declustered PFOs",
        m_outputCollection,
        std::string("PFOsAfterGamGamRemoval") );

    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "RecoParticleCollection",
        "Input collection of ReconstructedParticle objects. required for the PIDHandler, if PIDAlgoSync=true",
        m_inputPFOCollection,
        std::string("PandoraPFOs") );

    registerProcessorParameter("PIDAlgoSync", "Whether or not to sync PIDs. if false, the PID parameters will not be synced", m_PIDAlgoSync, false);
  	registerProcessorParameter("PIDAlgosToKeep", "list of PID Algo names whose parameters to sync. will sync all if empty. ", m_PIDAlgorithmsToKeep, std::vector<std::string>{});
}

void ExpandJetProcessor::init(){
    printParameters();
}

void ExpandJetProcessor::processRunHeader( LCRunHeader* run ){
    (void) run;
}

void ExpandJetProcessor::processEvent( LCEvent* evt ){
    LCCollection* jetCol = 0;
    const EVENT::LCCollection *inputPFOs = 0;

    try{
        jetCol = evt->getCollection(m_inputJetCollection);

        if (m_PIDAlgoSync) {
            inputPFOs = evt->getCollection(m_inputPFOCollection);
        }
    } catch(...){
        std::cout << "no events in JetsAfterGamGamRemoval" << std::endl;
    }
    LCCollectionVec* outCol = new LCCollectionVec( LCIO::RECONSTRUCTEDPARTICLE );
    outCol->setSubset( true );

    if( jetCol != 0){
        unsigned int njet = jetCol->getNumberOfElements();
        std::map<int, int> pid_algo_mapping;
        
        for( unsigned int i = 0; i < njet; i++ ){
            ReconstructedParticle* jet = dynamic_cast< ReconstructedParticle* >( jetCol->getElementAt(i) );
            const ReconstructedParticleVec& pfovec = jet->getParticles();
            for( unsigned int j = 0; j < pfovec.size(); j++ ){
                outCol->addElement( pfovec[j] );

                if (m_PIDAlgoSync) {
                    sync_pids(inputPFOs, outCol, pfovec[j], pfovec[j], &pid_algo_mapping, m_PIDAlgorithmsToKeep);
                }
            }
        }
    }

    evt->addCollection( outCol, m_outputCollection.c_str() );
}

void ExpandJetProcessor::check( LCEvent* evt ){
    (void) evt;
}

void ExpandJetProcessor::end(){
}