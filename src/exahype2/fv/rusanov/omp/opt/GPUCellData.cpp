#include "GPUCellData.h"
#include "Rusanov.h"

#include <omp.h>

exahype2::fv::rusanov::omp::opt::GPUCellData::GPUCellData()
    : deviceId(-1)
    , isAllocated(false) {}

exahype2::fv::rusanov::omp::opt::GPUCellData::~GPUCellData() {
    free();
}

void exahype2::fv::rusanov::omp::opt::GPUCellData::allocate(int deviceId_, size_t QInSize, size_t QOutSize) {
    deviceId = deviceId_;
    if (not isAllocated) {
        printf("Allocate GPUCellData\n");
        QIn = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * QInSize * sizeof(double), deviceId));
        QOut = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * QOutSize * sizeof(double), deviceId));
        cellCentre = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * Dimensions * sizeof(double), deviceId));
        cellSize = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * Dimensions * sizeof(double), deviceId));
        t = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * sizeof(double), deviceId));
        dt = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * sizeof(double), deviceId));
        maxEigenvalue = reinterpret_cast<double*>(omp_target_alloc(MaxNumberOfPossiblePatchEntries * sizeof(double), deviceId));
    }
    isAllocated = true;
}

void exahype2::fv::rusanov::omp::opt::GPUCellData::free() {
    if (isAllocated) {
        printf("Free GPUCellData\n");
        omp_target_free(QIn, deviceId);
        omp_target_free(QOut, deviceId);
        omp_target_free(cellCentre, deviceId);
        omp_target_free(cellSize, deviceId);
        omp_target_free(t, deviceId);
        omp_target_free(dt, deviceId);
        omp_target_free(maxEigenvalue, deviceId);
    }
    isAllocated = false;
}
