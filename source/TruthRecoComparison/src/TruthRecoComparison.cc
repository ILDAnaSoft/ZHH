#include "TruthRecoComparison.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <ctime>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/LCIntVec.h>
#include <IMPL/ReconstructedParticleImpl.h>
#include <UTIL/PIDHandler.h>

using namespace lcio ;
using namespace marlin ;

ROOT::Math::PxPyPzEVector vec4(const EVENT::MCParticle* mcp) {
	return ROOT::Math::PxPyPzEVector(mcp->getMomentum()[0], mcp->getMomentum()[1], mcp->getMomentum()[2], mcp->getEnergy());
}
ROOT::Math::PxPyPzEVector vec4(const EVENT::ReconstructedParticle* pfo) {
	return ROOT::Math::PxPyPzEVector(pfo->getMomentum()[0], pfo->getMomentum()[1], pfo->getMomentum()[2], pfo->getEnergy());
}

TruthRecoComparison aTruthRecoComparison ;

TruthRecoComparison::TruthRecoComparison() :

  Processor("TruthRecoComparison"),
  m_n_run(0),
  m_n_evt(0),
  m_error_code(ERROR_CODES::UNINITIALIZED),
  m_pTFile(NULL)
{

	_description = "TruthRecoComparison writes relevant observables to root-file " ;

	registerInputCollection(LCIO::MCPARTICLE,
				"MCParticleCollection" ,
				"Name of the MCParticle collection"  ,
				m_mcParticleCollection,
				std::string("MCParticle")
				);

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				"inputPfoCollection",
				"Name of input pfo collection",
				m_inputPfoCollection,
				std::string("PandoraPFOs")
				);

	registerInputCollection(LCIO::RECONSTRUCTEDPARTICLE,
				"inputJetCollection",
				"Name of input pfo collection",
				m_inputJetCollection,
				std::string("RefinedJets")
				);

	registerInputCollection(LCIO::LCRELATION,
				 "RecoMCTruthLink",
				 "Name of the RecoMCTruthLink input collection"  ,
				 m_recoMCTruthLink,
				 std::string("RecoMCTruthLink")
				 );

	registerInputCollection(LCIO::LCRELATION,
				"MCTruthRecoLink",
				"Name of the MCTruthRecoLink input collection"  ,
				m_mcTruthRecoLink,
				std::string("MCTruthRecoLink")
				);

  	registerProcessorParameter("outputRootFilename",
				"name of output root file",
				m_outputRootFile,
				std::string("")
				);

	registerProcessorParameter("ECM" ,
				"Center-of-Mass Energy in GeV",
				m_ECM,
				float(500.f)
				);
}



