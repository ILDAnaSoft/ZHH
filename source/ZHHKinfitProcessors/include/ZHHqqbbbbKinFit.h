#ifndef ZHHqqbbbbKinFit_h
#define ZHHqqbbbbKinFit_h 1

#include "ZHHBaseKinfitProcessor.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHqqbbbbKinFit : public Processor , public ZHHBaseKinfitProcessor
{

	public:

		virtual Processor*  newProcessor()
		{
			return new ZHHqqbbbbKinFit;
		}
		ZHHqqbbbbKinFit() ;
		virtual ~ZHHqqbbbbKinFit() = default;
		ZHHqqbbbbKinFit(const ZHHqqbbbbKinFit&) = delete;
		ZHHqqbbbbKinFit& operator=(const ZHHqqbbbbKinFit&) = delete;

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
						   

		pfoVectorVector combinations(pfoVectorVector collector,
					     pfoVectorVector sets, 
					     int n,
					     pfoVector combo);

		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
		std::vector<double> calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::vector<double> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);

		virtual void	check( LCEvent * evt );

	    TTree *m_pTTree = new TTree("ZHHqqbbbbKinFit","ZHHqqbbbbKinFit");;
};

#endif
