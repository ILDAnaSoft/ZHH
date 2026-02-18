#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "physsim/LCMEZHH.h"
#include "physsim/LCMEZZH.h"
#include "physsim/LCMEZZ.h"
#include "TLorentzVector.h"
#include "Python.h"

namespace py = pybind11;

void physsim_get_z_decay_modes() {
    GENPDTZBoson* fZBosonPtr = new GENPDTZBoson();
    fZBosonPtr->DebugPrint();

    delete fZBosonPtr;
}

/*
double* calc_me(
    int which,
    double pol_e,
    double pol_p,
){
    
};*/

py::array_t<double> calc_me_zhh_jm(
    double pol_e,
    double pol_p,
    int zDecayMode,
    py::array_t<double> input_kinematics,
    py::array_t<int8_t> jet_matching) {
     
    auto kref = input_kinematics.unchecked<2>();
    auto jmref = jet_matching.unchecked<2>();
    size_t nrows = (size_t)kref.shape(0);

    if (kref.shape(1) != 24)
        throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 24) in order (px,py,pz,E) for (zdecay1particle, zdecay2particle, higgs1decay1particle, higgs1decay2particle, higgs2decay1particle, higgs2decay2particle) where zdecay1particle is positive, 2 is negative");
    
    double *arr = new double[nrows];

    // load jet matching
    if (jmref.shape(1) != 4)
        throw std::runtime_error("Invalid number of jet matching parameters");

    // matrix element initialization
    lcme::LCMEZHH* calcme = new lcme::LCMEZHH("LCMEZHH", "ZHH", 125., pol_e, pol_p);
    calcme->SetZDecayMode(zDecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);

    // jet matching indidces and input kinematics
    int jet_idx1, jet_idx2, jet_idx3, jet_idx4;
    TLorentzVector lortz[4];
    Int_t hel[2] = {-1, 1};

    for (size_t idx = 0; idx < kref.shape(0); idx++) {
        jet_idx1 = (int)jmref(idx, 0);
        jet_idx2 = (int)jmref(idx, 1);
        jet_idx3 = (int)jmref(idx, 2);
        jet_idx4 = (int)jmref(idx, 3);

        //std::cerr << idx << "I: " << jet_idx1 << ", " << jet_idx2 << ", " << jet_idx3 << ", " << jet_idx4 << std::endl;
        //std::cerr << idx << "P: " << kref(idx, 0) << ", ";
        //std::cerr << kref(idx, 1) << ", ";
        //std::cerr << kref(idx, 2) << ", ";
        //std::cerr << kref(idx, 3) << std::endl;

        lortz[0].SetPxPyPzE(kref(idx, 0), kref(idx, 1), kref(idx, 2), kref(idx, 3)); // lepton 1

        lortz[1].SetPxPyPzE(kref(idx, 4), kref(idx, 5), kref(idx, 6), kref(idx, 7)); // lepton 2

        lortz[2].SetPxPyPzE(kref(idx,  8 + 4*jet_idx1) + kref(idx,  8 + 4*jet_idx2),
                            kref(idx,  9 + 4*jet_idx1) + kref(idx,  9 + 4*jet_idx2),
                            kref(idx, 10 + 4*jet_idx1) + kref(idx, 10 + 4*jet_idx2),
                            kref(idx, 11 + 4*jet_idx1) + kref(idx, 11 + 4*jet_idx2)); // higgs 1

        lortz[3].SetPxPyPzE(kref(idx,  8 + 4*jet_idx3) + kref(idx, +  8 + 4*jet_idx4),
                            kref(idx,  9 + 4*jet_idx3) + kref(idx, +  9 + 4*jet_idx4),
                            kref(idx, 10 + 4*jet_idx3) + kref(idx, + 10 + 4*jet_idx4),
                            kref(idx, 11 + 4*jet_idx3) + kref(idx, + 11 + 4*jet_idx4)); // higgs 2

        calcme->SetMomentumFinal(lortz);
        arr[idx] = calcme->GetMatrixElement2(); // calcme->GetMatrixElement2ByHelicity(hel);

        //std::cerr << "LCME ZHH: m(Z)=" << (lortz[0] + lortz[1]).M() << " | m(H1)=" << lortz[2].M() << " | m(H2)=" << lortz[3].M() << std::endl;
    }

    py::capsule free_when_done(arr, [](void *val) {
        double *arr = reinterpret_cast<double *>(val);
        std::cerr << "Element [0] = " << arr[0] << "\n";
        std::cerr << "freeing memory @ " << val << "\n";
        if (arr)
            free(arr);
    });

    return py::array_t<double>({nrows}, {sizeof(double)}, arr, free_when_done);
}

