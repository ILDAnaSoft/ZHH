#include "EventObservablesQQ.h"

EventObservablesQQ aEventObservablesQQ;

EventObservablesQQ::EventObservablesQQ(): EventObservablesBase("EventObservablesQQ") {
    _description = "EventObservablesQQ writes relevant observables to root-file " ;
}

void EventObservablesQQ::prepareChannelTree() {

};

void EventObservablesQQ::clearChannelValues() {

};

void EventObservablesQQ::updateChannelValues(EVENT::LCEvent *pLCEvent) {

};