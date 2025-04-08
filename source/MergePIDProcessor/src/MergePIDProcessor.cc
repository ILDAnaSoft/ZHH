#include "MergePIDProcessor.h"

#include <EVENT/LCCollection.h>
#include <IMPL/LCCollectionVec.h>

#include <marlin/VerbosityLevels.h>
#include <UTIL/PIDHandler.h>

using namespace lcio;
using namespace marlin;

MergePIDProcessor aMergePIDProcessor;

MergePIDProcessor::MergePIDProcessor(): Processor("MergePIDProcessor"),
m_do_nothing(false) {

    //processor description
    _description = "MergePIDProcessor takes in two collections (from, to; to subset of from) of ReconstructedParticle objects and copies PID parameters from the one to the other collection. ";

    //register steering parameters
    registerInputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "SourceCollection",
        "Input collection of PID information",
        m_sourceCollection,
        std::string("PandoraPFOs") );

    registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "TargetCollection",
        "Input collection of PFOs to apply the PID entries on. must be subse of SourceCollection",
        m_targetCollection,
        std::string("PFOsWithoutOverlay") );

    registerOutputCollection( LCIO::RECONSTRUCTEDPARTICLE,
        "OutputCollection",
        "Output collection to copy PID entries to. will be newly created and be a subset of SourceCollection",
        m_outputCollection,
        std::string("PFOsWithoutOverlayWPID") );

  	registerProcessorParameter("PIDAlgosToKeep", "list of PID Algo names whose parameters to sync. will sync all if empty. ", m_PIDAlgorithmsToKeep, std::vector<std::string>{});
}

void MergePIDProcessor::init(){
    printParameters();

    m_do_nothing = m_sourceCollection == m_outputCollection;
}

void MergePIDProcessor::processRunHeader( LCRunHeader* run ){
    (void) run;
}

void MergePIDProcessor::processEvent( LCEvent* evt ){
    if (m_do_nothing)
        return;

    try{
        const EVENT::LCCollection* source_col = evt->getCollection(m_sourceCollection);
        const EVENT::LCCollection *target_col = evt->getCollection(m_targetCollection);

        IMPL::LCCollectionVec* outputPfoCollection = new IMPL::LCCollectionVec(LCIO::RECONSTRUCTEDPARTICLE);
        outputPfoCollection->setSubset(true); 

        PIDHandler PIDs(source_col);
        PIDHandler PIDt(outputPfoCollection);        
        
        for (int i = 0; i < target_col->getNumberOfElements(); i++ ){
            ReconstructedParticle* pfo = dynamic_cast<ReconstructedParticle*> (target_col->getElementAt(i));

            outputPfoCollection->addElement(pfo);

            //std::cerr << "PFO #" << i << std::endl;            

            std::map<std::string, int> pid_algo_mapping;
            EVENT::IntVec target_algo_ids = PIDt.getAlgorithmIDs();
            for (size_t j = 0; j < target_algo_ids.size(); j++) {
                pid_algo_mapping[PIDt.getAlgorithmName(target_algo_ids[j])] = target_algo_ids[j];
            }

            EVENT::IntVec source_algo_ids = PIDs.getAlgorithmIDs();
            std::map<int, int> source_to_target_algo_ids;
            bool sync_all = m_PIDAlgorithmsToKeep.size() == 0;

            for (size_t j = 0; j < source_algo_ids.size(); j++) {
                //std::cerr << "LOOP_START" << std::endl;
                int sourceAlgoID = source_algo_ids[j];
                //std::cerr << "sync_all: " << sync_all << " sourceAlgoID: " << sourceAlgoID << std::endl;
                EVENT::ParticleIDVec source_pids = PIDs.getParticleIDs(pfo, sourceAlgoID);

                if (source_pids.size()) {
                    int targetAlgoID;
                    //std::cerr << "A" << std::endl;

                    EVENT::ParticleID* source_pid = source_pids.at(0);
                    std::string source_algo_name = PIDs.getAlgorithmName(sourceAlgoID);

                    if (!pid_algo_mapping.contains(source_algo_name)) {
                        //std::cerr << "B" << std::endl;

                        // skip this algo if neither sync_all = True nor the algo is not in the list of algos to be synced
                        if (!sync_all && std::find(m_PIDAlgorithmsToKeep.begin(), m_PIDAlgorithmsToKeep.end(), source_algo_name) == m_PIDAlgorithmsToKeep.end())
                            continue;

                        targetAlgoID = PIDt.addAlgorithm(source_algo_name, PIDs.getParameterNames(sourceAlgoID));

                        streamlog_out(MESSAGE) << "Registered algo " << source_algo_name << " (source->target algoIDs: " << sourceAlgoID << "->" << targetAlgoID << ") with " << PIDs.getParameterNames(sourceAlgoID).size() << " parameters" << std::endl;

                        pid_algo_mapping.insert({source_algo_name, targetAlgoID});
                    } else {
                        targetAlgoID = pid_algo_mapping[source_algo_name];
                    }

                    //std::cerr << "targetAlgo " << source_algo_name << " (" << sourceAlgoID << "->" << targetAlgoID << "): " << source_pid->getParameters().size() << std::endl;
                    
                    size_t n_params = source_pid->getParameters().size();
                    if (n_params && n_params == PIDt.getParameterNames(targetAlgoID).size()) {
                        //std::cerr << "E" << std::endl;
                        PIDt.setParticleID(pfo, source_pid->getType(), source_pid->getPDG(), source_pid->getLikelihood(), targetAlgoID, source_pid->getParameters());
                    }
                }
            }
        }

        //std::cerr << "EVENT DONE" << std::endl;

        evt->addCollection(outputPfoCollection, m_outputCollection.c_str());
    } catch(DataNotAvailableException &e){
        streamlog_out(ERROR) << "inputs collections not found " << std::endl;
    }
    

    
}

void MergePIDProcessor::check( LCEvent* evt ){
    (void) evt;
}

void MergePIDProcessor::end(){
}