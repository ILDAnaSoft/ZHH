#ifdef __CLING__
R__LOAD_LIBRARY(liblcioDict)
#endif

#include "IO/LCReader.h"
#include "IOIMPL/LCFactory.h"
#include "EVENT/MCParticle.h"
#include "UTIL/LCIterator.h"
#include "lcio.h"
#include <iostream>

void read_slcio() {
    using namespace lcio;
    
    auto* lcReader = IOIMPL::LCFactory::getInstance()->createLCReader();
    lcReader->open("piplus_gun_SIM.slcio");
    
    while(auto* evt = lcReader->readNextEvent()) {
        LCIterator<MCParticle> mcParticles(evt, "MCParticle");
        
        std::cout << "Event " << evt->getEventNumber() 
                  << " has " << mcParticles.size() << " MCParticles." << std::endl;
    }
}