py::array_t<double> calc_me_zzh_jm(
    double pol_e,
    double pol_p,
    int z1DecayMode,
    int z2DecayMode,
    py::array_t<double> input_kinematics,
    py::array_t<int8_t> jet_matching) {

    auto kref = input_kinematics.unchecked<2>();
    auto jmref = jet_matching.unchecked<2>();
    size_t nrows = (size_t)kref.shape(0);

    if (kref.shape(1) != 24)
        throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 24) in order (px,py,pz,E) for (zdecay1particle, zdecay2particle, higgs1decay1particle, higgs1decay2particle, higgs2decay1particle, higgs2decay2particle) where zdecay1particle is positive, 2 is negative");
    
    double *arr = new double[nrows];

    if (jmref.shape(1) != 4)
        throw std::runtime_error("Invalid number of jet matching parameters");

    // matrix element initialization
    lcme::LCMEZZH* calcme = new lcme::LCMEZZH("LCMEZZH", "ZZH", 125., pol_e, pol_p);
    calcme->SetZDecayMode(z1DecayMode, z2DecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);

    // jet matching indidces and input kinematics
    int jet_idx1, jet_idx2, jet_idx3, jet_idx4;
    TLorentzVector lortz[5];
    Int_t hel[2] = {-1, 1};

    for (size_t idx = 0; idx < kref.shape(0); idx++) {
        jet_idx1 = (int)jmref(idx, 0);
        jet_idx2 = (int)jmref(idx, 1);
        jet_idx3 = (int)jmref(idx, 2);
        jet_idx4 = (int)jmref(idx, 3);

        // std::cerr << idx << ": " << jet_idx1 << ", " << jet_idx2 << ", " << jet_idx3 << ", " << jet_idx4 << std::endl;

        lortz[0].SetPxPyPzE(kref(idx, 0), kref(idx, 1), kref(idx, 2), kref(idx, 3)); // lepton 1

        lortz[1].SetPxPyPzE(kref(idx, 4), kref(idx, 5), kref(idx, 6), kref(idx, 7)); // lepton 2

        lortz[2].SetPxPyPzE(kref(idx, 8 + 4*jet_idx3), kref(idx, 9 + 4*jet_idx3), kref(idx, 10 + 4*jet_idx3), kref(idx, 11 + 4*jet_idx3)); // z2 decay 1 particle
        lortz[3].SetPxPyPzE(kref(idx, 8 + 4*jet_idx4), kref(idx, 9 + 4*jet_idx4), kref(idx, 10 + 4*jet_idx4), kref(idx, 11 + 4*jet_idx4)); // z2 decay 2 particle

        lortz[4].SetPxPyPzE(kref(idx,  8 + 4*jet_idx1) + kref(idx,  8 + 4*jet_idx2),
                            kref(idx,  9 + 4*jet_idx1) + kref(idx,  9 + 4*jet_idx2),
                            kref(idx, 10 + 4*jet_idx1) + kref(idx, 10 + 4*jet_idx2),
                            kref(idx, 11 + 4*jet_idx1) + kref(idx, 11 + 4*jet_idx2)); // higgs 1

        calcme->SetMomentumFinal(lortz);
        arr[idx] = calcme->GetMatrixElement2(); // calcme->GetMatrixElement2ByHelicity(hel);

        //std::cerr << "LCME ZZH: m(Z1)=" << (lortz[0] + lortz[1]).M() << " | m(Z2)=" << (lortz[2] + lortz[3]).M() << " | m(H)=" << lortz[4].M() << std::endl;
    }

    py::capsule free_when_done(arr, [](void *val) {
        double *arr = reinterpret_cast<double *>(val);
        std::cerr << "Element [0] = " << arr[0] << "\n";
        std::cerr << "freeing memory @ " << val << "\n";
        if (arr)
            free(arr);
    });

    return py::array_t<double>({nrows}, {sizeof(double)}, arr, free_when_done);
}

