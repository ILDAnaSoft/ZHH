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
		
		void initChannelValues();
		void clearChannelValues();
		void updateChannelValues( EVENT::LCEvent *pLCEvent );
		unsigned short channel() { return CHANNEL_VV; };

		std::tuple<std::vector<double>, double, std::vector<unsigned short>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
};

#endif
