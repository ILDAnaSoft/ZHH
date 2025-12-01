#include "lcio.h"
#include <stdio.h>
#include <algorithm>

#include "IO/LCReader.h"
#include "IMPL/LCTOOLS.h"
#include "EVENT/LCRunHeader.h" 

#include "EVENT/SimCalorimeterHit.h" 
#include "EVENT/CalorimeterHit.h" 
#include "EVENT/RawCalorimeterHit.h" 
// #include "EVENT/SimTrackerHit.h" 

#include "UTIL/CellIDDecoder.h"

#include <cstdlib>

using namespace std ;
using namespace lcio ;


// from https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

int main(int argc, char** argv ){
  char* FILES ;

  // read file name from command line (only argument) 
  if( argc < 1 ) {

    cout << " usage: lcio_get_physics_meta filename" << endl ;
    cout << "    or: lcio_get_physics_meta filename1,filename2,[...] " << endl ;
    cout << "  where the first dumps the event with the specified run and event number" << endl ;
    cout << "  and the second simply dumps the n-th event in the file" << endl << endl ;
    cout << "  set the environment variable LCIO_READ_COL_NAMES to a space separated list" << endl ;
    cout << "  of collection names that you would like to dump (all are dumped if not set)" << endl ;

    exit(1) ;
  }
  
  int nFile = 0;
  FILES = argv[1] ;

  std::vector<std::string> paths = split(FILES, ",");

  for (std::string path: paths) {
    LCReader* lcReader = LCFactory::getInstance()->createLCReader(LCReader::directAccess) ;
    LCEvent* evt(0) ;

    try{
      lcReader->open( path ) ;
      //lcReader->skipNEvents(0) ;
      evt = lcReader->readNextEvent() ;

      std::string mcParticleCollection = "";
      int nRun = 0;
      int nEvt = 0;
      float beamPol1 = 0.;
      float beamPol2 = 0.;
      float Pol0 = 0.;
      float Pol1 = 0.;
      float crossSection = 0.;
      float crossSectionErr = 0.;
      int processId = 0;
      std::string processName = "";

      if( !evt  ){
        cout << " file " << path << " does not contain any events" << endl ;    
        exit(1) ;
      }

      const EVENT::LCParameters& parameters = evt->getParameters();

      nRun = evt->getRunNumber();
      nEvt = lcReader->getNumberOfEvents();

      // find MCParticle collection
      const EVENT::StringVec* collections = evt->getCollectionNames();

      if(std::find(collections->begin(), collections->end(), "MCParticlesSkimmed") != collections->end()) {
          mcParticleCollection = "MCParticlesSkimmed";
      } else if(std::find(collections->begin(), collections->end(), "MCParticle") != collections->end()) {
          mcParticleCollection = "MCParticle";
      } else {
        cerr << "Could not find any MCParticle collection: Neither MCParticlesSkimmed nor MCParticle exists ";
        exit(1);
      }

      beamPol1 = evt->getParameters().getFloatVal("beamPol1");
      beamPol2 = evt->getParameters().getFloatVal("beamPol2");
      Pol0 = evt->getParameters().getFloatVal("Pol0");
      Pol1 = evt->getParameters().getFloatVal("Pol1");

      crossSection = parameters.getFloatVal("crossSection");
      crossSectionErr = parameters.getFloatVal("crossSectionError");
      processId = parameters.getIntVal("ProcessID");
      processName = parameters.getStringVal("processName");

      //cout << "event " << nEvt << std::endl;

      if (nFile)
        cout << std::endl;

      cout << processName << "," << nEvt << "," << nRun << ","
          << Pol0 << "," << Pol1 << ","
          << beamPol1 << "," << beamPol2 << ","
          << crossSection << "," << crossSectionErr << ","
          << processId << "," << mcParticleCollection;

      /*
      params.getStringVal('processName'), n_events, run_number, \
      float(params.getFloatVal('Pol0')), float(params.getFloatVal('Pol1')), \
      float(params.getFloatVal('beamPol1')), float(params.getFloatVal('beamPol2')), \
      float(params.getFloatVal('crossSection')), float(params.getFloatVal('crossSectionError')), \
      int(params.getIntVal('ProcessID')), ''+mcp_col_name
      */

      lcReader->close() ;
    } catch( IOException& e) {
      cout << e.what() << endl ;
    }

    nFile++;
  }

   return 0 ;
}