#include "FinalStateResolver.h"
#include <EVENT/MCParticle.h>

using namespace marlin;
using namespace lcio;

FinalStateResolver::FinalStateResolver(
    std::string process_name,
    int process_id,
    int event_category,
    int n_fermions,
    int n_higgs,
    std::vector<int> isr_indices
):
    m_process_name(process_name),
    m_process_id(process_id),
    m_event_category(event_category),
    m_n_fermions(n_fermions),
    m_n_higgs(n_higgs),
    m_isr_indices(isr_indices) {};
FinalStateResolver::~FinalStateResolver() {};

int FinalStateResolver::pdg_of_particle(EVENT::LCObject* particle) {
    return (dynamic_cast<EVENT::MCParticle*>(particle))->getPDG();
};

std::vector<int> FinalStateResolver::pdgs_of_daughter_particles(EVENT::LCObject* particle) {
    return pdgs_of_daughter_particles((EVENT::MCParticle*)particle);
};

std::vector<int> FinalStateResolver::pdgs_of_daughter_particles(EVENT::MCParticle* particle) {
    std::vector<int> res;
    auto daughters = particle->getDaughters();

    for (size_t i = 0; i < daughters.size(); i++)
        res.push_back(abs(daughters[i]->getPDG()));

    return res;
};


std::vector<int> FinalStateResolver::pdgs_of_nth_hadronic_decay(LCCollection *mcp_collection, int n) {
    std::vector<int> res;

    for (int i = 0; i < mcp_collection->getNumberOfElements(); i++) {
        auto particle = mcp_collection->getElementAt(i);
        if (pdg_of_particle(particle) == 25) {
            auto decay = pdgs_of_nth_hadronic_decay(mcp_collection, n - 1);
            res.insert(res.end(), decay.begin(), decay.end());
        }
    }

    return res;
};