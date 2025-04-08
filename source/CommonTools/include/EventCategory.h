#ifndef ZHH_event_category_h
#define ZHH_event_category_h 1

struct EVENT_CATEGORY {
	enum Values: unsigned char {
		NONE = 0,

		// pre-selection: 1-19
		LEPTONIC = 1,
		NEUTRINO = 2,
		HADRONIC = 3,
		PRESEL_OTHER = 19,

		// final selection: 20-...
		FINAL_OTHER = 99
		//
	};
};

#endif