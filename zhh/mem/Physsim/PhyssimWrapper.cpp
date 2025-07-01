#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include "TLorentzVector.h"

namespace py = pybind11;

int add(int i, int j) {
    return i + j;
}

py::array_t<double> add_arrays(py::array_t<double> input1, py::array_t<double> input2) {
    py::buffer_info buf1 = input1.request(), buf2 = input2.request();

    if (buf1.ndim != 1 || buf2.ndim != 1)
        throw std::runtime_error("Number of dimensions must be one");

    if (buf1.size != buf2.size)
        throw std::runtime_error("Input shapes must match");

    /* No pointer is passed, so NumPy will allocate the buffer */
    auto result = py::array_t<double>(buf1.size);

    py::buffer_info buf3 = result.request();

    double *ptr1 = static_cast<double *>(buf1.ptr);
    double *ptr2 = static_cast<double *>(buf2.ptr);
    double *ptr3 = static_cast<double *>(buf3.ptr);

    for (size_t idx = 0; idx < buf1.shape[0]; idx++)
        ptr3[idx] = ptr1[idx] + ptr2[idx];

    return result;
}

void physsim_get_z_decay_modes() {
    GENPDTZBoson* fZBosonPtr = new GENPDTZBoson();
    fZBosonPtr->DebugPrint();

    delete fZBosonPtr;
}

py::array_t<double> calc_me_zhh(
    signed char pol_e,
    signed char pol_p,
    int zDecayMode,
    py::array_t<double> input1) {
        
    lcme::LCMEZHH* _zhh = new lcme::LCMEZHH("LCMEZHH", "ZHH", 125., pol_e, pol_p);

    _zhh->SetZDecayMode(zDecayMode); // to muon; see LCME code for internal mappings to PDG ids
    _zhh->SetPropagator(1);
    _zhh->SetMEType(2);
    
    py::buffer_info buf1 = input1.request();

    if (buf1.ndim != 2)
        throw std::runtime_error("Number of dimensions must be one");

    if (buf1.shape[1] != 16)
        throw std::runtime_error("Invalid number of kinematic inputs; need (n x 16) dimensional array. (px,py,pz,E) for (zdecay1particle, zdecay2particle, higgs1, higgs2)");
    
    /* No pointer is passed, so NumPy will allocate the buffer */
    auto result = py::array_t<double>(std::vector<size_t>{(size_t)buf1.shape[0]});

    py::buffer_info buf3 = result.request();

    double *ptrIn = static_cast<double *>(buf1.ptr);
    double *ptrOut = static_cast<double *>(buf3.ptr);

    TLorentzVector zhh_lortz[4]; /* = {
        TLorentzVector(0, 0, 0, 0),
        TLorentzVector(0, 0, 0, 0),
        TLorentzVector(0, 0, 0, 0),
        TLorentzVector(0, 0, 0, 0),
    };*/

    unsigned short i;

    for (size_t idx = 0; idx < buf1.shape[0]; idx++) {
        for (i = 0; i < 4; i++)
                zhh_lortz[i].SetPxPyPzE(
                    ptrIn[16*idx + 4*i],
                    ptrIn[16*idx + 4*i + 1],
                    ptrIn[16*idx + 4*i + 2],
                    ptrIn[16*idx + 4*i + 3]);

        ptrOut[idx] = _zhh->GetMatrixElement2();
    }

    return result;
}

PYBIND11_MODULE(PhyssimWrapper, m) {
    m.doc() = "PhyssimWrapper using pybind11";

    m.def("add", &add, "A function that adds two numbers");
    m.def("add_arrays", &add_arrays, "Add two NumPy arrays");
    m.def("calc_me_zhh", &calc_me_zhh, "Add two NumPy arrays");
    m.def("physsim_get_z_decay_modes", &physsim_get_z_decay_modes, "Get decay modes of Z boson");
}