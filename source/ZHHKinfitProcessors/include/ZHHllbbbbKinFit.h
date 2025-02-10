#ifndef ZHHllbbbbKinFit_h
#define ZHHllbbbbKinFit_h 1

#include "ZHHBaseKinfitProcessor.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHllbbbbKinFit : public Processor , public ZHHBaseKinfitProcessor
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

		virtual void	init();
		virtual void	Clear();
		virtual void	processRunHeader();
		virtual void	processEvent( EVENT::LCEvent *pLCEvent );
		
		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
		std::vector<double> calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::tuple<std::vector<double>, double, std::vector<unsigned int>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);

		virtual void	check( LCEvent * evt );

	    TTree *m_pTTree = new TTree("ZHHllbbbbKinFit","ZHHllbbbbKinFit");
};

#endif
