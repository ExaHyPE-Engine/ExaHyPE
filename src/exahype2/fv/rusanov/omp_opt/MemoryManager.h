// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_MEMORY_MANAGER_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_MEMORY_MANAGER_H_

namespace exahype2::fv::rusanov::omp::opt {
    // First draft of a GPU memory manager.
    // TODO: Add synchronisation. Currently, I test it with only 1 thread.
    class MemoryManager {
        int _deviceId;

        int _counter; // Counter "A", use atomics to increment/decrement
        int _numberOfPatches; // "|P|"
        static constexpr int MaxNumberOfPatches = 5; // "|P_max|", magic number for now

        static MemoryManager _singleton;

        MemoryManager() = default;

    public:
        // Memory allocator for temporary data only needed on the device
        struct ScratchpadMemoryAllocator {
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

            inline ScratchpadMemoryAllocator(int deviceId_, int numberOfCells, std::size_t fluxSize, std::size_t ncpSize, std::size_t eigenvalueSize)
                :deviceId(deviceId_) {

                tempFluxX = reinterpret_cast<double*>(omp_target_alloc(fluxEnumeratorSize * sizeof(double), deviceId));
                tempFluxY = reinterpret_cast<double*>(omp_target_alloc(fluxEnumeratorSize * sizeof(double), deviceId));
                tempFluxZ = reinterpret_cast<double*>(omp_target_alloc(fluxEnumeratorSize * sizeof(double), deviceId));
                tempNonconservativeProductX = reinterpret_cast<double*>(omp_target_alloc(ncpHostEnumeratorSize * sizeof(double), deviceId));
                tempNonconservativeProductY = reinterpret_cast<double*>(omp_target_alloc(ncpHostEnumeratorSize * sizeof(double), deviceId));
                tempNonconservativeProductZ = reinterpret_cast<double*>(omp_target_alloc(ncpHostEnumeratorSize * sizeof(double), deviceId));
                tempEigenvalueX = reinterpret_cast<double*>(omp_target_alloc(eigenvalueHostEnumeratorSize * sizeof(double), deviceId));
                tempEigenvalueY = reinterpret_cast<double*>(omp_target_alloc(eigenvalueHostEnumeratorSize * sizeof(double), deviceId));
                tempEigenvalueZ = reinterpret_cast<double*>(omp_target_alloc(eigenvalueHostEnumeratorSize * sizeof(double), deviceId));
            }

            inline ~ScratchpadMemoryAllocator() {
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
        };

        struct CellDataMemoryAllocator {
            double**   QIn;
            tarch::la::Vector<Dimensions,double>*   patchCentre;
            tarch::la::Vector<Dimensions,double>*   patchSize;
            double*    t;
            double*    dt;
            double**   QOut;
            double*    maxEigenvalue;

            CellDataMemoryAllocator(std::size_t QInSize, std::size QOutSize);
        };

        static MemoryManager& getInstance();

        ~MemoryManager() = default;
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator = (const MemoryManager&) = delete;
        MemoryManager& operator = (MemoryManager&&) = delete;
    };
}

#endif
