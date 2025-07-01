#include "MergeCollections.h"

#include <EVENT/LCCollection.h>
#include <IMPL/LCCollectionVec.h>

#include <marlin/VerbosityLevels.h>
#include <UTIL/PIDHandler.h>

using namespace lcio;
using namespace marlin;

MergeCollections aMergeCollections;

MergeCollections::MergeCollections(): Processor("MergeCollections") {

    //processor description
    _description = "MergeCollections takes in up to three collections and merges the PFOs into one collection ";

    //register steering parameters
    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "SourceCollection1",
        "Input collection 1",
        m_sourceCollection1,
        std::string("") );

    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "SourceCollection2",
        "Input collection 2",
        m_sourceCollection2,
        std::string("") );
    
    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "SourceCollection3",
        "Input collection 3",
        m_sourceCollection3,
        std::string("") );

    registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "OutputCollection",
        "Output collection to copy PID entries to. will be newly created and be a subset of SourceCollection",
        m_outputCollection,
        std::string("MergedCollection") );
}

void MergeCollections::init(){
    printParameters();

    for (const auto sourceCollection : {
        m_sourceCollection1,
        m_sourceCollection2,
        m_sourceCollection3
    }) {
        if (sourceCollection != "")
            m_sourceCollections.push_back(sourceCollection);
    }
}

void MergeCollections::processRunHeader( LCRunHeader* run ){
    (void) run;
}

void MergeCollections::processEvent( LCEvent* evt ){
    IMPL::LCCollectionVec* outputPfoCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
    outputPfoCollection->setSubset(true); 

    for (std::string col_name: m_sourceCollections) {
        try {
            EVENT::LCCollection* col_ptr = evt->getCollection(col_name);

            for (int i = 0; i < col_ptr->getNumberOfElements(); i++ ){
                ReconstructedParticle* pfo = dynamic_cast<ReconstructedParticle*> (col_ptr->getElementAt(i));
                outputPfoCollection->addElement(pfo);
            }
        } catch(DataNotAvailableException &e) {
            streamlog_out(ERROR) << "input collection not found (will be skipped): " << col_name << std::endl;
        }
    }

    evt->addCollection(outputPfoCollection, m_outputCollection.c_str());    
}

void MergeCollections::check( LCEvent* evt ){
    (void) evt;
}

void MergeCollections::end(){
}