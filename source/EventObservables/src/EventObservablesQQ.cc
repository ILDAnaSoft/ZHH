#include "EventObservablesQQ.h"

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ"),
m_JMP("best_perm_qq") {
    _description = "EventObservablesQQ writes relevant observables to root-file " ;
}

void EventObservablesQQ::prepareChannelTree() {

};

void EventObservablesQQ::clearChannelValues() {

};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {

};