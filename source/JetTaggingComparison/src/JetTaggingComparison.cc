#include "JetTaggingComparison.h"
#include <stdlib.h>
#include <iostream>
#include "TTree.h"

#include "marlin/VerbosityLevels.h"

using namespace lcio ;
using namespace marlin ;
using namespace std ;

JetTaggingComparison aJetTaggingComparison;

JetTaggingComparison::JetTaggingComparison():
  Processor("JetTaggingComparison"),
  m_n_run(0),
  m_n_evt(0)
{
  _description = "JetTaggingComparison saves PID values for comparison for a specified given collection";

  registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
			  "Collection",
			  "Name of collection whose PIDHandler to take",
			  m_inputCollection,
			  string("RefinedJets")
			  );
  
  registerProcessorParameter("PIDAlgorithm1",
			  "Name of the first PID Handler",
			  m_pidAlgorithm1,
			  string("lcfiplus")
			  );

  registerProcessorParameter("PIDAlgorithm2",
			  "Name of the second PID Handler",
			  m_pidAlgorithm2,
			  string("particlenet")
			  );

  registerProcessorParameter("PIDParameter1",
			  "Name of the argument in the first PID Handler",
			  m_pidParameter1,
			  string("BTag")
			  );

  registerProcessorParameter("PIDParameter2",
			  "Name of the argument in the second PID Handler",
			  m_pidParameter2,
			  string("BTag")
			  );

  registerProcessorParameter("RootFile", "Name of the output root file", m_rootFile, string("JetTaggingComparison.root"));
}

void JetTaggingComparison::init() {
  streamlog_out(DEBUG0) << "   init called  " << endl ;
  printParameters();

  m_pTFile = new TFile(m_rootFile.c_str(), "recreate");
  m_pTTree = new TTree("JetTaggingComparison", "JetTaggingComparison");

  m_pTTree->SetDirectory(m_pTFile);
  m_pTTree->Branch("event", &m_n_evt);
  m_pTTree->Branch("run", &m_n_run);
  m_pTTree->Branch("njet", &m_n_jet);
  m_pTTree->Branch("tag1", &m_tag1);
  m_pTTree->Branch("tag2", &m_tag2);

  this->Clear();

  m_n_run = 0;
  m_n_evt = 0;

  streamlog_out(DEBUG) << "   init finished  " << endl;
}

void JetTaggingComparison::Clear() {
  streamlog_out(DEBUG) << "   Clear called  " << endl;

  m_n_jet = 0;
  m_tag1 = 0.;
  m_tag2 = 0.;
}

void JetTaggingComparison::processRunHeader( LCRunHeader* run ) { 

} 

void JetTaggingComparison::processEvent( EVENT::LCEvent *pLCEvent ) {
  streamlog_out(DEBUG0) << "      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<       processEvent Called     >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl ;
  
  m_n_run = pLCEvent->getRunNumber();
  m_n_evt = pLCEvent->getEventNumber();
  
  streamlog_out(DEBUG4) << "" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////    Processing event        " << m_n_evt << "        ////////////////////" << endl;
  streamlog_out(DEBUG4) << "      ////////////////////////////////////////////////////////////////////////////" << endl;

  this->Clear();

  EVENT::LCCollection *inputCollection{};

  try {
    inputCollection = pLCEvent->getCollection(m_inputCollection);
    streamlog_out(DEBUG7) << "Input collections successfully found in event " << m_n_evt << endl; 

    PIDHandler PIDh(inputCollection);

    int algorithm1IDx = PIDh.getAlgorithmID(m_pidAlgorithm1);
		int parameter1IDx = PIDh.getParameterIndex(algorithm1IDx, m_pidParameter1);

    int algorithm2IDx = PIDh.getAlgorithmID(m_pidAlgorithm2);
		int parameter2IDx = PIDh.getParameterIndex(algorithm2IDx, m_pidParameter2);

    for (size_t j = 0; j < inputCollection->getNumberOfElements(); j++) {
      ReconstructedParticle* object = (ReconstructedParticle*) inputCollection->getElementAt(j);
      
      const ParticleIDImpl& pid1 = dynamic_cast<const ParticleIDImpl&>(PIDh.getParticleID(object, algorithm1IDx));
      const FloatVec& pid1Params = pid1.getParameters();
      m_tag1 = pid1Params[parameter1IDx];

      const ParticleIDImpl& pid2 = dynamic_cast<const ParticleIDImpl&>(PIDh.getParticleID(object, algorithm2IDx));
      const FloatVec& pid2Params = pid2.getParameters();
      m_tag2 = pid2Params[parameter2IDx];

      m_pTTree->Fill();
      m_n_jet++;
		}
  } catch( DataNotAvailableException &e ) {
    streamlog_out(MESSAGE) << "     Input collection not found in event " << m_n_evt << endl;
  };
}

void JetTaggingComparison::check(EVENT::LCEvent *pLCEvent) {
  // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void JetTaggingComparison::end() {
  m_pTFile->cd();
  m_pTTree->Write();
  m_pTFile->Close();
  delete m_pTFile;
}
