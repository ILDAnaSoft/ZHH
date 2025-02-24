#ifndef ZHHqqbbbbKinFit_h
#define ZHHqqbbbbKinFit_h 1

#include "ZHHBaseKinfitProcessor.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHqqbbbbKinFit : public ZHHBaseKinfitProcessor
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

		virtual void initChannelValues();
		virtual void clearChannelValues();
		virtual void updateChannelValues( EVENT::LCEvent *pLCEvent );
		virtual unsigned short channel() { return CHANNEL_QQ; };

		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);	    
		std::vector<double> calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::tuple<std::vector<double>, double, std::vector<unsigned short>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
};

#endif
