// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_RUSANOV_H_



#include "Functors.h"
#include "LoopBody.h"


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * Apply a generic Rusanov solver over a set of patches.
       *
       * Here's the explanation of the implementation details marked via _subscript:
       *
       * _overlap1: This solver assumes that the input data has a halo layer of one.
       *
       * _sequential: This solver runs through the individual patches one by one.
       *   We neither exploit any parallelism, not do we merge the patch traversals.
       *   We do however inform Otter that there would be some parallelism in theory.
       *
       * _AoS: All internal (temporary) data structures are organised as AoS.
       *
       * ## Implementation details
       *
       * The function is marked as static and put into a header file. I still want to
       * separate the declaration from the definition, so the actual implementation is
       * in a file with the extension cpph. This indicates that it should (logically)
       * be in a cpp file, but to allow the compiler to inline aggressively, it has to
       * be in the header.
       *
       * As I put the routine in a header and as multiple code parts might include it,
       * we often end up with multiple definition errors in Clang. Therefore, I mark the
       * routine as static.
       */
      static void timeStepWithRusanov_overlap1_sequential_AoS(
        PatchData&              patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     unknowns,
        int                     auxiliaryVariables,
        bool                    evaluateFlux,
        bool                    evaluateNonconservativeProduct,
        bool                    evaluateSource,
        bool                    evaluateMaximumEigenvalueAfterTimeStep,
        Flux                    flux,
        NonconservativeProduct  nonconservativeProduct,
        Source                  source,
        MaxEigenvalue           maxEigenvalue
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;

/*
      double timeStepWithRusanov_overlap1_batching(
        PatchData2d&                                 patchData,
        int                                          numberOfVolumesPerAxisInPatch,
        int                                          unknowns,
        int                                          auxiliaryVariables,
        bool                                         evaluateMaximumEigenvalueAfterTimeStep,
        bool                                         evaluateFlux,
        bool                                         evaluateNCP,
        bool                                         evaluateSource,
        bool                                         evaluateFluxes,
        Flux                    flux,
        NonconservativeProduct  nonconservativeProduct,
        Source                  source,
        MaxEigenvalue           maxEigenvalue
      );


      double timeStepWithRusanov_overlap1_OpenMP_offloading(
        PatchData2d&                                 patchData,
        int                                          numberOfVolumesPerAxisInPatch,
        int                                          unknowns,
        int                                          auxiliaryVariables,
        bool                                         evaluateMaximumEigenvalueAfterTimeStep,
        bool                                         evaluateFlux,
        bool                                         evaluateNCP,
        bool                                         evaluateSource,
        bool                                         evaluateFluxes,
        Flux                    flux,
        NonconservativeProduct  nonconservativeProduct,
        Source                  source,
        MaxEigenvalue           maxEigenvalue
      );
*/
    }
  }
}


#include "Rusanov.cpph"


#endif
