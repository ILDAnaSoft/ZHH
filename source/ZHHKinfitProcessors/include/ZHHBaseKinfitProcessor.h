#ifndef ZHHBaseKinfitProcessor_h
#define ZHHBaseKinfitProcessor_h 1

#include <iostream>
#include <vector>
#include <string>
#include <tuple>

#include "marlin/Processor.h"
#include "marlin/VerbosityLevels.h"
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
//#include "FourJetPairing.h"
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
#include "EventObservablesBase.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

typedef vector<EVENT::ReconstructedParticle*>	pfoVector;
typedef vector<vector<EVENT::ReconstructedParticle*>>	pfoVectorVector;

class ZHHBaseKinfitProcessor: public Processor
{
	public:
		ZHHBaseKinfitProcessor(const std::string& name) ;
		virtual ~ZHHBaseKinfitProcessor() = default;
		ZHHBaseKinfitProcessor(const ZHHBaseKinfitProcessor&) = delete;
		ZHHBaseKinfitProcessor& operator=(const ZHHBaseKinfitProcessor&) = delete;

		// must be defined in child
		virtual void initChannelValues() = 0;
		virtual void clearChannelValues() = 0;
		virtual void updateChannelValues(EVENT::LCEvent *pLCEvent) = 0;
		virtual unsigned short channel() = 0;

		static const unsigned short CHANNEL_LL = 1;
		static const unsigned short CHANNEL_VV = 2;
		static const unsigned short CHANNEL_QQ = 3;

		// these are defined by Base
		virtual void init();
		virtual void processEvent( LCEvent* evt );
		virtual void processRunHeader( LCRunHeader* run );
        virtual void check( LCEvent* evt );
        virtual void end();

		string get_recoMCTruthLink() {
			return _recoMCTruthLink;
		};

		TFile *m_pTFile{};
		TTree *m_pTTree{};

		struct FitResult {
		  FitResult():
		  	fitter(shared_ptr<BaseFitter>()),
		    constraints(map<string, shared_ptr<BaseHardConstraint>>()),
		    fitobjects(shared_ptr<vector<shared_ptr<BaseFitObject>>>()),
			permutation({}) {};

		  FitResult(shared_ptr<BaseFitter> _fitter, 
			    map<string, shared_ptr<BaseHardConstraint>> _constraints, 
			    shared_ptr<vector<shared_ptr<BaseFitObject>>> _fitobjects,
				vector<unsigned int> _permutation) : 
		  		fitter(_fitter), constraints(_constraints), fitobjects(_fitobjects), permutation(_permutation) {};

		  shared_ptr<BaseFitter> fitter;
		  map<string, shared_ptr<BaseHardConstraint>> constraints;
		  shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects;
		  vector<unsigned int> permutation;
		};

		// operation mode flag by fit hypothesis
		static const unsigned short MODE_NMC = 0;
		static const unsigned short MODE_ZHH = 1;
		static const unsigned short MODE_ZZH = 2;
		static const unsigned short MODE_ZZZ = 3;
		static const unsigned short MODE_ZZHsoft = 4;
		static const unsigned short MODE_MH = 5;
		static const unsigned short MODE_EQM = 6;

		// currently set operation mode
		unsigned short MODE{};
		bool MODE_IS_NMC{};
		bool MODE_IS_ZHH{};
		bool MODE_IS_ZZH{};
		bool MODE_IS_ZZZ{};
		bool MODE_IS_ZZHsoft{};
		bool MODE_IS_MH{};
		bool MODE_IS_EQM{};

		std::vector<std::string> m_readContraints{};

    protected:
		// control flow
		void clearBaseValues();
		void fillOutputCollections(EVENT::LCEvent *pLCEvent);
		void assignPostFitMasses(FitResult fitResult, bool woNu);

