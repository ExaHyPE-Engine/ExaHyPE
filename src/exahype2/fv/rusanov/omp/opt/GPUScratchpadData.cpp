#include "GPUScratchpadData.h"

#include <omp.h>

exahype2::fv::rusanov::omp::opt::GPUScratchpadData::GPUScratchpadData(int deviceId_, size_t fluxSize, size_t ncpSize, size_t eigenvalueSize)
    : deviceId(deviceId_) {

    tempFluxX = reinterpret_cast<double*>(omp_target_alloc(fluxSize * sizeof(double), deviceId));
    tempFluxY = reinterpret_cast<double*>(omp_target_alloc(fluxSize * sizeof(double), deviceId));
    tempFluxZ = reinterpret_cast<double*>(omp_target_alloc(fluxSize * sizeof(double), deviceId));
    tempNonconservativeProductX = reinterpret_cast<double*>(omp_target_alloc(ncpSize * sizeof(double), deviceId));
    tempNonconservativeProductY = reinterpret_cast<double*>(omp_target_alloc(ncpSize * sizeof(double), deviceId));
    tempNonconservativeProductZ = reinterpret_cast<double*>(omp_target_alloc(ncpSize * sizeof(double), deviceId));
    tempEigenvalueX = reinterpret_cast<double*>(omp_target_alloc(eigenvalueSize * sizeof(double), deviceId));
    tempEigenvalueY = reinterpret_cast<double*>(omp_target_alloc(eigenvalueSize * sizeof(double), deviceId));
    tempEigenvalueZ = reinterpret_cast<double*>(omp_target_alloc(eigenvalueSize * sizeof(double), deviceId));
}

exahype2::fv::rusanov::omp::opt::GPUScratchpadData::~GPUScratchpadData() {
    omp_target_free(tempFluxX, deviceId);
    omp_target_free(tempFluxY, deviceId);
    omp_target_free(tempFluxZ, deviceId);
    omp_target_free(tempNonconservativeProductX, deviceId);
    omp_target_free(tempNonconservativeProductY, deviceId);
    omp_target_free(tempNonconservativeProductZ, deviceId);
    omp_target_free(tempEigenvalueX, deviceId);
    omp_target_free(tempEigenvalueY, deviceId);
    omp_target_free(tempEigenvalueZ, deviceId);
}