void TruthRecoComparison::init()
{
	streamlog_out(DEBUG) << "   init called  " << std::endl;

	// Construct data structure to store data of particle species
	// kaons, pions, protons, electrons, muons
	for (int pdg: m_species_abs_pdgs) {
		streamlog_out(DEBUG) << "Preparing output for " << pdg << std::endl;

		std::vector<std::vector<float>> species_data;
		for (size_t j = 0; j < 2*m_species_features.size(); j++) {
			std::vector<float> feature_data;
			species_data.push_back(feature_data);
		}
		m_species[pdg] = species_data;

		std::vector<EVENT::MCParticle*> truth_particles_vec;
		m_species_particles_true[pdg] = truth_particles_vec;

		std::vector<EVENT::ReconstructedParticle*> reco_particles_vec;
		m_species_particles_reco[pdg] = reco_particles_vec;
	}
	
	this->clear();

	m_pTTree = new TTree("TruthRecoComparison", "TruthRecoComparison");

	if (m_outputRootFile.size()) {
		m_pTFile = new TFile(m_outputRootFile.c_str(),"recreate");
		m_pTTree->SetDirectory(m_pTFile);
	}

	m_pTTree->Branch("run", &m_n_run, "run/I");
	m_pTTree->Branch("event", &m_n_evt, "event/I");
	m_pTTree->Branch("error_code", &m_error_code);

	m_pTTree->Branch("true_Et", &m_true_Et);
	m_pTTree->Branch("reco_Et", &m_reco_Et);

	m_pTTree->Branch("true_pt_miss", &m_true_pt_miss);
	m_pTTree->Branch("reco_pt_miss", &m_reco_pt_miss);

	m_pTTree->Branch("true_m_miss", &m_true_m_miss);
	m_pTTree->Branch("reco_m_miss", &m_reco_m_miss);

	m_pTTree->Branch("true_E_miss", &m_true_E_miss);
	m_pTTree->Branch("reco_E_miss", &m_reco_E_miss);

	m_pTTree->Branch("true_E_vis", &m_true_E_vis);
	m_pTTree->Branch("reco_E_vis", &m_reco_E_vis);

	m_pTTree->Branch("mcp_mom_tot", &m_mcp_mom_tot);
	m_pTTree->Branch("mcp_mom_detected", &m_mcp_mom_detected);
	m_pTTree->Branch("mcp_mom_undetected", &m_mcp_mom_undetected);
	m_pTTree->Branch("pfo_mom_tot", &m_pfo_mom_tot);

	m_pTTree->Branch("reco_jet_pt", &m_reco_jet_pt);
	m_pTTree->Branch("reco_jet_E", &m_reco_jet_E);

	for (size_t i = 0; i < m_species_abs_pdgs.size(); i++) {
		streamlog_out(DEBUG) << "Attaching branches for species " << m_species_names[i];

		for (size_t j = 0; j < m_species_features.size(); j++) {
			streamlog_out(DEBUG) << "Attaching branch for feature " << m_species_features[j];

			m_pTTree->Branch((m_species_names[i] + "_" + m_species_features[j] + "_true").c_str(), &m_species[m_species_abs_pdgs[i]][j*2]);
			m_pTTree->Branch((m_species_names[i] + "_" + m_species_features[j] + "_reco").c_str(), &m_species[m_species_abs_pdgs[i]][j*2+1]);
		}
	}

	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void TruthRecoComparison::clear() 
{
	streamlog_out(DEBUG) << "   clear called  " << std::endl;

	m_error_code = ERROR_CODES::UNKNOWN_ERROR;

	m_mcp_mom_tot.SetXYZT(0., 0., 0., 0.);
	m_mcp_mom_detected.SetXYZT(0., 0., 0., 0.);
	m_mcp_mom_undetected.SetXYZT(0., 0., 0., 0.);
	m_pfo_mom_tot.SetXYZT(0., 0., 0., 0.);

	updateKinematics();

	// Reset data of particle species
	for (int pdg : m_species_abs_pdgs) {
		for (size_t j = 0; j < m_species[pdg].size(); j++) {
			m_species[pdg][j].clear();
		}

		m_species_particles_true[pdg].clear();
		m_species_particles_reco[pdg].clear();
	}

	m_reco_jet_pt.clear();
	m_reco_jet_E.clear();
}

void TruthRecoComparison::updateKinematics()
{
	// Transverse energy

	m_true_Et = m_mcp_mom_detected.Et();
	m_reco_Et = m_pfo_mom_tot.Et();

	// Missing pT, mass and energy

	float target_p_due_crossing_angle = m_ECM * 0.007; // crossing angle = 14 mrad
	double E_lab = 2 * sqrt( std::pow( 0.548579909e-3 , 2 ) + std::pow( m_ECM / 2 , 2 ) + std::pow( target_p_due_crossing_angle , 2 ) + 0. + 0.);

	ROOT::Math::PxPyPzEVector ecms(target_p_due_crossing_angle, 0., 0., E_lab) ;

	ROOT::Math::PxPyPzEVector p_miss_true = ecms - m_mcp_mom_detected;
	ROOT::Math::PxPyPzEVector p_miss_reco = ecms - m_pfo_mom_tot;

	m_true_pt_miss = p_miss_true.Pt();
	m_reco_pt_miss = p_miss_reco.Pt();

	m_true_m_miss = p_miss_true.M();
	m_reco_m_miss = p_miss_reco.M();

	m_true_E_miss = p_miss_true.E();
	m_reco_E_miss = p_miss_reco.E();

	m_true_E_vis = m_mcp_mom_detected.E();
	m_reco_E_vis = m_pfo_mom_tot.E();
}

void TruthRecoComparison::processRunHeader( LCRunHeader*  /*run*/) { 
	m_n_run++ ;
}

void TruthRecoComparison::processEvent( EVENT::LCEvent *pLCEvent )
{
	this->clear();
	
	streamlog_out(DEBUG)  << "processing event: " << pLCEvent->getEventNumber() << "  in run: " << pLCEvent->getRunNumber() << std::endl;
	
	m_n_run = pLCEvent->getRunNumber();
	m_n_evt = pLCEvent->getEventNumber();

	try {
		LCCollection *inputPFOCollection;
		LCCollection *inputJetCollection;
		LCCollection *inputMCParticleCollection;

		streamlog_out(DEBUG) << "        getting PFO collection: " << m_inputPfoCollection << std::endl ;
		inputPFOCollection = pLCEvent->getCollection(m_inputPfoCollection);

		streamlog_out(DEBUG) << "        getting jet collection: " << m_inputJetCollection << std::endl ;
		inputJetCollection = pLCEvent->getCollection(m_inputJetCollection);

		streamlog_out(DEBUG) << "        getting MCParticle collection: " << m_mcParticleCollection << std::endl ;
		inputMCParticleCollection = pLCEvent->getCollection(m_mcParticleCollection);

		streamlog_out(DEBUG) << "        getting RecoMCTruthLink relation: " << m_recoMCTruthLink << std::endl ;
		LCRelationNavigator RecoMCParticleNav = pLCEvent->getCollection(m_recoMCTruthLink);

		streamlog_out(DEBUG) << "        getting TruthRecoLink relation: " << m_mcTruthRecoLink << std::endl ;
      	LCRelationNavigator MCParticleRecoNav = pLCEvent->getCollection(m_mcTruthRecoLink);

		//E,theta,phi,pt

		for (int i = 0; i < inputMCParticleCollection->getNumberOfElements(); i++) {
			MCParticle* mcp = (MCParticle*) inputMCParticleCollection->getElementAt(i);

			if (mcp->getDaughters().size() == 0) {
				ROOT::Math::PxPyPzEVector mcp_mom = vec4(mcp);
				m_mcp_mom_tot += mcp_mom;

				float weightPFOtoMCP = 0.0;
	  			float weightMCPtoPFO = 0.0;

				ReconstructedParticle* linkedPFO = getLinkedPFO( mcp , RecoMCParticleNav , MCParticleRecoNav , false , false , weightPFOtoMCP , weightMCPtoPFO );
				if (linkedPFO == NULL) {
					m_mcp_mom_undetected += mcp_mom;
				} else {
					m_mcp_mom_detected += mcp_mom;
				}

				// Check if we're interested in this particle species
				if (std::find(m_species_abs_pdgs.begin(), m_species_abs_pdgs.end(), abs(mcp->getPDG())) != m_species_abs_pdgs.end()) {
					m_species_particles_true[abs(mcp->getPDG())].push_back(mcp);
				}
			}
		}

		for (int i = 0; i < inputPFOCollection->getNumberOfElements(); i++) {
			ReconstructedParticle* pfo = (ReconstructedParticle*) inputPFOCollection->getElementAt(i);
			m_pfo_mom_tot += vec4(pfo);

			// Check if we're interested in this particle species
			if (std::find(m_species_abs_pdgs.begin(), m_species_abs_pdgs.end(), abs(pfo->getType())) != m_species_abs_pdgs.end()) {
				m_species_particles_reco[abs(pfo->getType())].push_back(pfo);
			}
		}

		for (int i = 0; i < inputJetCollection->getNumberOfElements(); i++) {
			ReconstructedParticle* jet = (ReconstructedParticle*) inputJetCollection->getElementAt(i);
			ROOT::Math::PxPyPzEVector jet_mom = vec4(jet);

			m_reco_jet_pt.push_back(jet_mom.Pt());
			m_reco_jet_E.push_back(jet_mom.E());
		}

		for (int pdg : m_species_abs_pdgs) {
			streamlog_out(DEBUG) << "Extracting " << m_species[pdg].size() << " features for species " << pdg << std::endl;
			streamlog_out(DEBUG) << "    True particles: " << m_species_particles_true[pdg].size() << std::endl;

			extract_true_particle_features(m_species_particles_true[pdg], m_species[pdg]);
			extract_reco_particle_features(m_species_particles_reco[pdg], m_species[pdg]);
		}

		updateKinematics();

		m_error_code = ERROR_CODES::OK;

	} catch(DataNotAvailableException &e) {
		m_error_code = ERROR_CODES::COLLECTION_NOT_FOUND;
		streamlog_out(MESSAGE) << "processEvent : Input collections not found in event " << m_n_evt << std::endl;
	}

	m_pTTree->Fill();
}

void TruthRecoComparison::check()
{
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}


void TruthRecoComparison::end()
{
	// Write ROOT file
	if (m_pTFile != NULL) {
		m_pTFile->cd();
	}

	m_pTTree->Write();
	
	if (m_pTFile != NULL) {
		m_pTFile->Close();
		delete m_pTFile;
	}
}

EVENT::ReconstructedParticle* TruthRecoComparison::getLinkedPFO( EVENT::MCParticle *mcParticle , LCRelationNavigator RecoMCParticleNav , LCRelationNavigator MCParticleRecoNav , bool getChargedPFO , bool getNeutralPFO , float &weightPFOtoMCP , float &weightMCPtoPFO )
{
	streamlog_out(DEBUG1) << "" << std::endl;
	streamlog_out(DEBUG1) << "Look for PFO linked to visible MCParticle:" << std::endl;

	ReconstructedParticle *linkedPFO{};
	bool foundlinkedPFO = false;
	const EVENT::LCObjectVec &PFOvec = MCParticleRecoNav.getRelatedToObjects(mcParticle);
	const EVENT::FloatVec &PFOweightvec = MCParticleRecoNav.getRelatedToWeights(mcParticle);
	streamlog_out(DEBUG0) << "Visible MCParticle is linked to " << PFOvec.size() << " PFO(s)" << std::endl;
	weightPFOtoMCP = 0.0;
	weightMCPtoPFO = 0.0;
	double maxweightPFOtoMCP = 0.;
	double maxweightMCPtoPFO = 0.;
	int iPFOtoMCPmax = -1;
	int iMCPtoPFOmax = -1;

	for (unsigned int i_pfo = 0; i_pfo < PFOvec.size(); i_pfo++) {
		double pfo_weight = 0.0;
		double trackWeight = (int(PFOweightvec.at(i_pfo)) % 10000) / 1000.0;
		double clusterWeight = (int(PFOweightvec.at(i_pfo)) / 10000) / 1000.0;
		if (getChargedPFO && !getNeutralPFO) {
			pfo_weight = trackWeight;
		} else if (getNeutralPFO && !getChargedPFO) {
			pfo_weight = clusterWeight;
		} else {
			pfo_weight = (trackWeight > clusterWeight ? trackWeight : clusterWeight);
		}

		streamlog_out(DEBUG0) << "Visible MCParticle linkWeight to PFO: " << PFOweightvec.at(i_pfo) << " (Track: " << trackWeight << " , Cluster: " << clusterWeight << ")" << std::endl;
		ReconstructedParticle *testPFO = (ReconstructedParticle *)PFOvec.at(i_pfo);
		if (pfo_weight > maxweightMCPtoPFO) { //&& track_weight >= m_MinWeightTrackMCTruthLink )
			maxweightMCPtoPFO = pfo_weight;
			iMCPtoPFOmax = i_pfo;
			streamlog_out(DEBUG0) << "PFO at index: " << testPFO->id() << " has TYPE: " << testPFO->getType() << " and MCParticle to PFO link weight is " << pfo_weight << std::endl;
		}
	}
	if (getChargedPFO && maxweightMCPtoPFO < 0.8) {
		streamlog_out(DEBUG1) << "MCParticle has link weight lower than 0.8 ( " << maxweightMCPtoPFO << " ), looking for linked PFO in clusters" << std::endl;
		for (unsigned int i_pfo = 0; i_pfo < PFOvec.size(); i_pfo++) {
			double pfo_weight = (int(PFOweightvec.at(i_pfo)) / 10000) / 1000.0;
			streamlog_out(DEBUG0) << "Visible MCParticle linkWeight to PFO: " << PFOweightvec.at(i_pfo) << " (Track: " << (int(PFOweightvec.at(i_pfo)) % 10000) / 1000.0 << " , Cluster: " << (int(PFOweightvec.at(i_pfo)) / 10000) / 1000.0 << ")" << std::endl;
			ReconstructedParticle *testPFO = (ReconstructedParticle *)PFOvec.at(i_pfo);
			if (pfo_weight > maxweightMCPtoPFO) { //&& track_weight >= m_MinWeightTrackMCTruthLink )
				maxweightMCPtoPFO = pfo_weight;
				iMCPtoPFOmax = i_pfo;
				streamlog_out(DEBUG0) << "PFO at index: " << testPFO->id() << " has TYPE: " << testPFO->getType() << " and MCParticle to PFO link weight is " << pfo_weight << std::endl;
			}
		}
	}
	if (iMCPtoPFOmax != -1) {
		ReconstructedParticle *testPFO = (ReconstructedParticle *)PFOvec.at(iMCPtoPFOmax);
		const EVENT::LCObjectVec &MCPvec = RecoMCParticleNav.getRelatedToObjects(testPFO);
		const EVENT::FloatVec &MCPweightvec = RecoMCParticleNav.getRelatedToWeights(testPFO);
		for (unsigned int i_mcp = 0; i_mcp < MCPvec.size(); i_mcp++) {
			double mcp_weight = 0.0;
			double trackWeight = (int(MCPweightvec.at(i_mcp)) % 10000) / 1000.0;
			double clusterWeight = (int(MCPweightvec.at(i_mcp)) / 10000) / 1000.0;

			if (getChargedPFO && !getNeutralPFO) {
				mcp_weight = trackWeight;
			} else if (getNeutralPFO && !getChargedPFO) {
				mcp_weight = clusterWeight;
			} else {
				mcp_weight = (trackWeight > clusterWeight ? trackWeight : clusterWeight);
			}

			MCParticle *testMCP = (MCParticle *)MCPvec.at(i_mcp);
			if (mcp_weight > maxweightPFOtoMCP) {//&& mcp_weight >= m_MinWeightTrackMCTruthLink )
				maxweightPFOtoMCP = mcp_weight;
				iPFOtoMCPmax = i_mcp;
				streamlog_out(DEBUG0) << "MCParticle at index: " << testMCP->id() << " has PDG: " << testMCP->getPDG() << " and PFO to MCParticle link weight is " << mcp_weight << std::endl;
			}
		}
		if (iPFOtoMCPmax != -1) {
			if (MCPvec.at(iPFOtoMCPmax) == mcParticle) {
				linkedPFO = testPFO;
				foundlinkedPFO = true;
			}
		}
	}

	if (foundlinkedPFO) {
		streamlog_out(DEBUG1) << "Linked PFO to MCParticle found successfully " << std::endl;
		weightPFOtoMCP = maxweightPFOtoMCP;
		weightMCPtoPFO = maxweightMCPtoPFO;
		return linkedPFO;
	} else {
		streamlog_out(DEBUG1) << "Couldn't Find a PFO linked to MCParticle" << std::endl;
		return NULL;
	}
}

void TruthRecoComparison::extract_true_particle_features(std::vector<EVENT::MCParticle*> mcparticles, std::vector<std::vector<float>> &feature_vec){
	for (size_t i = 0; i < mcparticles.size(); i++) {
		ROOT::Math::PxPyPzEVector mom4vec = vec4(mcparticles[i]);

		feature_vec[0].push_back(mom4vec.E());
		feature_vec[2].push_back(mom4vec.Theta());
		feature_vec[4].push_back(mom4vec.Phi());
		feature_vec[6].push_back(mom4vec.Pt());
	}
}

void TruthRecoComparison::extract_reco_particle_features(std::vector<EVENT::ReconstructedParticle*> reco_particles, std::vector<std::vector<float>> &feature_vec) {
	for (size_t i = 0; i < reco_particles.size(); i++) {
		ROOT::Math::PxPyPzEVector mom4vec = vec4(reco_particles[i]);

		feature_vec[1].push_back(mom4vec.E());
		feature_vec[3].push_back(mom4vec.Theta());
		feature_vec[5].push_back(mom4vec.Phi());
		feature_vec[7].push_back(mom4vec.Pt());
	}
}