		static std::tuple<std::vector<double>, double, std::vector<unsigned int>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons, const unsigned short MODE);

		// helper functions
		static pfoVectorVector combinations(pfoVectorVector collector, pfoVectorVector sets, int n, pfoVector combo);
		static ReconstructedParticle* addNeutrinoCorrection(ReconstructedParticle* jet, pfoVector neutrinos);

		static std::pair<MCParticle*,ReconstructedParticle*> getMCNeutrino(LCRelationNavigator* NuMCNav,
											LCRelationNavigator* SLDNuNav,
											EVENT::ReconstructedParticle* neutrino);

		static pfoVectorVector getNeutrinosInJet( LCRelationNavigator* JetSLDNav, LCRelationNavigator* SLDNuNav,  EVENT::ReconstructedParticle* jet);

		virtual void	getJetParameters( ReconstructedParticle* jet , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );
		virtual void	getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );

		void attachBestPermutation(LCCollection *jets, vector<unsigned int> bestperm, string parameterSuffix, bool fromKinfit);

		void assignPermutations(size_t njets, string fithypothesis);
		vector<vector<unsigned int>> perms{};

		std::vector<double> calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type);
		double calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects);

		std::vector<ROOT::Math::PxPyPzEVector> m_v4_postfit_jets{};
		std::vector<ROOT::Math::PxPyPzEVector> m_v4_postfit_leptons{};

		std::string 		    m_ttreeName{};
        std::string				m_inputIsolatedleptonCollection{};
		std::string				m_inputJetCollection{};
		std::string				m_inputSLDVertexCollection{};
		std::string				m_inputJetSLDLink{};
		std::string				m_inputSLDNuLink{};
		std::string             m_recoNumcNuLinkName{};
		std::string				_MCParticleColllectionName{};
		std::string				_recoParticleCollectionName{};
		std::string				_recoMCTruthLink{};
		std::string				m_outputFile{};
		std::string				m_fithypothesis{};
		std::string				m_outputLeptonCollection{};
		std::string				m_outputJetCollection{};
		std::string				m_outputStartLeptonCollection{};
		std::string				m_outputStartJetCollection{};
		std::string             m_outputNuEnergyCollection{};
		std::string             _OutLeptonPullsCol{};
		std::string             _OutJetPullsCol{};

		int						m_nAskedJets{};
		int						m_nAskedIsoLeps{};
		bool					m_fitISR = true;
		int						m_fitter{};
		bool					m_traceall{};
		int						m_ievttrace{};
		bool					m_matchTrueJetWithAngle = false;

		int						m_nJets{};
		int						m_nIsoLeps{};
		int						m_nRun;
		int						m_nEvt;
		int						m_nRunSum;
		int						m_nEvtSum;
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

		int						m_nSLDecayBHadron{};
		int						m_nSLDecayCHadron{};
		int						m_nSLDecayTauLepton{};
		int						m_nSLDecayTotal{};
		int						m_nCorrectedSLD{};
        float                   m_ISREnergyTrue{};
        float                   m_BSEnergyTrue{};
        float                   m_HHMassHardProcess{};

        int						m_FitErrorCode_woNu{};
		float					m_ZMassBeforeFit_woNu{};
		float					m_Z2MassBeforeFit_woNu{};
		float					m_H1MassBeforeFit_woNu{};
        float					m_H2MassBeforeFit_woNu{};
        float                   m_HHMassBeforeFit_woNu{};
        float                   m_ZHHMassBeforeFit_woNu{};
        float                   m_ISREnergyBeforeFit_woNu{};
		float					m_p1stBeforeFit_woNu{};
		float					m_cos1stBeforeFit_woNu{};

		float					m_ZMassAfterFit_woNu{};
		float					m_Z2MassAfterFit_woNu{};
		float					m_H1MassAfterFit_woNu{};
		float					m_H2MassAfterFit_woNu{};
        float                   m_HHMassAfterFit_woNu{};
        float                   m_ZHHMassAfterFit_woNu{};
        float                   m_ISREnergyAfterFit_woNu{};
		float					m_p1stAfterFit_woNu{};
		float					m_cos1stAfterFit_woNu{};
		float					m_FitProbability_woNu{};
		float					m_FitChi2_woNu{};
		float 					m_FitChi2_byMass{};
		
		std::vector<unsigned int>   m_bestMatchingKinfit{};
		std::vector<unsigned int>   m_bestMatchingByMass{};
		std::vector<float>			m_pullJetEnergy_woNu{};
		std::vector<float>			m_pullJetTheta_woNu{};
		std::vector<float>			m_pullJetPhi_woNu{};
		std::vector<float>			m_pullLeptonInvPt_woNu{};
		std::vector<float>			m_pullLeptonTheta_woNu{};
		std::vector<float>			m_pullLeptonPhi_woNu{};

		int						m_FitErrorCode{};
		float					m_ZMassBeforeFit{};
		float					m_Z2MassBeforeFit{};
		float					m_H1MassBeforeFit{};
		float					m_H2MassBeforeFit{};
		float                   m_HHMassBeforeFit{};
		float                   m_ZHHMassBeforeFit{};
		float                   m_ISREnergyBeforeFit{};
		float					m_p1stBeforeFit{};
		float					m_cos1stBeforeFit{};

		float					m_ZMassAfterFit{};
		float 					m_Z2MassAfterFit{};
		float					m_H1MassAfterFit{};
		float					m_H2MassAfterFit{};
		float                   m_HHMassAfterFit{};
		float                   m_ZHHMassAfterFit{};
		float                   m_ISREnergyAfterFit{};
		float					m_p1stAfterFit{};
		float					m_cos1stAfterFit{};
		float					m_FitProbability{};
		float					m_FitChi2{};

		std::vector<float>			m_pullJetEnergy{};
		std::vector<float>			m_pullJetTheta{};
		std::vector<float>			m_pullJetPhi{};
		std::vector<float>			m_pullLeptonInvPt{};
		std::vector<float>			m_pullLeptonTheta{};
		std::vector<float>			m_pullLeptonPhi{};
		std::vector<float>          m_TrueNeutrinoEnergy{};
		std::vector<float>          m_RecoNeutrinoEnergy{};
		std::vector<float>          m_RecoNeutrinoEnergyKinfit{};
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
};

#endif