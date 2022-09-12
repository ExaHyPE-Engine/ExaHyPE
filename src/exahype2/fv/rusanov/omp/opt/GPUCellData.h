// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_CELL_DATA_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_CELL_DATA_H_

#include <cstddef>

namespace exahype2::fv::rusanov::omp::opt {
    struct GPUCellData {
        double* QIn;
        double* QOut;
        double* cellCentre;
        double* cellSize;
        double* t;
        double* dt;
        double* maxEigenvalue;

        int deviceId;
        bool isAllocated;

        explicit GPUCellData();
        ~GPUCellData();
        GPUCellData(GPUCellData const&) = delete;
        GPUCellData& operator = (GPUCellData const&) = delete;
        GPUCellData(GPUCellData&&) = delete;
        GPUCellData& operator = (GPUCellData&&) = delete;

        void allocate(int deviceId, size_t QInSize, size_t QOutSize);
        void free();
    };
}

#endif
