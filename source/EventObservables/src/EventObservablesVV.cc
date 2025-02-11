#include "EventObservablesVV.h"

EventObservablesVV aEventObservablesVV;

EventObservablesVV::EventObservablesVV(): EventObservablesBase("EventObservablesVV") {
    _description = "EventObservablesVV writes relevant observables to root-file " ;
}

void EventObservablesVV::prepareChannelTree() {

};

void EventObservablesVV::clearChannelValues() {

};

void EventObservablesVV::updateChannelValues(EVENT::LCEvent *pLCEvent) {

};