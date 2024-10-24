#include "HdecayMode.h"
#include <iostream>
#include <vector>
#include <string>
#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <IMPL/MCParticleImpl.h>

using namespace lcio ;
using namespace marlin ;
using namespace std ;

HdecayMode aHdecayMode ;

HdecayMode::HdecayMode() :

	Processor("HdecayMode"),
	m_isDecayedTob(0),
	m_isDecayedToc(0),
	m_isDecayedToother(0),
	m_isBothDecayedToHeavy(0),
	m_ISR1Energy(0.f),
	m_ISR1Px(0.f),
	m_ISR1Py(0.f),
	m_ISR1Pz(0.f),
	m_ISR2Energy(0.f),
	m_ISR2Px(0.f),
	m_ISR2Py(0.f),
	m_ISR2Pz(0.f)
{

	_description = "HdecayMode identifies Higgs decay mode and finds ISR in generator level" ;

	registerInputCollection( 	LCIO::MCPARTICLE,
					"MCParticleCollection" ,
					"Name of the MCParticle collection"  ,
					m_mcParticleCollection,
					std::string("MCParticle")
					);

	registerOutputCollection( 	LCIO::MCPARTICLE,
					"HdecayModeCollection",
					"Collection of Higgs decay mode plus ISR",
					m_HdecayModeCollection,
					std::string("HdecayMode")
					);
	registerProcessorParameter(     "nHiggs" ,
					"Number of Higgs decays",
					_nhiggs,
					(int)2.);
	registerProcessorParameter(     "outputFilename",
					"name of output root file",
					m_outputFile,
					std::string("")
					);

	registerProcessorParameter(     "outputFilename",
					"name of output root file",
					m_outputFile,
					std::string("")
					);
}

void HdecayMode::init()
{

	streamlog_out(DEBUG) << "   init called  " << std::endl ;
	m_isDecayedTob = 0;
	m_isDecayedToc = 0;
	m_isDecayedToother = 0;
	m_isBothDecayedToHeavy = 0;
	m_ISR1Energy = 0.;
	m_ISR1Px = 0.;
	m_ISR1Py = 0.;
	m_ISR1Pz = 0.;
	m_ISR2Energy = 0.;
	m_ISR2Px = 0.;
	m_ISR2Py = 0.;
	m_ISR2Pz = 0.;
	this->Clear();

	m_nRun = 0;
	m_nEvt = 0;

	m_pTFile = new TFile(m_outputFile.c_str(),"recreate");
	m_pTTree = new TTree("eventTree","eventTree");
	m_pTTree->SetDirectory(m_pTFile);

	m_pTTree->Branch("run", &m_nRun, "run/I");
	m_pTTree->Branch("event", &m_nEvt, "event/I");
	m_pTTree->Branch("nHdecayTob",&m_nHdecayTob,"nHdecayTob/I") ;
	m_pTTree->Branch("nZdecayTob",&m_nZdecayTob,"nZdecayTob/I") ;

	m_nHdecayTob = 0;
	m_nZdecayTob = 0;	

	streamlog_out(DEBUG) << "   init finished  " << std::endl;
}

void HdecayMode::Clear()
{
	m_isDecayedTob = 0;
	m_isDecayedToc = 0;
	m_isDecayedToother = 0;
	m_isBothDecayedToHeavy = 0;
	m_ISR1Energy = 0.;
	m_ISR1Px = 0.;
	m_ISR1Py = 0.;
	m_ISR1Pz = 0.;
	m_ISR2Energy = 0.;
	m_ISR2Px = 0.;
	m_ISR2Py = 0.;
	m_ISR2Pz = 0.;
	m_nHdecayTob = 0;
	m_nZdecayTob = 0;
}

void HdecayMode::processRunHeader()
{

}

