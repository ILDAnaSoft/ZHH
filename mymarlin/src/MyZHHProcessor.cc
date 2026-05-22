#include "MyZHHProcessor.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ReconstructedParticle.h>
#include <AIDA/IHistogramFactory.h>

MyZHHProcessor aMyZHHProcessor ;

MyZHHProcessor::MyZHHProcessor() : Processor("MyZHHProcessor") {
    
    // Register the input collections we want to process
    registerInputCollection( lcio::LCIO::MCPARTICLE,
                            "MCParticleCollection" , 
                            "Name of the MCParticle collection"  ,
                            _mcParticleCollectionName ,
                            std::string("MCParticle") ) ;

    registerInputCollection( lcio::LCIO::RECONSTRUCTEDPARTICLE,
                            "RecoParticleCollection" , 
                            "Name of the ReconstructedParticle collection"  ,
                            _recoParticleCollectionName ,
                            std::string("PandoraPFOs") ) ; // PandoraPFOs are the reconstructed tracks/clusters
}

void MyZHHProcessor::init() {
    printParameters() ;
    
    // Create the histograms (Name, Title, Bins, Min, Max)
    _h_mc_energy   = marlin::AIDAProcessor::histogramFactory(this)->createHistogram1D("MC_Energy", "MC Truth Energy (GeV)", 100, 0.0, 15.0);
    _h_reco_energy = marlin::AIDAProcessor::histogramFactory(this)->createHistogram1D("Reco_Energy", "Reconstructed Energy (GeV)", 100, 0.0, 15.0);
    _h_reco_pt     = marlin::AIDAProcessor::histogramFactory(this)->createHistogram1D("Reco_Pt", "Reconstructed Transverse Momentum (GeV)", 100, 0.0, 15.0);
}

void MyZHHProcessor::processRunHeader( lcio::LCRunHeader* run) { }

void MyZHHProcessor::processEvent( lcio::LCEvent* evt ) { 

    // --- 1. Process MC Particles ---
    try {
        lcio::LCCollection* mcCol = evt->getCollection( _mcParticleCollectionName ) ;
        for(int i=0; i < mcCol->getNumberOfElements(); i++){
            lcio::MCParticle* p = dynamic_cast<lcio::MCParticle*>( mcCol->getElementAt(i) );
            _h_mc_energy->fill( p->getEnergy() );
        }
    } catch(lcio::DataNotAvailableException &e) { }

    // --- 2. Process Reconstructed Particles (PFOs) ---
    try {
        lcio::LCCollection* recoCol = evt->getCollection( _recoParticleCollectionName ) ;
        for(int i=0; i < recoCol->getNumberOfElements(); i++){
            lcio::ReconstructedParticle* p = dynamic_cast<lcio::ReconstructedParticle*>( recoCol->getElementAt(i) );
            
            _h_reco_energy->fill( p->getEnergy() );
            
            // Calculate Transverse Momentum (Pt) = sqrt(Px^2 + Py^2)
            const double* mom = p->getMomentum();
            double pt = std::sqrt( mom[0]*mom[0] + mom[1]*mom[1] );
            _h_reco_pt->fill( pt );
        }
    } catch(lcio::DataNotAvailableException &e) { }
}

void MyZHHProcessor::check( lcio::LCEvent* evt ) { }

void MyZHHProcessor::end(){ }
