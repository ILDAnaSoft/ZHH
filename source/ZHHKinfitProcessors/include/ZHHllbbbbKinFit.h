#ifndef ZHHllbbbbKinFit_h
#define ZHHllbbbbKinFit_h 1

#include "ZHHBaseKinfitProcessor.h"

using namespace lcio ;
using namespace marlin ;
using namespace std;

class ZHHllbbbbKinFit : public ZHHBaseKinfitProcessor
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

		void initChannelValues();
		void clearChannelValues();
		void updateChannelValues( EVENT::LCEvent *pLCEvent );
		unsigned short channel() { return CHANNEL_LL; };
		
		std::tuple<std::vector<double>, double, std::vector<unsigned short>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
};

#endif
