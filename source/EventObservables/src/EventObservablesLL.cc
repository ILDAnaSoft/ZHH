#include "EventObservablesLL.h"

EventObservablesLL aEventObservablesLL;

EventObservablesLL::EventObservablesLL(): EventObservablesBase("EventObservablesLL") {
    _description = "EventObservablesLL writes relevant observables to root-file " ;
}

void EventObservablesLL::prepareChannelTree() {

};

void EventObservablesLL::clearChannelValues() {

};

void EventObservablesLL::updateChannelValues(EVENT::LCEvent *pLCEvent) {

};