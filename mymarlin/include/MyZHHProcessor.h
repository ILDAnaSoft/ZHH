#ifndef MyZHHProcessor_h
#define MyZHHProcessor_h 1

#include "marlin/Processor.h"
#include "lcio.h"
#include <string>

// Add the AIDA and math headers
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogram1D.h>
#include <cmath>

using namespace lcio ;
using namespace marlin ;


/**  Example processor for marlin.
 * 
 *  If compiled with MARLIN_USE_AIDA 
 *  it creates a histogram (cloud) of the MCParticle energies.
 * 
 *  <h4>Input - Prerequisites</h4>
 *  Needs the collection of MCParticles.
 *
 *  <h4>Output</h4> 
 *  A histogram.
 * 
 * @param CollectionName Name of the MCParticle collection
 * 
 * @author F. Gaede, DESY
 * @version $Id: MyZHHProcessor.h,v 1.4 2005-10-11 12:57:39 gaede Exp $ 
 */

class MyZHHProcessor : public Processor {
  
 public:
  
  virtual Processor*  newProcessor() { return new MyZHHProcessor ; }
  
  
  MyZHHProcessor() ;
  
  /** Called at the begin of the job before anything is read.
   * Use to initialize the processor, e.g. book histograms.
   */
  virtual void init() ;
  
  /** Called for every run.
   */
  virtual void processRunHeader( LCRunHeader* run ) ;
  
  /** Called for every event - the working horse.
   */
  virtual void processEvent( LCEvent * evt ) ; 
  
  
  virtual void check( LCEvent * evt ) ; 
  
  
  /** Called after data processing for clean up.
   */
  virtual void end() ;
  
  
 protected:

  /** Input collection name.
   */
  std::string _colName{} ;

  int _nRun{} ;
  int _nEvt{} ;

  // Variables to hold the names of the collections we want to read
  std::string _mcParticleCollectionName{};
  std::string _recoParticleCollectionName{};

  // Pointers to our ROOT Histograms
  AIDA::IHistogram1D* _h_mc_energy{};
  AIDA::IHistogram1D* _h_reco_energy{};
  AIDA::IHistogram1D* _h_reco_pt{};
} ;

#endif



