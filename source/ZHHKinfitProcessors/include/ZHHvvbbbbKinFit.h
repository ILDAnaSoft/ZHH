#ifndef ZHHvvbbbbKinFit_h
#define ZHHvvbbbbKinFit_h 1

#include "ZHHBaseKinfitProcessor.h"
#include "ZinvisibleFitObject.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHvvbbbbKinFit : public ZHHBaseKinfitProcessor
{

	public:
		virtual Processor*  newProcessor()
		{
			return new ZHHvvbbbbKinFit;
		}
		ZHHvvbbbbKinFit() ;
		virtual ~ZHHvvbbbbKinFit() = default;
		ZHHvvbbbbKinFit(const ZHHvvbbbbKinFit&) = delete;
		ZHHvvbbbbKinFit& operator=(const ZHHvvbbbbKinFit&) = delete;
		
		virtual void initChannelValues();
		virtual void clearChannelValues();
		virtual void updateChannelValues( EVENT::LCEvent *pLCEvent );
		virtual unsigned short channel() { return CHANNEL_QQ; };

		FitResult performFIT( pfoVector jets,pfoVector leptons,bool traceEvent);
		std::vector<double> calculateInitialMasses(pfoVector jets, pfoVector leptons, vector<unsigned int> perm);
		std::tuple<std::vector<double>, double, std::vector<unsigned int>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
};

#endif
