#ifndef FinalStateRecorderCommon_h
#define FinalStateRecorderCommon_h 1

#include <string>
#include <vector>
#include <map>
#include <algorithm>

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
		f4_szeorsw_l = 3202,

		// fffff
		f5_ae_eeevv = 9101,
		f5_ea_eyyyy = 9102,
		f5_ae_elevv = 9103,
		f5_ae_eyyyy = 9104,
		f5_ea_exxxx = 9105,
		f5_ae_evvxx = 9106,
		f5_ae_eeeyy = 9107,
		f5_ae_eevxy = 9108,
		f5_ae_lvvyy = 9109,
		f5_ea_eeevv = 9110,
		f5_ea_ellxx = 9111,
		f5_ae_eeeee = 9112,
		f5_ea_elvxy = 9113,
		f5_ea_evvyy = 9114,
		f5_ea_evlxy = 9115,
		f5_ae_ellvv = 9116,
		f5_ea_vxyyy = 9117,
		f5_ea_eeexx = 9118,
		f5_ae_ellll = 9119,
		f5_ae_eeell = 9120,
		f5_ae_ellxx = 9121,
		f5_ae_elvxy = 9122,
		f5_ae_vxxxy = 9123,
		f5_ae_exxxx = 9124,
		f5_ea_eelvv = 9125,
		f5_ea_eeeee = 9126,
		f5_ae_evvvv = 9127,
		f5_ea_lvvyy = 9128,
		f5_ae_evvyy = 9129,
		f5_ea_exxyy = 9130,
		f5_ea_eevxy = 9131,
		f5_ea_eeeyy = 9132,
		f5_ea_ellyy = 9133,
		f5_ea_vxxxy = 9134,
		f5_ae_vvvxy = 9135,
		f5_ea_vvvxy = 9136,
		f5_ae_ellyy = 9137,
		f5_ea_evvvv = 9138,
		f5_ae_exxyy = 9139,
		f5_ae_evlxy = 9140,
		f5_ae_vxyyy = 9141,
		f5_ae_lllvv = 9142,
		f5_ae_eelvv = 9143,
		f5_ae_lvvxx = 9144,
		f5_ea_ellvv = 9145,
		f5_ae_evexy = 9146,
		f5_ea_ellll = 9147,
		f5_ea_elevv = 9148,
		f5_ea_eeell = 9149,
		f5_ae_lvvvv = 9150,
		f5_ea_evexy = 9151,
		f5_ae_eeexx = 9152,
		f5_ea_lllvv = 9153,
		f5_ea_evvxx = 9154,
		f5_ea_llvxy = 9155,
		f5_ae_llvxy = 9156,
		f5_ea_lvvvv = 9157,

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
		bbcsdu = 83,

		OTHER_EVENTS = 90,
		f5_any = 91,
	};
};

struct PDG {
	enum Values: int {
		d = 1,
		u = 2,
		s = 3,
		c = 4,
		b = 5,
		t = 6,
		e = 11,
		ve = 12,
		μ = 13,
		vμ = 14,
		𝜏 = 15,
		v𝜏 = 16
	};
};

bool vec_contains(std::vector<int> vec, int num) {
	return (std::find(vec.begin(), vec.end(), num) != vec.end());
};

#endif