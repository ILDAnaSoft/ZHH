#ifndef ZHHKinFit_h
#define ZHHKinFit_h 1

#include <iostream>
#include <vector>
#include <string>

#include "marlin/Processor.h"
#include "lcio.h"
#include <UTIL/LCRelationNavigator.h>
#include <EVENT/Vertex.h>
#include <EVENT/ReconstructedParticle.h>
#include "TLorentzVector.h"
#include "DDMarlinCED.h"
#include "v4.h"

#include <GeometryUtil.h>
#include <CLHEP/Vector/LorentzVector.h>
#include "JetFitObject.h"
#include "LeptonFitObject.h"
#include "ISRPhotonFitObject.h"
#include "ZinvisibleFitObject.h"
#include "ZinvisibleFitObjectNew.h"
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

class ZHHKinFit : public Processor
{
  
public:
  
  virtual Processor*  newProcessor()
  {
    return new ZHHKinFit;
  }
  ZHHKinFit() ;
  virtual ~ZHHKinFit() = default;
  ZHHKinFit(const ZHHKinFit&) = delete;
  ZHHKinFit& operator=(const ZHHKinFit&) = delete;
  typedef vector<EVENT::ReconstructedParticle*>	pfoVector;
  typedef vector<vector<EVENT::ReconstructedParticle*>>	pfoVectorVector;
  struct FitResult {
    FitResult():
      fitter(shared_ptr<BaseFitter>()),
      constraints(shared_ptr<vector<shared_ptr<BaseHardConstraint>>>()),
      fitobjects(shared_ptr<vector<shared_ptr<BaseFitObject>>>()) {};
    FitResult(shared_ptr<BaseFitter> _fitter, 
	      shared_ptr<vector<shared_ptr<BaseHardConstraint>>> _constraints, 
	      shared_ptr<vector<shared_ptr<BaseFitObject>>> _fitobjects) : 
      fitter(_fitter), constraints(_constraints), fitobjects(_fitobjects) {};
    shared_ptr<BaseFitter> fitter;
    shared_ptr<vector<shared_ptr<BaseHardConstraint>>> constraints;
    shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects;
  };
  struct JetAndCorrection {
    ReconstructedParticle* particle;
    pfoVector nu;
  };
  virtual void	init();
  virtual void	Clear();
  virtual void	processRunHeader();
  virtual void	processEvent( EVENT::LCEvent *pLCEvent );
  ReconstructedParticle* addNeutrinoCorrection(ReconstructedParticle* jet,
					       pfoVector neutrinos);
  

  template<typename TYPE>
  std::vector<std::vector<TYPE*>> combinations(std::vector<std::vector<TYPE*>> collector,
			       std::vector<std::vector<TYPE*>> sets, 
			       unsigned int n,
			       std::vector<TYPE*> combo);
  pfoVectorVector getNeutrinosInJet( LCRelationNavigator* JetSLDNav , 
				     LCRelationNavigator* SLDNuNav , 
				     EVENT::ReconstructedParticle* jet);
  std::pair<MCParticle*,ReconstructedParticle*> getMCNeutrino(LCRelationNavigator* NuMCNav,
							      LCRelationNavigator* SLDNuNav,
							      EVENT::ReconstructedParticle* neutrino);
  FitResult performllbbbbFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
  FitResult performvvbbbbFIT( pfoVector jets,bool traceEvent);	    
  FitResult performqqbbbbFIT( pfoVector jets,bool traceEvent);	    
  virtual void	getJetParameters( ReconstructedParticle* jet , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );
  virtual void	getLeptonParameters( ReconstructedParticle* lepton , float (&parameters)[ 3 ] , float (&errors)[ 3 ] );
  std::pair<std::vector<double>,std::vector<double>> calculateInitialValues(pfoVector jets, pfoVector leptons, vector<int> perm);
  std::vector<double> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
  std::vector<double> calculatePulls(std::shared_ptr<ParticleFitObject> fittedobject, ReconstructedParticle* startobject, int type);
  double calcChi2(shared_ptr<vector<shared_ptr<BaseFitObject>>> fitobjects);
  virtual void	check( LCEvent * evt );
  virtual void	end();
  string get_recoMCTruthLink()
  {
    return _recoMCTruthLink;
  };
  
private:
  
