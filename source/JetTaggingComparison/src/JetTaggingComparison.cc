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

  registerProcessorParameter("PIDAlgorithm3",
			  "Name of the second PID Handler",
			  m_pidAlgorithm3,
			  string("")
			  );

  registerProcessorParameter("PIDParameters1",
			  "Names of parameters to read for the first PID Handler",
			  m_pidParameters1,
			  std::vector<std::string>{"BTag"}
			  );

  registerProcessorParameter("PIDParameters2",
			  "Names of parameters to read for the second PID Handler",
			  m_pidParameters2,
			  std::vector<std::string>{"BTag"}
			  );

  registerProcessorParameter("PIDParameters3",
			  "Names of parameters to read for the third PID Handler",
			  m_pidParameters3,
			  std::vector<std::string>{}
			  );

  registerProcessorParameter("RootFile", "Name of the output root file. set to empty, this will output to AIDA", m_rootFile, string(""));
}

void JetTaggingComparison::init() {
  streamlog_out(DEBUG0) << "   init called  " << endl ;
  printParameters();

  if (m_rootFile.size()) {
    m_pTFile = new TFile(m_rootFile.c_str(), "recreate");
    m_pTTree->SetDirectory(m_pTFile);
  }

  m_pTTree->Branch("event", &m_n_evt);
  m_pTTree->Branch("run", &m_n_run);
  m_pTTree->Branch("njet", &m_n_jet);
  m_pTTree->Branch("energy", &m_jet_energy);

  m_read_algo2 = m_pidAlgorithm2 != "";
  m_read_algo3 = m_pidAlgorithm3 != "";

  m_pTTree->Branch("tags1", &m_tags1);

  if (m_read_algo2)
    m_pTTree->Branch("tags2", &m_tags2);

  if (m_read_algo3)
    m_pTTree->Branch("tags3", &m_tags3);

  this->Clear();

  m_n_run = 0;
  m_n_evt = 0;

  streamlog_out(DEBUG) << "   init finished  " << endl;
}

void JetTaggingComparison::Clear() {
  streamlog_out(DEBUG) << "   Clear called  " << endl;

  m_n_jet = 0;
  m_tags1.clear();
  m_tags2.clear();
  m_tags3.clear();

  m_parametersIDs1.clear();
  m_parametersIDs2.clear();
  m_parametersIDs3.clear();
}

void JetTaggingComparison::processRunHeader( LCRunHeader* run ) { 
  (void) run;
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
    for (size_t i = 0; i < m_pidParameters1.size(); i++)
      m_parametersIDs1.push_back(PIDh.getParameterIndex(algorithm1IDx, m_pidParameters1[i]));

    int algorithm2IDx = -1;
    int algorithm3IDx = -1;

    if (m_read_algo2) {
      algorithm2IDx = PIDh.getAlgorithmID(m_pidAlgorithm2);
      for (size_t i = 0; i < m_pidParameters2.size(); i++)
        m_parametersIDs2.push_back(PIDh.getParameterIndex(algorithm2IDx, m_pidParameters2[i]));
    }

    if (m_read_algo3) {
      algorithm3IDx = PIDh.getAlgorithmID(m_pidAlgorithm3);
      for (size_t i = 0; i < m_pidParameters3.size(); i++)
        m_parametersIDs3.push_back(PIDh.getParameterIndex(algorithm3IDx, m_pidParameters3[i]));
    }

    for (int j = 0; j < inputCollection->getNumberOfElements(); j++) {
      ReconstructedParticle* jet = (ReconstructedParticle*) inputCollection->getElementAt(j);
      
      const ParticleIDImpl& pid1 = dynamic_cast<const ParticleIDImpl&>(PIDh.getParticleID(jet, algorithm1IDx));
      const FloatVec& pid1Params = pid1.getParameters();
      for (size_t i = 0; i < m_parametersIDs1.size(); i++) {
        m_tags1.push_back(pid1Params[m_parametersIDs1[i]]);
      }

      if (algorithm2IDx != -1) {
        const ParticleIDImpl& pid2 = dynamic_cast<const ParticleIDImpl&>(PIDh.getParticleID(jet, algorithm2IDx));
        const FloatVec& pid2Params = pid2.getParameters();
        for (size_t i = 0; i < m_parametersIDs2.size(); i++) {
          m_tags2.push_back(pid2Params[m_parametersIDs2[i]]);
        }
      }

      if (algorithm3IDx != -1) {
        const ParticleIDImpl& pid3 = dynamic_cast<const ParticleIDImpl&>(PIDh.getParticleID(jet, algorithm3IDx));
        const FloatVec& pid3Params = pid3.getParameters();
        for (size_t i = 0; i < m_parametersIDs3.size(); i++) {
          m_tags3.push_back(pid3Params[m_parametersIDs3[i]]);
        }
      }
      
      m_jet_energy = jet->getEnergy();
      m_pTTree->Fill();

      m_n_jet++;

      m_tags1.clear();
      m_tags2.clear();
      m_tags3.clear();
		}
  } catch( DataNotAvailableException &e ) {
    streamlog_out(MESSAGE) << "     Input collection not found in event " << m_n_evt << endl;
  };
}

void JetTaggingComparison::check(EVENT::LCEvent *pLCEvent) {
  // nothing to check here - could be used to fill checkplots in reconstruction processor
  (void) pLCEvent;
}


void JetTaggingComparison::end() {
  if (m_rootFile.size()) {
    m_pTFile->cd();
  }
  
  m_pTTree->Write();
  
  if (m_rootFile.size()) {
    m_pTFile->Close();
    delete m_pTFile;
  }
}
