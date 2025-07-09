#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include "TLorentzVector.h"

namespace py = pybind11;

void physsim_get_z_decay_modes() {
    GENPDTZBoson* fZBosonPtr = new GENPDTZBoson();
    fZBosonPtr->DebugPrint();

    delete fZBosonPtr;
}

py::array_t<double> calc_me_zhh(
    double pol_e,
    double pol_p,
    int zDecayMode,
    py::array_t<double> kinematics) {
        
    lcme::LCMEZHH* calcme = new lcme::LCMEZHH("LCMEZHH", "ZHH", 125., pol_e, pol_p);

    calcme->SetZDecayMode(zDecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);
    
    py::buffer_info bufIn = kinematics.request();

    if (bufIn.ndim != 2)
        throw std::runtime_error("Number of dimensions must be one");

    if (bufIn.shape[1] != 16)
        throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 16) in order (px,py,pz,E) for (zdecay1particle, zdecay2particle, higgs1, higgs2) where zdecay1particle is positive, 2 is negative");
    
    auto result = py::array_t<double>(std::vector<size_t>{(size_t)bufIn.shape[0]});

    py::buffer_info bufOut = result.request();

    double *ptrIn = static_cast<double *>(bufIn.ptr);
    double *ptrOut = static_cast<double *>(bufOut.ptr);

    TLorentzVector lortz[4];

    unsigned short i;

    for (size_t idx = 0; idx < bufIn.shape[0]; idx++) {
        for (i = 0; i < 4; i++) {
            lortz[i].SetPxPyPzE(
                ptrIn[16*idx + 4*i],
                ptrIn[16*idx + 4*i + 1],
                ptrIn[16*idx + 4*i + 2],
                ptrIn[16*idx + 4*i + 3]);
            //std::cerr << lortz[i].E() << " ";
        }

        calcme->SetMomentumFinal(lortz);
        ptrOut[idx] = calcme->GetMatrixElement2();
        
        //std::cerr << "-> " << ptrOut[idx] << std::endl;
    }

    //delete calcme;

    return result;
}

py::array_t<double> calc_me_zzh(
    double pol_e,
    double pol_p,
    int z1DecayMode,
    int z2DecayMode,
    py::array_t<double> kinematics) {
        
    lcme::LCMEZZH* calcme = new lcme::LCMEZZH("LCMEZZH", "ZZH", 125., pol_e, pol_p);

    calcme->SetZDecayMode(z1DecayMode, z2DecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);
    
    py::buffer_info bufIn = kinematics.request();

    if (bufIn.ndim != 2)
        throw std::runtime_error("Number of dimensions must be one");

    if (bufIn.shape[1] != 20)
        throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 20) in order (px,py,pz,E) for (z1decay1particle, z1decay2particle, z2decay1particle, z2decay2particle, higgs1) where zdecay1particle is positive, 2 is negative");
    
    /* No pointer is passed, so NumPy will allocate the buffer */
    auto result = py::array_t<double>(std::vector<size_t>{(size_t)bufIn.shape[0]});

    py::buffer_info bufOut = result.request();

    double *ptrIn = static_cast<double *>(bufIn.ptr);
    double *ptrOut = static_cast<double *>(bufOut.ptr);

    TLorentzVector lortz[5];

    unsigned short i;

    for (size_t idx = 0; idx < bufIn.shape[0]; idx++) {
        for (i = 0; i < 5; i++) {
            lortz[i].SetPxPyPzE(
                ptrIn[20*idx + 4*i],
                ptrIn[20*idx + 4*i + 1],
                ptrIn[20*idx + 4*i + 2],
                ptrIn[20*idx + 4*i + 3]);
            //std::cerr << lortz[i].E() << " ";
        }

        calcme->SetMomentumFinal(lortz);
        ptrOut[idx] = calcme->GetMatrixElement2();
        // std::cerr << "-> " << ptrOut[idx] << std::endl;
    }

    delete calcme;

    return result;
}

PYBIND11_MODULE(PhyssimWrapper, m) {
    m.doc() = "PhyssimWrapper using pybind11";

    m.def("calc_me_zhh", &calc_me_zhh, "Calculate e+e- -> ZHH matrix element");
    m.def("calc_me_zzh", &calc_me_zzh, "Calculate e+e- -> ZZH matrix element");
    m.def("physsim_get_z_decay_modes", &physsim_get_z_decay_modes, "Get decay modes of Z boson");
}