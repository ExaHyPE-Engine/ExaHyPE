// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_CELL_DATA_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_CELL_DATA_H_

#include <cstddef>

namespace exahype2::fv::rusanov::omp::opt {
    struct GPUCellData {
        double* QIn{nullptr};
        double* QOut{nullptr};
        double* cellCentre{nullptr};
        double* cellSize{nullptr};
        double* t{nullptr};
        double* dt{nullptr};
        double* maxEigenvalue{nullptr};

        int deviceId;

        GPUCellData(int deviceId, int maxNumberOfPatches, size_t QInSize, size_t QOutSize);
        ~GPUCellData();
        GPUCellData(const GPUCellData&) = delete;
        GPUCellData(GPUCellData&&) = delete;
        GPUCellData& operator = (const GPUCellData&) = delete;
        GPUCellData& operator = (GPUCellData&&) = delete;
    };
}

#endif
