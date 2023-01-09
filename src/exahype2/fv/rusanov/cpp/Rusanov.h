// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <vector>
#include <iterator>
#include <execution>
#include <algorithm>
#include <numeric>
#include "extra/cartesian_product.h"

namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * The name sequential is a little bit irritating in a GPU context.
       * What it means is that the code runs logically through the patches
       * one by one and issues all kernel on one patch before it continues
       * with the next one. Obviously, the patches are all independent of
       * each other. Therefore, we can deploy this sequence of patch
       * updates to a distributed for over the team.
       *
       * ## Memory management
       *
       * If you run this routine without GPU offloading, we place all temporary data on the heap.
       * If GPUs are to be used, then we create pointer for the temporary data, but we assign
       * them all null, as the actual temporary data is to be create on the GPU. This
       * 'create a pointer which is a nullptr' ensures that the code still compiles on the host
       * even though it is semantically broken as the temporary data is not acquired. However, we
       * want to run on the GPU, so we use the pragma omp enter data instruction to allocate the
       * data directly on the device.
       *
       * ## Patch data transfer
       *
       * All the data within the CellData object is trivially copyable besides the
       * actual patch sizes and positions. Trivially copyable means that they are
       * plain double arrays. That is, we can map them onto the device. The actual
       * input and output data is stored as pointer to pointers. Here, we have to
       * loop over the entries and copy each individual pointer over separatedly.
       *
       * The positions and sizes of the patches are in principle trivially copyable
       * as well, as they simply host arrays of known sizes. However, it is quite
       * cumbersome to tell OpenMP that this stuff can just be copied. It also might
       * become annoying the compiler decided to add padding bytes. So we copy those
       * guys manually into one large buffer of double values, and then we reconstruct
       * the objects on the device. In this particular context, the resulting overhead
       * is acceptable, as this meta data is not extraordinary large.
       */
      namespace cpp {
        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_patchwise_heap_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;


        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_batched_heap_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;

        #if defined(GPUOffloadingCPP)
        /**
         * On the GPU, we don't need any of exahype2::CellData's fancy routines or
         * dynamic data management. We simply have to be able to create a struct
         * on the accelerator, which we then befill with the required pointers
         * which we copy over from the host.
         *
         * Therefore, this class is a mere copy of exahype2::CellData without any
         * of its routines. The only different to the host variant (from a data
         * field point of view) is that numberOfCells is not const.
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
        };
        #endif

        /**
         * Dummy version delegating to one well-defined proper kernel. By
         * offering such a routine, we can generate generic compute code
         * in Python without knowing which kernel variants are on offer for
         * a particular GPU backend.
         */
        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_generic_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        ) {
          timeStepWithRusanov_batched_heap_static_calls<
            Solver,
            numberOfVolumesPerAxisInPatch,
            haloSize,
            unknowns,
            auxiliaryVariables,
            TempDataEnumerator
          >(
            targetDevice,
            patchData,
            evaluateFlux,
            evaluateNonconservativeProduct,
            evaluateSource,
            evaluateMaximumEigenvalueAfterTimeStep
          );
        }
      }
    }
  }
}


#include "Rusanov_patchwise_heap_static_calls.cpph"
#include "Rusanov_batched_heap_static_calls.cpph"

