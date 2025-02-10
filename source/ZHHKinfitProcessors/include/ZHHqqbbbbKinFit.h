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

		virtual void	init();
		virtual void	Clear();
		virtual void	processRunHeader();
		virtual void	processEvent( EVENT::LCEvent *pLCEvent );

		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
		std::vector<double> calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::tuple<std::vector<double>, double, std::vector<unsigned int>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);

		virtual void	check( LCEvent * evt );

	    TTree *m_pTTree = new TTree("ZHHqqbbbbKinFit","ZHHqqbbbbKinFit");;
};

#endif
