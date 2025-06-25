#ifndef _CODECPP_
#define _CODECPP_

#ifdef __cplusplus
  #define EXPORT_C extern "C"
#else
  #define EXPORT_C
  #include <stddef.h>
#endif

//============ C++ Only Header =================//
#ifdef __cplusplus  // Enabled only for C++ compilers
#define _USE_MATH_DEFINES
#include <iostream>

#include <ios>
#include <iomanip>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <memory>

// MEElement
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include "TLorentzVector.h"

struct PhysConstants {
  double mb = 4.8;
  double epsilon = 1e-3;
  double dEmax = .1;
  double dpmax = .1;
  double sqrt_s = 500.;
  double system_E = 0.;
  double system_px = 0.;
  double system_py = 0.;
  double system_pz = 0.;
  double mH = 125.;
};

/*
class MEElement
{
    protected:
        PhysConstants *constants;
        
    public:
        virtual ~MEElement() {};
        virtual double single(std::vector<double*> momenta);

        void setConstants(PhysConstants *consts) { constants = consts; };
};
*/

class MEElementPhyssimBase
{
    protected:
        PhysConstants *constants;

    public:
        virtual ~MEElementPhyssimBase() {};
        virtual double single(std::vector<double*> momenta) = 0;
        void setConstants(PhysConstants *consts) { constants = consts; };
};

class MEElementPhyssimZHH: public MEElementPhyssimBase {
    protected:
        PhysConstants *constants;

    private:
        lcme::LCMEZHH *_lcme = nullptr;

    public:
        MEElementPhyssimZHH(PhysConstants *consts, double pol_e, double pol_p, int z_decay_mode);
        ~MEElementPhyssimZHH() { delete _lcme; };
        double single(std::vector<double*> momenta);
};

class MEElementPhyssimZZH: public MEElementPhyssimBase {
    protected:
        PhysConstants *constants;

    private:
        lcme::LCMEZZH *_lcme = nullptr;

    public:
        MEElementPhyssimZZH(PhysConstants *consts, double pol_e, double pol_p, int z1_decay_mode, int z2_decay_mode);
        ~MEElementPhyssimZZH() { delete _lcme; };
        double single(std::vector<double*> momenta);
};

using vec3 = std::array<double,3>;

class calc_me {
  private:
    PhysConstants constants; // TODO

    // MEM related
    int err_map[11]{ -1, -2, -3, -4, -5, -6, -7, -8, -9, -10 , -11 };

    void kin_debug_print();

    // Other often needed stuff
    double mb_pow2 = std::pow(4.8, 2.);
    bool mem_init_run{false};

  protected:
    MEElementPhyssimBase *me_element = nullptr;

  public:
    calc_me(double energy); // , std::string param_card);
    calc_me(calc_me const&)            = delete;
    calc_me& operator=(calc_me const&) = delete;
    ~calc_me() {
      delete me_element;
    };

    void set_helicity(int particle, int helicity);
    double* calc_me_multi(double momenta[], int n_elements, double buffer[]);

    // MEM related
    void mem_init(double evt_constants[]);
    void me_set(int me_type, bool is_signal); // 0: MG5; 1: Physsim
};

#endif //-- End of __cplusplus definition //

//============ C-interface for class calc_me ============//

// Opaque pointer type alias for C-lang
typedef void* pStat;

EXPORT_C pStat   calc_me_new                  (const char param_card[], double energy);
EXPORT_C void    calc_me_set_helicity         (pStat self, int particle, int helicity);
EXPORT_C void    calc_me_del                  (pStat self);
EXPORT_C double* calc_me_multi                (pStat self, double momenta[], int n_elements);
EXPORT_C void    calc_me_me_set               (pStat self, int me_type);

#endif