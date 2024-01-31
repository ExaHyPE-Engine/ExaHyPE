// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "exahype2/fd/Functors.h"
#include "exahype2/CellData.h"


#include "KernelVariants.h"


namespace exahype2 {
  namespace fd {
    namespace fd4 {
      /**
       * Fourth-order Finite Differences
       *
       * This routine relies on the fact that we have a halo size of 3.
       *
       *
       * ## Enumerators
       *
       * The enumerators span only one patch each, as we run through the patches
       * one by one. It is important to recognise that the input enumerator creates
       * indices over an array which holds auxiliary (material) parameters, while
       * the output does not hold auxiliary ones. It solely holds the fluxes of the
       * unknowns which are evolved according to the PDE.
       *
       * Both input and output enumerator use AoS ordering.
       *
       *
       * ## Comparison to other solvers
       *
       * Compared to the Finite Differences solver, this routine does not
       * accept any eigenvalue. On the one hand, this is clear, as the finite
       * differences formulation per se does not require an eigenvalue within
       * the discretisation. On the other hand, it is important to keep in mind
       * that the routine is used within a Runge-Kutta context, where the
       * eigenvalue is required for adaptive time stepping after the last
       * Runge-Kutta step, when we have actually computed the new solution.
       * For this reason, it makes sense to outsource the eigenvalue
       * computation to a routine of its own. Compuare to reduceMaxEigenvalue_patchwise_functors()
       * for some details.
       *
       *
       * ## Arguments
       *
       * @param copyOldTimeStepAndScaleWithTimeStepSize The routine can be used
       *   in two ways: Either for a classic explicit Euler or in a Runge-Kutta
       *   context. For the latter, we evaluate the outcome of the right-hand
       *   side of an ODE formulation @f$ \partial _t Q(t) = F(t) @f$, i.e. we
       *   store the F. In an Euler context, we evaluate F, too, but then
       *   immediately scale it with the time step size and add the original
       *   time step's data. We copy stuff over.
       *
       * @param KOSigma Penalty term for the sixth order derivative. Scales
       *   the Kreiss Oliger dissipation. If you set it to 0.0, the term is not
       *   evaluated at all.
       */
      void timeStep_patchwise_heap_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        double                  KOSigma,
        bool                    evaluateFlux,
        bool                    evaluateDifferentialSource, //for ncp
        bool                    evaluateAlgebraicSource, //real source
        bool                    copyOldTimeStepAndScaleWithTimeStepSize,
        DifferentialSourceTermVariant variant,
        Flux                    flux,
        NonconservativeProduct  DifferentialSource,
        Source                  AlgebraicSource
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;

      /**
       * Helper routine to reconstruct the first derivatives
       *
       * This operation computes the first derivatives over a patch due to
       *
       * @Han Your call
       *
       * @param patchData Container for the actual data. The pointer to old
       *   data points to the patch data plus its halo of size haloSize. The
       *   new pointer is to be befilled, i.e. it contains garbage by default.
       *   The fields timeStamp and timeStepSize here are not used, so you
       *   can set them to anything you like. The function doesn't care.
       */
      void reconstruct_first_derivatives(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;

      template <
        typename Solver,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        typename TempDataEnumerator
      >
      static void timeStep_batched_heap_static_calls(
        ::exahype2::CellData&   patchData,
        double                  KOSigma,
        bool                    evaluateFlux,
        bool                    evaluateDifferentialSource, //for ncp
        bool                    evaluateAlgebraicSource, //real source
        bool                    copyOldTimeStepAndScaleWithTimeStepSize,
        DifferentialSourceTermVariant variant
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;


      template <
        typename Solver,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        typename TempDataEnumerator
      >
      static void timeStep_patchwise_heap_static_calls(
        ::exahype2::CellData&   patchData,
        double                  KOSigma,
        bool                    evaluateFlux,
        bool                    evaluateDifferentialSource, //for ncp
        bool                    evaluateAlgebraicSource, //real source
        bool                    copyOldTimeStepAndScaleWithTimeStepSize,
        DifferentialSourceTermVariant variant
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;


      template <
        typename Solver,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        typename TempDataEnumerator
      >
      static void timeStep_batched_heap_multicore_static_calls(
        ::exahype2::CellData&   patchData,
        double                  KOSigma,
        bool                    evaluateFlux,
        bool                    evaluateDifferentialSource, //for ncp
        bool                    evaluateAlgebraicSource, //real source
        bool                    copyOldTimeStepAndScaleWithTimeStepSize,
        DifferentialSourceTermVariant variant
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;
    }
  }
}



#if defined(GPUOffloadingOMP) or defined(SharedOMP)
#include "exahype2/fd/fd4/omp/FD4.h"
#endif

//#if defined(GPUOffloadingCPP) or defined(SharedCPP)
//#include "exahype2/fd/fd4/cpp/FD4.h"
//#endif

/*
#if defined(GPUOffloadingSYCL) or defined(SharedSYCL)
#include "exahype2/fd/fd4/sycl/FD4.h"
#endif
*/

#include "../FD_Helper.cpph"
#include "FD4_batched_static_calls.cpph"
#include "FD4_patchwise_static_calls.cpph"
