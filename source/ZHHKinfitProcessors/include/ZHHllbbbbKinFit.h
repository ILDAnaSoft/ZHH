#ifndef ZHHllbbbbKinFit_h
#define ZHHllbbbbKinFit_h 1

#include <iostream>
#include <vector>
#include <string>

#include "marlin/Processor.h"
#include "lcio.h"
#include "TrueJet_Parser.h"
#include <EVENT/Vertex.h>
#include <EVENT/ReconstructedParticle.h>
#include "TLorentzVector.h"
#include "DDMarlinCED.h"

#include <GeometryUtil.h>
#include <CLHEP/Vector/LorentzVector.h>
#include "JetFitObject.h"
#include "LeptonFitObject.h"
#include "ISRPhotonFitObject.h"
#include "MomentumConstraint.h"
#include "OPALFitterGSL.h"
#include "NewFitterGSL.h"
#include "TextTracer.h"
#include "NewtonFitterGSL.h"
#include "MassConstraint.h"
#include "SoftGaussParticleConstraint.h"
#include "SoftGaussMassConstraint.h"
#include "FourJetPairing.h"
#include "IMPL/ReconstructedParticleImpl.h"
#include "IMPL/LCCollectionVec.h"
#include <EVENT/LCCollection.h>
#include "TFile.h"
#include "TH1F.h"
#include "TH2I.h"
#include "TH2F.h"
#include "TTree.h"
#include <Math/Vector4D.h>
#include "BaseHardConstraint.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHllbbbbKinFit : public Processor , public TrueJet_Parser
{

	public:

		virtual Processor*  newProcessor()
		{
			return new ZHHllbbbbKinFit;
		}
		ZHHllbbbbKinFit() ;
		virtual ~ZHHllbbbbKinFit() = default;
		ZHHllbbbbKinFit(const ZHHllbbbbKinFit&) = delete;
		ZHHllbbbbKinFit& operator=(const ZHHllbbbbKinFit&) = delete;
		typedef vector<EVENT::ReconstructedParticle*>	pfoVector;
		typedef vector<vector<EVENT::ReconstructedParticle*>>	pfoVectorVector;
		struct FitResult {
		  FitResult() {
		    fitter = shared_ptr<BaseFitter>();
		    constraints = shared_ptr<vector<shared_ptr<BaseHardConstraint>>>();
		    fitobjects = shared_ptr<vector<shared_ptr<BaseFitObject>>>();
		  };
		  FitResult(shared_ptr<BaseFitter> _fitter, 
			    shared_ptr<vector<shared_ptr<BaseHardConstraint>>> _constraints, 
			    shared_ptr<vector<shared_ptr<BaseFitObject>>> _fitobjects) : 
		  fitter(_fitter), constraints(_constraints), fitobjects(_fitobjects) {};
		  shared_ptr<BaseFitter> fitter;
		  shared_ptr<vector<shared_ptr<BaseHardConstraint>>> constraints;
		  shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects;
		};
		virtual void	init();
		virtual void	Clear();
		virtual void	processRunHeader();
		virtual void	processEvent( EVENT::LCEvent *pLCEvent );
		ReconstructedParticle* addNeutrinoCorrection(ReconstructedParticle* jet,
							     pfoVector neutrinos);
						   

		pfoVectorVector combinations(pfoVectorVector collector,
					     pfoVectorVector sets, 
					     int n,
					     pfoVector combo);
		pfoVectorVector getNeutrinosInJet( LCRelationNavigator* JetSLDNav , 
						   LCRelationNavigator* SLDNuNav , 
						   EVENT::ReconstructedParticle* jet);
		std::pair<MCParticle*,ReconstructedParticle*> getMCNeutrino(LCRelationNavigator* NuMCNav,
									    LCRelationNavigator* SLDNuNav,
									    EVENT::ReconstructedParticle* neutrino);
		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
		virtual void	getJetParameters( ReconstructedParticle* jet , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );
		virtual void	getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );
		std::vector<double> calculateStartZH1H2(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::vector<double> calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type);
		double calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects);
		virtual void	check( LCEvent * evt );
		virtual void	end();
		string get_recoMCTruthLink()
		{
			return _recoMCTruthLink;
		};

	private:

		std::string				m_inputIsolatedleptonCollection{};
		std::string				m_inputJetCollection{};
		std::string				m_inputSLDVertexCollection{};
		std::string				m_inputJetSLDLink{};
		std::string				m_inputSLDNuLink{};
		std::string                             m_recoNumcNuLinkName{};
		std::string				_MCParticleColllectionName{};
		std::string				_recoParticleCollectionName{};
		std::string				_recoMCTruthLink{};
		std::string				m_outputFile{};
		std::string				m_fithypothesis{};
		std::string				m_outputLeptonCollection{};
		std::string				m_outputJetCollection{};
		std::string				m_outputStartLeptonCollection{};
		std::string				m_outputStartJetCollection{};
		std::string                             m_outputNuEnergyCollection{};
		std::string                             _OutLeptonPullsCol{};
		std::string                             _OutJetPullsCol{};
		int					m_nAskedJets{};
		int					m_nAskedIsoLeps{};
		bool					m_fitISR = true;
		int					m_fitter{};
		bool					m_traceall{};
		int					m_ievttrace{};
		bool					m_matchTrueJetWithAngle = false;

		int					m_nJets{};
		int					m_nIsoLeps{};
		int					m_nRun;
		int					m_nEvt;
		int					m_nRunSum;
		int					m_nEvtSum;
		float					m_Bfield;
		float					c;
		float					mm2m;
		float					eV2GeV;
		float					eB;
		float					m_ECM{};
		float					m_isrpzmax{};
		float					m_SigmaInvPtScaleFactor{};
		float					m_SigmaEnergyScaleFactor{};
		float					m_SigmaAnglesScaleFactor{};
		double					b{};
		double					ISRPzMaxB{};
		TFile					*m_pTFile{};
	        TTree					*m_pTTree{};
		int					m_nSLDecayBHadron{};
		int					m_nSLDecayCHadron{};
		int					m_nSLDecayTauLepton{};
		int					m_nSLDecayTotal{};
		int					m_nCorrectedSLD{};
		int					m_FitErrorCode_woNu{};
		float					m_ZMassBeforeFit_woNu{};
		float					m_H1MassBeforeFit_woNu{};
		float					m_H2MassBeforeFit_woNu{};
		float					m_ZMassAfterFit_woNu{};
		float					m_H1MassAfterFit_woNu{};
		float					m_H2MassAfterFit_woNu{};
		float					m_FitProbability_woNu{};
		float					m_FitChi2_woNu{};
		std::vector<float>			m_pullJetEnergy_woNu{};
		std::vector<float>			m_pullJetTheta_woNu{};
		std::vector<float>			m_pullJetPhi_woNu{};
		std::vector<float>			m_pullLeptonInvPt_woNu{};
		std::vector<float>			m_pullLeptonTheta_woNu{};
		std::vector<float>			m_pullLeptonPhi_woNu{};
		/*		int					m_FitErrorCode_wNu{};
		float					m_ZMassBeforeFit_wNu{};
		float					m_H1MassBeforeFit_wNu{};
		float					m_H2MassBeforeFit_wNu{};
		float					m_ZMassAfterFit_wNu{};
		float					m_H1MassAfterFit_wNu{};
		float					m_H2MassAfterFit_wNu{};
		float					m_FitProbability_wNu{};
		float					m_FitChi2_wNu{};
		std::vector<float>			m_pullJetEnergy_wNu{};
		std::vector<float>			m_pullJetTheta_wNu{};
		std::vector<float>			m_pullJetPhi_wNu{};
		std::vector<float>			m_pullLeptonInvPt_wNu{};
		std::vector<float>			m_pullLeptonTheta_wNu{};
		std::vector<float>			m_pullLeptonPhi_wNu{};*/
		int					m_FitErrorCode{};
		float					m_ZMassBeforeFit{};
		float					m_H1MassBeforeFit{};
		float					m_H2MassBeforeFit{};
		float					m_ZMassAfterFit{};
		float					m_H1MassAfterFit{};
		float					m_H2MassAfterFit{};
		float					m_FitProbability{};
		float					m_FitChi2{};
		std::vector<float>			m_pullJetEnergy{};
		std::vector<float>			m_pullJetTheta{};
		std::vector<float>			m_pullJetPhi{};
		std::vector<float>			m_pullLeptonInvPt{};
		std::vector<float>			m_pullLeptonTheta{};
		std::vector<float>			m_pullLeptonPhi{};
		std::vector<float>                      m_TrueNeutrinoEnergy{};
		std::vector<float>                      m_RecoNeutrinoEnergy{};
		std::vector<float>                      m_RecoNeutrinoEnergyKinfit{};
		std::vector<float>			m_Sigma_Px2{};
		std::vector<float>			m_Sigma_PxPy{};
		std::vector<float>			m_Sigma_Py2{};
		std::vector<float>			m_Sigma_PxPz{};
		std::vector<float>			m_Sigma_PyPz{};
		std::vector<float>			m_Sigma_Pz2{};
		std::vector<float>			m_Sigma_PxE{};
		std::vector<float>			m_Sigma_PyE{};
		std::vector<float>			m_Sigma_PzE{};
		std::vector<float>			m_Sigma_E2{};

		double					ZEnergy{};
		double					Zmomentum[3]{0.0};
		double					H1Energy{};
		double					H1momentum[3]{0.0};
		double					H2Energy{};
		double					H2momentum[3]{0.0};
		double					ISREnergy{};
		double					ISRmomentum[3]{0.0};
		//float					Hmass_NoFit{};
		//int					Error_code{};
		//int					errorcode{};
		/*float					hpull_jet_E{};
		float					hpull_jet2_E{};
		float					hpull_jet_th{};
		float					hpull_jet2_th{};
		float					hpull_jet_phi{};
		float					hpull_jet2_phi{};
		float					hpull_lepton_InvpT{};
		float					hpull_lepton2_InvpT{};
		float					hpull_lepton_th{};
		float					hpull_lepton2_th{};
		float					hpull_lepton_phi{};
		float					hpull_lepton2_phi{};*/
};

#endif
