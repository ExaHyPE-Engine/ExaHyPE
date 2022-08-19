// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_SCRATCHPAD_DATA_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_GPU_SCRATCHPAD_DATA_H_

#include <cstddef>

namespace exahype2::fv::rusanov::omp::opt {
    struct GPUScratchpadData {
        double* tempFluxX{nullptr};
        double* tempFluxY{nullptr};
        double* tempFluxZ{nullptr};
        double* tempNonconservativeProductX{nullptr};
        double* tempNonconservativeProductY{nullptr};
        double* tempNonconservativeProductZ{nullptr};
        double* tempEigenvalueX{nullptr};
        double* tempEigenvalueY{nullptr};
        double* tempEigenvalueZ{nullptr};

        int deviceId;

        GPUScratchpadData(int deviceId, size_t fluxSize, size_t ncpSize, size_t eigenvalueSize);
        ~GPUScratchpadData();
        GPUScratchpadData(const GPUScratchpadData&) = delete;
        GPUScratchpadData(GPUScratchpadData&&) = delete;
        GPUScratchpadData& operator = (const GPUScratchpadData&) = delete;
        GPUScratchpadData& operator = (GPUScratchpadData&&) = delete;
    };
}

#endif
