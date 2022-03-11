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
       * ## Implementation details (signatures)
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
       *
       * Please study the comments on the namespace internal (hosted in LoopBody.h) for
       * further details why certain code parts are written the way they are written.
       *
       * If I evaluate the fluxes, I have to take into account that diagonal volumes
       * don't hold valid data. After all, we only use split or face-normal fluxes, but
       * do not support transversal waves. So we should mask out those diagonal volumes
       * and should not compute anything on them. Indeed, some applications use
       * assertions to ensure that their user functions are not invoked on uninitialised
       * data and then fail. Unfortunately, such a mask stops the vectoriser from doing
       * its job, as the control flow simply becomes too complicated. So I enable the
       * masking if and only if I am not in release mode.
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


      static void timeStepWithRusanov_overlap1_batched_AoS(
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


//      template <typename Solver, typename TempDataEnumerator = exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>
      template <typename Solver, typename TempDataEnumerator = exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>
      static void timeStepWithRusanov_overlap1_sequential_AoS(
        PatchData&              patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     unknowns,
        int                     auxiliaryVariables,
        bool                    evaluateFlux,
        bool                    evaluateNonconservativeProduct,
        bool                    evaluateSource,
        bool                    evaluateMaximumEigenvalueAfterTimeStep,
        Flux                    flux
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;


      template <typename Solver, typename TempDataEnumerator = exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>
      static void timeStepWithRusanov_overlap1_batched_AoS(
        PatchData&              patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     unknowns,
        int                     auxiliaryVariables,
        bool                    evaluateFlux,
        bool                    evaluateNonconservativeProduct,
        bool                    evaluateSource,
        bool                    evaluateMaximumEigenvalueAfterTimeStep
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;
    }
  }
}


#include "Rusanov_Helper.cpph"
#include "Rusanov_overlap1_sequential_AoS_functors.cpph"
#include "Rusanov_overlap1_sequential_AoS_static_calls.cpph"
#include "Rusanov_overlap1_batched_AoS_functors.cpph"
#include "Rusanov_overlap1_batched_AoS_static_calls.cpph"


#endif
