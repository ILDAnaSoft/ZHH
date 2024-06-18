#include "FinalStateResolver.h"
#include "common.h"

using namespace marlin;
using namespace lcio;

FinalStateResolver::FinalStateResolver(
    int process_id,
    int event_category
){
    m_process = process_id;
    m_event_category = event_category;
};

FinalStateResolver::pdg_of_particle(EVENT::LCObject* particle) {
    return (dynamic_cast<EVENT::MCParticle*>(particle))->getPDG();
};
