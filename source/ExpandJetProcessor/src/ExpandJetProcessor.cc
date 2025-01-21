#include "ExpandJetProcessor.h"

#include <EVENT/LCCollection.h>
#include <IMPL/LCCollectionVec.h>

#include <marlin/VerbosityLevels.h>
#include <UTIL/PIDHandler.h>

using namespace lcio;
using namespace marlin;

void sync_pids(
	const EVENT::LCCollection* PFOs,
	IMPL::LCCollectionVec* OutputPfoCollection,
	EVENT::LCObject* pfo_in,
	ReconstructedParticle* pfo_out,
	std::map<int, int> *pid_algo_mapping,
	std::vector<std::string> pid_algo_names_to_keep)
{
	PIDHandler PIDs(PFOs);
	PIDHandler PIDt(OutputPfoCollection);

	EVENT::IntVec algorithmIDs = PIDs.getAlgorithmIDs();
	std::map<int, int> source_to_target_algo_ids;
	bool sync_all = pid_algo_names_to_keep.size() == 0;

	for (size_t i = 0; i < algorithmIDs.size(); i++) {
		int sourceAlgoID = algorithmIDs[i];
		EVENT::ParticleIDVec source_pids = PIDs.getParticleIDs(pfo_in, sourceAlgoID);

		if (source_pids.size()) {
			int targetAlgoID;

			EVENT::ParticleID* source_pid = source_pids.at(0);

			if (!pid_algo_mapping->contains(sourceAlgoID)) {
				std::string algo_name = PIDs.getAlgorithmName(sourceAlgoID);
				
				// skip this algo if neither sync_all = True nor the algo is not in the list of algos to be synced
				if (!sync_all && std::find(pid_algo_names_to_keep.begin(), pid_algo_names_to_keep.end(), algo_name) == pid_algo_names_to_keep.end())
					continue;

				targetAlgoID = PIDt.addAlgorithm(algo_name, PIDs.getParameterNames(sourceAlgoID));
				pid_algo_mapping->insert({sourceAlgoID, targetAlgoID});
			} else {
				targetAlgoID = (*pid_algo_mapping)[sourceAlgoID];
			}

			if (source_pid->getParameters().size() == PIDt.getParameterNames(targetAlgoID).size()) {
				PIDt.setParticleID(pfo_out, source_pid->getType(), source_pid->getPDG(), source_pid->getLikelihood(), targetAlgoID, source_pid->getParameters());
			}
		}

	}	
}

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