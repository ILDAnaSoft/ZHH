#ifndef FinalStateRecorderCommon_h
#define FinalStateRecorderCommon_h 1

#include <string>
#include <vector>
#include <map>

// If the final state is a ZHH (with H -> bbar), the channel is given by the decay channel of the Z boson (else OTHER)
// NONE is for initialization only and should not occur in practice
struct EVENT_CATEGORY_ZHH {
	enum Values: int {
		OTHER = 0,
		LEPTONIC = 11,
		NEUTRINO = 21,
		HADRONIC = 31
	};
};

// Map processes to integers
struct PROCESS_ID {
	enum Values: int{
		// ffhh
		e1e1hh = 1111,
		e2e2hh = 1112,
		e3e3hh = 1113,

		n1n1hh = 1311,
		n23n23hh = 1312,
		qqhh = 1511,

		// ffffh
		e1e1qqh = 2161,
		e2e2qqh = 2162,
		e3e3qqh = 2163,
		n1n1qqh = 2341,
		n23n23qqh = 2342,
		qqqqh = 2520,

		// ff
		f2_z_l = 3170,
		f2_z_h = 3570,
		f2_z_nung = 3350,
		f2_z_bhabhag = 3171,
		f2_z_bhabhagg = 3172,

		// ffff
		f4_zz_l = 3181,
		f4_zz_h = 3581,
		f4_zz_sl = 3191,
		f4_sze_l = 3182,
		f4_sze_sl = 3192,
		f4_sznu_l = 3201,
		f4_sznu_sl = 3360,

		f4_ww_l = 3183,
		f4_ww_h = 3582,
		f4_ww_sl = 3193,
		f4_sw_l = 3184,
		f4_sw_sl = 3194,

		f4_zzorww_l = 3185,
		f4_zzorww_h = 3583,
		f4_szeorsw_l = 3202

		// fffff

		// ffffff
	};
};

int PROCESS_INVALID = -999;

// Event categorization
struct EVENT_CATEGORY_TRUE {
	enum Values: int {
		OTHER = 0,
		
		// LEPTONIC
		OTHER_LL = 10,
		llHH = 11, // llbbbb (ZHH signal)

		eebb = 12,
		μμbb = 13,
		𝜏𝜏bb = 14,
		llbbbb = 15,
		llqqH = 16,
		ll = 17,
		llll = 18,
		llqq = 19,
		llvv = 20,

		// NEUTRINO
		OTHER_VV = 30,
		vvHH = 31, // vvbbbb (ZHH signal)

		vvbb = 32,
		vvbbbb = 33,
		vvqqH = 34,
		vv = 35,
		vvqq = 36,

		// HADRONIC
		OTHER_QQ = 50,
		qqHH = 51, // qqbbbb (ZHH signal)

		qqqqH = 52,
		qqbbbb = 53,
		bbbb = 54,
		ttZ = 55,
		ttbb = 56,
		qq = 57,
		qqqq = 58,
		
		// ttbar -> lvbbqq [t->Wb, W->lv/qq, b->bb]
		// so far not accounted: ttbar -> llvvbb (two leptonically decaying W bosons)
		// reason: https://tikz.net/sm_decay_piechart/
		// W -> qqbar 67%; W -> lv 33%
		// => 2xW -> qqbar 67% * 67% = 44.89% (two hadronic decays)
		// => 2xW -> lv 33% * 33% = 10.89% (two leptonic decays)
		// rest: 44.22% (one hadronic, one leptonic decay)
		OTHER_TTBAR = 70,
		evbbqq = 71,
		μvbbqq = 72,
		𝜏vbbqq = 73,

		// tt/WWZ -> bbqqqq
		// for tt: tt -> bbqqqq : 2x [t->Wb; W->qq]
		// for WWZ: WWZ -> bbqqqq : 2x [W->qq; Z->bb]
		OTHER_FULL_HADRONIC = 80,
		bbcssc = 81,
		bbuddu = 82,
		bbcsdu = 83
	};
};

bool vec_contains(std::vector<int> vec, int num) {
	return (std::find(vec.begin(), vec.end(), num) != vec.end());
};