  std::string m_inputleptonCollection{};
  std::string m_inputJetCollection{};
  std::string m_inputSLDVertexCollection{};
  std::string m_inputJetSLDLink{};
  std::string m_inputSLDNuLink{};
  std::string m_recoNumcNuLinkName{};
  std::string _MCParticleColllectionName{};
  std::string _recoParticleCollectionName{};
  std::string _recoMCTruthLink{};
  std::string m_outputFile{};
  std::string m_treeName{};
  std::string m_fithypothesis{};
  std::string m_outputLeptonCollection{};
  std::string m_outputJetCollection{};
  std::string m_outputStartLeptonCollection{};
  std::string m_outputStartJetCollection{};
  std::string m_outputNuEnergyCollection{};
  std::string _OutLeptonPullsCol{};
  std::string _OutJetPullsCol{};
  std::string m_signature{};
  int m_nAskedJets{};
  int m_nAskedLeps{};
  bool m_fitISR = true;
  bool m_solveNu = false;
  int m_fitter{};
  bool m_traceall{};
  int m_ievttrace{};
  bool m_matchTrueJetWithAngle = false;
  
  int m_nJets{};
  int m_nLeps{};
  int m_nRun;
  int m_nEvt;
  int m_nRunSum;
  int m_nEvtSum;
  float m_Bfield;
  float c;
  float mm2m;
  float eV2GeV;
  float eB;
  float m_ECM{};
  float m_isrpzmax{};
  float m_SigmaInvPtScaleFactor{};
  float m_SigmaEnergyScaleFactor{};
  float m_SigmaAnglesScaleFactor{};
  float m_ZinvisiblePxPyError{};
  float m_ZinvisiblePzError{};
  double b{};
  double ISRPzMaxB{};
  TFile *m_pTFile{};
  TTree *m_pTTree{};
  int m_nSLDecayBHadron{};
  int m_nSLDecayCHadron{};
  int m_nSLDecayTauLepton{};
  int m_nSLDecayTotal{};
  int m_nCorrectedSLD{};
  float m_ISREnergyTrue{};
  float m_BSEnergyTrue{};
  float m_HHMassHardProcess{};
  int m_FitErrorCode{};
  int m_invalidCovMatrixAtJet{};
  float m_pxcstartvalue{};
  float m_pycstartvalue{};
  float m_pzcstartvalue{};
  float m_ecstartvalue{};
  float m_ZMassBeforeFit{};
  float m_H1MassBeforeFit{};
  float m_H2MassBeforeFit{};
  float m_HHMassBeforeFit{};
  float m_ZHHMassBeforeFit{};
  float m_ISREnergyBeforeFit{};
  float	m_p1stBeforeFit{};
	float m_cos1stBeforeFit{};
  float m_ZMassAfterFit{};
  float m_H1MassAfterFit{};
  float m_H2MassAfterFit{};
  float m_HHMassAfterFit{};
  float m_ZHHMassAfterFit{};
  float m_ISREnergyAfterFit{};
  float	m_p1stAfterFit{};
	float m_cos1stAfterFit{};
  float m_FitProbability{};
  float m_FitChi2{};
  std::vector<int> m_perm{};
  std::vector<ROOT::Math::PxPyPzEVector> m_PrefitJetFourMomentum{};
  std::vector<ROOT::Math::PxPyPzEVector> m_PostfitJetFourMomentum{};
  std::vector<float> m_pullJetEnergy{};
  std::vector<float> m_pullJetTheta{};
  std::vector<float> m_pullJetPhi{};
  std::vector<float> m_pullLeptonInvPt{};
  std::vector<float> m_pullLeptonTheta{};
  std::vector<float> m_pullLeptonPhi{};
  std::vector<float> m_TrueNeutrinoEnergy{};
  std::vector<float> m_RecoNeutrinoEnergy{};
  std::vector<float> m_RecoNeutrinoEnergyKinfit{};
  std::vector<float> m_Sigma_Px2{};
  std::vector<float> m_Sigma_PxPy{};
  std::vector<float> m_Sigma_Py2{};
  std::vector<float> m_Sigma_PxPz{};
  std::vector<float> m_Sigma_PyPz{};
  std::vector<float> m_Sigma_Pz2{};
  std::vector<float> m_Sigma_PxE{};
  std::vector<float> m_Sigma_PyE{};
  std::vector<float> m_Sigma_PzE{};
  std::vector<float> m_Sigma_E2{};
  
};

#endif