void HdecayMode::processEvent( LCEvent *pLCEvent ) {
	try {
		this->Clear();
		m_nRun = pLCEvent->getRunNumber();
    	m_nEvt = pLCEvent->getEventNumber();

		const EVENT::LCCollection *mcpCollection = pLCEvent->getCollection(m_mcParticleCollection);
		streamlog_out(DEBUG) << "Processing event " << pLCEvent->getEventNumber() << std::endl;
		m_col_HDecayMode = new LCCollectionVec(LCIO::MCPARTICLE);

		for (int i = 0; i < mcpCollection->getNumberOfElements(); ++i) {
			const EVENT::MCParticle *pMCParticle = dynamic_cast<EVENT::MCParticle*>(mcpCollection->getElementAt(i));
			if ( pMCParticle->getPDG() == 25 && (pMCParticle->getDaughters()).size() == 2 ) {
				if ( abs((pMCParticle->getDaughters()[0])->getPDG()) == 5 && (pMCParticle->getDaughters()[0])->getPDG() / (pMCParticle->getDaughters()[1])->getPDG() == -1 ) {
					m_isDecayedTob ++;
					streamlog_out(DEBUG) << "Higgs decays to bbbar!" << std::endl;
				}
				else if ( abs((pMCParticle->getDaughters()[0])->getPDG()) == 4 && (pMCParticle->getDaughters()[0])->getPDG() / (pMCParticle->getDaughters()[1])->getPDG() == -1 ) {
					m_isDecayedToc ++;
					streamlog_out(DEBUG) << "Higgs decays to ccbar!" << std::endl;
				} else {
					m_isDecayedToother ++;
					streamlog_out(DEBUG) << "Higgs decays to neither bbbar nor ccbar!: ( " << (pMCParticle->getDaughters()[0])->getPDG() << " , " << (pMCParticle->getDaughters()[1])->getPDG() << " )" << std::endl;
				}
			}
			if ( pMCParticle->getPDG() == 22 && (pMCParticle->getParents()).size() == 1 && abs((pMCParticle->getParents()[0])->getPDG()) == 11 ) {
				if ( i == 6 ) {
					m_ISR1Energy = pMCParticle->getEnergy();
					m_ISR1Px = pMCParticle->getMomentum()[0];
					m_ISR1Py = pMCParticle->getMomentum()[1];
					m_ISR1Pz = pMCParticle->getMomentum()[2];
				} else if ( i == 7 ) {
				m_ISR2Energy = pMCParticle->getEnergy();
				m_ISR2Px = pMCParticle->getMomentum()[0];
				m_ISR2Py = pMCParticle->getMomentum()[1];
				m_ISR2Pz = pMCParticle->getMomentum()[2];
				}
			}
		}

		m_nHdecayTob = m_isDecayedTob;
		if (abs(dynamic_cast<EVENT::MCParticle*>(mcpCollection->getElementAt(8))->getPDG()) == 5 
		&& (dynamic_cast<EVENT::MCParticle*>(mcpCollection->getElementAt(8))->getPDG()/dynamic_cast<EVENT::MCParticle*>(mcpCollection->getElementAt(9))->getPDG() == -1 )) {
			m_nZdecayTob = 1;
		}

		if (m_isDecayedTob + m_isDecayedToc == 2) m_isBothDecayedToHeavy ++;
		streamlog_out(DEBUG) << "ISR energy is " << m_ISR1Energy << " GeV" << std::endl;
		m_col_HDecayMode->parameters().setValue("isDecayedTob", (int)m_isDecayedTob);		
		m_col_HDecayMode->parameters().setValue("isDecayedToc", (int)m_isDecayedToc);		
		m_col_HDecayMode->parameters().setValue("isDecayedToother", (int)m_isDecayedToother);
		m_col_HDecayMode->parameters().setValue("isBothDecayedToHeavy", (int)m_isBothDecayedToHeavy);		
		m_col_HDecayMode->parameters().setValue("ISR1Energy", (float)m_ISR1Energy);
		m_col_HDecayMode->parameters().setValue("ISR1Px", (float)m_ISR1Px);
		m_col_HDecayMode->parameters().setValue("ISR1Py", (float)m_ISR1Py);
		m_col_HDecayMode->parameters().setValue("ISR1Pz", (float)m_ISR1Pz);
		m_col_HDecayMode->parameters().setValue("ISR2Energy", (float)m_ISR2Energy);
		m_col_HDecayMode->parameters().setValue("ISR2Px", (float)m_ISR2Px);
		m_col_HDecayMode->parameters().setValue("ISR2Py", (float)m_ISR2Py);
		m_col_HDecayMode->parameters().setValue("ISR2Pz", (float)m_ISR2Pz);

		int PDG;
		if (m_isDecayedTob == 2)
			PDG = 5555;
		else if (m_isDecayedTob == 1 && m_isDecayedToc == 1)
			PDG = 5544;
		else if (m_isDecayedToc == 2)
			PDG = 4444;
		else
			PDG = 9999;

		MCParticleImpl* HHdecay = new MCParticleImpl;
		HHdecay->setPDG(PDG);
		m_col_HDecayMode->addElement(HHdecay);
		pLCEvent->addCollection(m_col_HDecayMode, m_HdecayModeCollection);		
	}
	catch(...) {
	streamlog_out(WARNING) << "Could not extract MCParticle collection: " << m_mcParticleCollection << std::endl;
	}
	//if (m_isDecayedTob + m_isDecayedToc == _nhiggs) setReturnValue("GoodEvent", true);
	if (m_isDecayedTob == _nhiggs)
		setReturnValue("GoodEvent", true); 
	else
		setReturnValue("GoodEvent", false) ;

	if (m_isDecayedTob == 2)
		setReturnValue("bbbb", true);
	else
		setReturnValue("bbbb", false);

	if (m_isDecayedToc == 2)
		setReturnValue("cccc", true);
	else
		setReturnValue("cccc", false);

	if (m_isDecayedToother == 2)
		setReturnValue("qqqq", true);
	else
		setReturnValue("qqqq", false);

	m_pTTree->Fill();
}

void HdecayMode::check()
{
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}


void HdecayMode::end()
{
	m_pTFile->cd();
	m_pTTree->Write();
	m_pTFile->Close();
	delete m_pTFile;
}



