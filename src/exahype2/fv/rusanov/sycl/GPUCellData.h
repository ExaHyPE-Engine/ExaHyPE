// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "config.h"

#if !defined(_EXAHYPE2_FV_RUSANOV_SYCL_GPUCELLDATA_H_) and defined(GPUOffloadingSYCL)
#define _EXAHYPE2_FV_RUSANOV_SYCL_GPUCELLDATA_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/sycl/SYCL.h"


namespace exahype2 {
  /**
   * Forward declaration
   */
  struct CellData;

  namespace fv {
    namespace enumerator {
      /**
       * Forward declaration
       */
      class AoSLexicographicVolumeEnumerator;
    }

    namespace rusanov {
      namespace sycl {
        /**
         * Wrapper around CellData
         *
         * We cannot transfer the CellData objects directly to the GPU in SYCL,
         * as this would require deep copies. This class' signature (public pointers)
         * equals exactly ::exahype2::CellData. However, instead of ::exahype2::CellData's
         * operations, we only provide some routines to move data to the GPU and back.
         * The moving is realised via shared memory.
         *
         * ## Usage
         *
         * Create an instance of this class on the host using the constructor that
         * accepts the CellData. This instance wraps around the real cell data, i.e.
         * creates copies copy of all relevant data in the shared memory.
         *
         * Copy this wrapper object over to the accelerator. This is a byte-wise
         * copy. We can do this, as we have made all pointer of this wrapper object
         * point to shared memory.
         *
         * Destroy the object on the accelerator. No data is actually freed.
         *
         * Call destroy() on the host once the kernels have finished. This actually
         * frees the copies in the shared memory.
         *
         */
        struct GPUCellData {
          double**   QIn;
          tarch::la::Vector<Dimensions,double>*   cellCentre;
          tarch::la::Vector<Dimensions,double>*   cellSize;
          double*    t;
          double*    dt;
          int        numberOfCells;
          double**   QOut;
          double*    maxEigenvalue;

          /**
           * We never create a GPU wrapper object directly
           *
           * CPUCellData is a wrapper and either wraps around an
           * existing ::exahype::CellData object, or it is instantiated
           * as copy of an existing object on the accelerator.
           */
          GPUCellData()  = delete;

          /**
           * This operation is required to support the migration
           * to the GPU. As we label it as default, it is a plain
           * pointer copy. This means: Once we have a valid instance
           * of this object on the host which points to shared memory
           * regions (shared between GPU and CPU), we can simply copy
           * the object over and use it within a queue.
           */
          GPUCellData(const GPUCellData& copy) = default;

          /**
           * Do nothing.
           *
           * The object is either a GPU copy or a wrapper around an
           * existing object on the CPU. If it is the GPU representative,
           * we can always destroy it if we want. The object has no
           * data ownership, so we don't have to free data. If the
           * object is hosted on the CPU, it wraps around an existing
           * ::exahype::CellData object, and we expect the user code
           * to call destroy() explicitly.
           */
          ~GPUCellData() = default;

          /**
           * Create a wrapper around a cell data object which can then be
           * copied to the CPU.
           *
           * The user has to call destroy() eventually to free all internal
           * data structures.
           *
           * @see destroy()
           */
          GPUCellData(
            const ::exahype2::CellData& hostObject,
            const exahype2::enumerator::AoSLexicographicEnumerator& inEnumerator,
            const exahype2::enumerator::AoSLexicographicEnumerator& outEnumerator,
            ::sycl::queue& queue
          );

          void destroy(
            ::exahype2::CellData& hostObject,
            const exahype2::enumerator::AoSLexicographicEnumerator& outEnumerator,
            bool copyEigenvaluesBack,
            ::sycl::queue& queue
          );
        };
      }
    }
  }
}


#endif