std::map<std::string, std::vector<int>> const ProcessMap {
	// 1st number: process ID
	// 2nd number: number of fermions in final state
	// 3rd number: number of Higgs bosons (their daughter PDGs are inferred)
	// 	following numbers: position in the MCParticle collection
	//  if Higgs bosons are present, their position is given at the end of the vector
	
	// Process ID
	// 1st digit: 1 for di-higgs, 2 for single Higgs, 3 for other
	// 2nd+3rd digit: see event categorization
	// 4th digit: flavor or other differentiation (e.g. s/t-channel etc.)

	// Processes including two Higgs bosons
    { "e1e1hh",    { PROCESS_ID::e1e1hh, EVENT_CATEGORY_TRUE::llHH, 2, 2, 8, 9, 10, 11 }}, // e- e+ h h
    { "e2e2hh",    { PROCESS_ID::e2e2hh, EVENT_CATEGORY_TRUE::llHH, 2, 8, 9, 10, 11 }}, // mu- mu+ h h
    { "e3e3hh",    { PROCESS_ID::e3e3hh, EVENT_CATEGORY_TRUE::llHH, 2, 8, 9, 10, 11 }}, // tau- tau+ h h

    { "n1n1hh",    { PROCESS_ID::n1n1hh  , EVENT_CATEGORY_TRUE::vvHH, 2, 2, 8, 9, 10, 11 }}, // nue anti-nue h h
    { "n23n23hh",  { PROCESS_ID::n23n23hh, EVENT_CATEGORY_TRUE::vvHH, 2, 2, 8, 9, 10, 11 }}, // nu(mu/tau) anti-nu(mu/tau) hh
    { "qqhh",      { PROCESS_ID::qqhh    , EVENT_CATEGORY_TRUE::qqHH, 2, 2, 8, 9, 10, 11 }},
    
	// Background events
	// Processes including one Higgs boson
    { "e1e1qqh",   { PROCESS_ID::e1e1qqh  , EVENT_CATEGORY_TRUE::llqqH, 4, 1, 8, 9, 10, 11, 12 }}, // e- e+ q q h
    { "e2e2qqh",   { PROCESS_ID::e2e2qqh  , EVENT_CATEGORY_TRUE::llqqH, 4, 1, 8, 9, 10, 11, 12 }}, // mu- mu+ q q h
    { "e3e3qqh",   { PROCESS_ID::e3e3qqh  , EVENT_CATEGORY_TRUE::llqqH, 4, 1, 8, 9, 10, 11, 12 }}, // tau- tau+ q q h
    { "n1n1qqh",   { PROCESS_ID::n1n1qqh  , EVENT_CATEGORY_TRUE::vvqqH, 4, 1, 8, 9, 10, 11, 12 }}, // nue anti-nue q q h
    { "n23n23qqh", { PROCESS_ID::n23n23qqh, EVENT_CATEGORY_TRUE::vvqqH, 4, 1, 8, 9, 10, 11, 12 }}, // nu(mu/tau) anti-nu(mu/tau) q q h    
    { "qqqqh",     { PROCESS_ID::qqqqh    , EVENT_CATEGORY_TRUE::qqqqH, 4, 1, 8, 9, 10, 11, 12 }},

	// Processes without a Higgs boson
	// Two fermion processes
	{ "2f_z_l",        { PROCESS_ID::f2_z_l       , EVENT_CATEGORY_TRUE::ll, 2, 0, 6, 7 }},
	{ "2f_z_h",        { PROCESS_ID::f2_z_h       , EVENT_CATEGORY_TRUE::qq, 2, 0, 6, 7 }},
	{ "2f_z_nung",     { PROCESS_ID::f2_z_nung    , EVENT_CATEGORY_TRUE::vv, 2, 0, 6, 7 }},
	{ "2f_z_bhabhag",  { PROCESS_ID::f2_z_bhabhag , EVENT_CATEGORY_TRUE::ll, 2, 0, 6, 7 }},
	{ "2f_z_bhabhagg", { PROCESS_ID::f2_z_bhabhagg, EVENT_CATEGORY_TRUE::ll, 2, 0, 6, 7 }},
    
	// Four fermion final states
	{ "4f_zz_l",      { PROCESS_ID::f4_zz_l   , EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }},
	{ "4f_zz_h",      { PROCESS_ID::f4_zz_h   , EVENT_CATEGORY_TRUE::qqqq, 4, 0, 6, 7, 8, 9 }},
	{ "4f_zz_sl",     { PROCESS_ID::f4_zz_sl  , EVENT_CATEGORY_TRUE::llqq, 4, 0, 6, 7, 8, 9 }},
	{ "4f_sze_l",     { PROCESS_ID::f4_sze_l  , EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_sze_sl",    { PROCESS_ID::f4_sze_sl , EVENT_CATEGORY_TRUE::llqq, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_sznu_l",    { PROCESS_ID::f4_sznu_l , EVENT_CATEGORY_TRUE::llvv, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_sznu_sl",   { PROCESS_ID::f4_sznu_sl, EVENT_CATEGORY_TRUE::vvqq, 4, 0, 6, 7, 8, 9 }}, // ? ONLY with vvqq?

	{ "4f_ww_l",      { PROCESS_ID::f4_ww_l , EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }},
	{ "4f_ww_h",      { PROCESS_ID::f4_ww_h , EVENT_CATEGORY_TRUE::qqqq, 4, 0, 6, 7, 8, 9 }},
	{ "4f_ww_sl",     { PROCESS_ID::f4_ww_sl, EVENT_CATEGORY_TRUE::llqq, 4, 0, 6, 7, 8, 9 }},
	{ "4f_sw_l",      { PROCESS_ID::f4_sw_l , EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_sw_sl",     { PROCESS_ID::f4_sw_sl, EVENT_CATEGORY_TRUE::llqq, 4, 0, 6, 7, 8, 9 }}, // ?

	{ "4f_zzorww_l",  { PROCESS_ID::f4_zzorww_l , EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_zzorww_h",  { PROCESS_ID::f4_zzorww_h , EVENT_CATEGORY_TRUE::qqqq, 4, 0, 6, 7, 8, 9 }}, // ?
	{ "4f_szeorsw_l", { PROCESS_ID::f4_szeorsw_l, EVENT_CATEGORY_TRUE::llll, 4, 0, 6, 7, 8, 9 }}, // ?

	// Five fermion final states
	// { "ea_lvvvv",     {  }},

    // 2f_Z_hadronic (only in new production sample; however with some generator level cuts)
    { "z_h0", { PROCESS_ID::f2_z_h, EVENT_CATEGORY_TRUE::qq, 2, 0, 9, 10 }} // z(8) f f | processName: z_h0 
};

#endif