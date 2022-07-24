#include "GPUCellData.h"

#include <omp.h>

exahype2::fv::rusanov::omp::opt::GPUCellData::GPUCellData(int deviceId_, int maxNumberOfPatches, size_t QInSize, size_t QOutSize)
    : deviceId(deviceId_) {

    QIn = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * QInSize * sizeof(double), deviceId));
    QOut = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * QOutSize * sizeof(double), deviceId));
    cellCentre = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * Dimensions * sizeof(double), deviceId));
    cellSize = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * Dimensions * sizeof(double), deviceId));
    t = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * sizeof(double), deviceId));
    dt = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * sizeof(double), deviceId));
    maxEigenvalue = reinterpret_cast<double*>(omp_target_alloc(maxNumberOfPatches * sizeof(double), deviceId));
}

exahype2::fv::rusanov::omp::opt::GPUCellData::~GPUCellData() {
    omp_target_free(QIn, deviceId);
    omp_target_free(QOut, deviceId);
    omp_target_free(cellCentre, deviceId);
    omp_target_free(cellSize, deviceId);
    omp_target_free(t, deviceId);
    omp_target_free(dt, deviceId);
    omp_target_free(maxEigenvalue, deviceId);
}
