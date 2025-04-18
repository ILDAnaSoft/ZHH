#ifndef ZHH_sync_pids_h
#define ZHH_sync_pids_h 1

#include <UTIL/PIDHandler.h>
#include "EVENT/LCCollection.h"
#include "EVENT/ReconstructedParticle.h"
#include "IMPL/LCCollectionVec.h"

// use inline for header-only stuff
inline void sync_pids(
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

#endif