py::array_t<double> calc_me_zzh(
    double pol_e,
    double pol_p,
    int z1DecayMode,
    int z2DecayMode,
    py::array_t<double> input_kinematics) {
        
    lcme::LCMEZZH* calcme = new lcme::LCMEZZH("LCMEZZH", "ZZH", 125., pol_e, pol_p);

    calcme->SetZDecayMode(z1DecayMode, z2DecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);
    
    py::buffer_info bufIn = input_kinematics.request();

    if (bufIn.ndim != 2)
        throw std::runtime_error("Number of dimensions must be two");

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

py::array_t<double> calc_me_zz(
    double pol_e,
    double pol_p,
    int z1DecayMode,
    int z2DecayMode,
    py::array_t<double> input_kinematics,
    std::optional<py::array_t<double>> out_rest_frame_data
) {
        
    lcme::LCMEZZ* calcme = new lcme::LCMEZZ("LCMEZZ", "ZZ", pol_e, pol_p, 0);

    calcme->SetZDecayMode(z1DecayMode, z2DecayMode);
    calcme->SetPropagator(1);
    //calcme->SetMEType(2);
    
    py::buffer_info bufIn = input_kinematics.request();

    if (bufIn.ndim != 2)
        throw std::runtime_error("Number of dimensions must be two");

    if (bufIn.shape[1] != 16)
        throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 16) in order (px,py,pz,E) for (z1decay1particle, z1decay2particle, z2decay1particle, z2decay2particle) where the first(second) decay particle of each Z is positive(negative)");
    
    auto result = py::array_t<double>(std::vector<size_t>{(size_t)bufIn.shape[0]});

    py::buffer_info bufOut = result.request();

    double *ptrIn = static_cast<double *>(bufIn.ptr);
    double *ptrOut = static_cast<double *>(bufOut.ptr);

    // output cosTheta, phi angles in rest frame of Z1,Z2,ZZ
    py::buffer_info buf_rf_data;
    double *rf_angles = nullptr;
    bool output_rest_frame_data = out_rest_frame_data.has_value();
    if (output_rest_frame_data) {
        buf_rf_data = out_rest_frame_data.value().request();

        if (buf_rf_data.ndim != 2 || buf_rf_data.shape[1] != 9)
            throw std::runtime_error("Invalid number of kinematic inputs; need array of size (n x 9) and will be filled in order (Q^2, CosTheta, Phi) for (Z1, Z2, ZZ system)");

        rf_angles = static_cast<double *>(bufOut.ptr);
    }    

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

        if (output_rest_frame_data) {
            rf_angles[idx * 9    ] = calcme->GetQ2Z1();
            rf_angles[idx * 9 + 1] = calcme->GetCosThetaZ1F();
            rf_angles[idx * 9 + 2] = calcme->GetPhiZ1F();

            rf_angles[idx * 9 + 3] = calcme->GetQ2Z2();
            rf_angles[idx * 9 + 4] = calcme->GetCosThetaZ2F();
            rf_angles[idx * 9 + 5] = calcme->GetPhiZ2F();

            rf_angles[idx * 9 + 6] = calcme->GetQ2ZZ();
            rf_angles[idx * 9 + 7] = calcme->GetCosTheta();
            rf_angles[idx * 9 + 8] = calcme->GetPhi();
        }
        
        //std::cerr << "-> " << ptrOut[idx] << std::endl;
    }

    delete calcme;

    return result;
}

PYBIND11_MODULE(PhyssimWrapper, m) {
    m.doc() = "PhyssimWrapper using pybind11";

    //m.def("calc_me_zhh", &calc_me_zhh, "Calculate e+e- -> ZHH matrix element");
    m.def("calc_me_zhh_jm", &calc_me_zhh_jm, "Calculate e+e- -> ZHH matrix element with a given jet matching"); // , py::call_guard<py::gil_scoped_release>());
    m.def("calc_me_zzh_jm", &calc_me_zzh_jm, "Calculate e+e- -> ZZH matrix element with a given jet matching"); // , py::call_guard<py::gil_scoped_release>());
    //m.def("calc_me_zzh", &calc_me_zzh, "Calculate e+e- -> ZZH matrix element");
    /*
    m.def("calc_me_zz", &calc_me_zz, "Calculate e+e- -> ZZ matrix element",
        py::arg("pol_e"), py::arg("pol_p"),
        py::arg("z1DecayMode"), py::arg("z2DecayMode"),
        py::arg("input_kinematics"),
        py::arg("out_rest_frame_data") = py::none());*/

    //m.def("calc_me", &calc_me, "Calculate polarized-weighted squared matrix elements");
    m.def("physsim_get_z_decay_modes", &physsim_get_z_decay_modes, "Get decay modes of Z boson");
}