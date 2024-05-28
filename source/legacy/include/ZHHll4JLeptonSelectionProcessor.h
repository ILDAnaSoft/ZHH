#ifndef ZHHll4JLeptonSelectionProcessor_h
#define ZHHll4JLeptonSelectionProcessor_h 1

#include "marlin/Processor.h"
#include "lcio.h"
#include <string>
#include "TH1D.h"
//#include <iostream>
#include <sstream>

#include "TMVA/Reader.h"

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
 * @version $Id: ZHHll4JLeptonSelectionProcessor.h,v 1.4 2005-10-11 12:57:39 gaede Exp $ 
 */

class ZHHll4JLeptonSelectionProcessor : public Processor {
  
 public:
  
  virtual Processor*  newProcessor() { return new ZHHll4JLeptonSelectionProcessor ; }
  
  
  ZHHll4JLeptonSelectionProcessor() ;
  
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
  std::string _colMCP ;
  std::string _colMCTL ;
  std::string _colPFOs ;
  std::string _colNewPFOs ;
  std::string _colLeptons ;
  std::string _colZPFOs ;
  std::string _isolated_electron_weights ;
  std::string _isolated_muon_weights ;

  std::vector<TMVA::Reader*> _readers;
  Float_t _coneec, _coneen, _momentum, _coslarcon, _energyratio;
  Float_t _ratioecal, _ratiototcal, _nsigd0, _nsigz0, _yokeenergy, _totalcalenergy;

  bool _is_lep_tune, _is_training_done;
  int _nRun ;
  int _nEvt ;

  TH1D *hStat;
  std::stringstream gCutName[20];
} ;

#endif



