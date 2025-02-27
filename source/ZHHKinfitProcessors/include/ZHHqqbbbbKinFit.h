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

		void initChannelValues();
		void clearChannelValues();
		void updateChannelValues( EVENT::LCEvent *pLCEvent );
		unsigned short channel() { return CHANNEL_QQ; };

		std::tuple<std::vector<double>, double, std::vector<unsigned short>> calculateMassesFromSimpleChi2Pairing(pfoVector jets, pfoVector leptons);
};

#endif
