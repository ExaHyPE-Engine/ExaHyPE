// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_SCRATCHPAD_DATA_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_SCRATCHPAD_DATA_H_

#include <cstddef>

namespace exahype2::fv::rusanov::omp::opt {
    struct GPUScratchpadData {
        double* tempFluxX;
        double* tempFluxY;
        double* tempFluxZ;
        double* tempNonconservativeProductX;
        double* tempNonconservativeProductY;
        double* tempNonconservativeProductZ;
        double* tempEigenvalueX;
        double* tempEigenvalueY;
        double* tempEigenvalueZ;

        int deviceId;
        bool isAllocated;

        explicit GPUScratchpadData();
        ~GPUScratchpadData();
        GPUScratchpadData(GPUScratchpadData const&) = delete;
        GPUScratchpadData& operator = (GPUScratchpadData const&) = delete;
        GPUScratchpadData(GPUScratchpadData&&) = delete;
        GPUScratchpadData& operator = (GPUScratchpadData&&) = delete;

        void allocate(int deviceId, size_t fluxSize, size_t ncpSize, size_t eigenvalueSize);
        void free();
    };
}

#endif
