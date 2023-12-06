// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fd/Functors.h"
#include "KernelVariants.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"

namespace exahype2 {
  namespace fd {
    namespace fd4 {
      namespace internal {

        /**
         * This function calculates the auxiliary variables as gradietns of the primary
         * variables. The graident is calculated using a 5-point stencil in 1D which has
         * 2 neighbouring cells on each side of the central one. The computation requires    
   * us to have access to 2 halo layers on each side of a given patch.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void computeAuxiliaryVariables_LoopBody(
          double* __restrict__                           QIn,
          const QInEnumeratorType&                       QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          int                                            normal,
          const QOutEnumeratorType&                      QOutEnumerator
        ) InlineMethod;

        /**
         * This function calculates the source term that involves B_i\nabla_iQ where
         * i is the spatial dimension. These involve first gradients of quantitiets 
         * which are now calculated using a standard fourth-order finite difference  
         * scheme with a five-point stancil. This requires one to have access to at 
         * least two halo layers on each side of a given patch.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void computeDifferentialSourceTerm_LoopBody(
          const double* __restrict__                     QIn,
          const QInEnumeratorType&                       QInEnumerator,
          exahype2::fd::NonconservativeProduct           DifferentialSource,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                                         dt,
          int                                            normal,
          double* __restrict__                           QDiffSrc,
          const QOutEnumeratorType&                      QDiffSrcEnumerator,
          DifferentialSourceTermVariant                  variant
        ) InlineMethod;


/*
 *      not implemented yet
 *      ===================

        template <>
        void computeDifferentialSourceTerm_LoopBody(
          const double* __restrict__                     QIn,
          const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          exahype2::fd::NonconservativeProduct           DifferentialSource,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                                         dt,
          int                                            normal,
          double* __restrict__                           QDiffSrc,
          const exahype2::enumerator::AoSLexicographicEnumerator& QDiffSrcEnumerator,
          DifferentialSourceTermVariant                  variant
        ) InlineMethod;
*/


        /**
         * Overlaoded version with static calls.
         */
        template <typename Solver, class QInEnumeratorType, class QOutEnumeratorType>
        static void computeDifferentialSourceTerm_LoopBody(
          const double* __restrict__                     QIn,
          const QInEnumeratorType&                       QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                                         dt,
          int                                            normal,
          double* __restrict__                           QDiffSrc,
          const QOutEnumeratorType&                      QDiffSrcEnumerator,
          DifferentialSourceTermVariant                  variant
        ) InlineMethod;


        /**
         * Add the difference source contributions to one volume.
         *
         * The differential source is defined at the centres of volumes, i.e. we know 
	 * that QDiffSrcX, QDiffSrcY, QDiffSrcZ hold one (vector) entry per volume and 
	 * QDiffSrcEnumerator, which is identical to QOutEnumerator, is aware that we
         * work with volume centres.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QOutEnumeratorType>
        static void updateSolutionWithDifferentialSourceTerm_LoopBody(
          const double* __restrict__                   QDiffSrcX,
          const double* __restrict__                   QDiffSrcY,
          const double* __restrict__                   QDiffSrcZ,
          const QOutEnumeratorType&                    QDiffSrcEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        /**
         * This function calculates the Kreiss Oliger dissipation term that is needed
         * to dissipate high-frequency modes. The KO term is calculated at order N=3
         * in order to make sure that it is of higher order accruacy than the chosen    
         * finite difference scheme used in other parts of the equations. It requires
	 * us to have access to at least 3 halo layers on each side of a given patch.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void computeKreissOligerDissipationTerm_LoopBody(
          const double* __restrict__                     QIn,
          const QInEnumeratorType&                       QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                                         dt,
          int                                            normal,
          double* __restrict__                           QKODsp,
          const QOutEnumeratorType&                      QKODspEnumerator
        ) InlineMethod;

        /**
         * Add the Kreiss Oliger dissipation contributions to one volume.
         *
         * The KO dissipation term is defined at the centres of volumes, i.e. we know 
	 * that QKODspX, QKODspY, QKODspZ hold one (vector) entry per volume and 
	 * QKODspEnumerator, which is identical to QOutEnumerator, is aware that we
         * work with volume centres.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QOutEnumeratorType>
        static void updateSolutionWithKODissipationTerm_LoopBody(
          const double                                 KOSigma,
          const double* __restrict__                   QKODspX,
          const double* __restrict__                   QKODspY,
          const double* __restrict__                   QKODspZ,
          const QOutEnumeratorType&                    QKODspEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif


        /**
         * This routine computes @f$ \partial_i F_i @f$ where i is the
         * argument normal.
         *
         * It is not optimised: Whenever we use the AoS data format,
         * we could directly evaluate the flux terms without any gathering
         * or copying. As we don't optimise/specialise, we always first have
         * to gather the solution before we invoke the user kernel. A similar
         * argument holds or QFluxGathered: the solution is gathered within
         * this temporary array, before we scatter it into the image.
         *
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          exahype2::fd::Flux                           flux,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QFlux,
          const QOutEnumeratorType&                    QFluxEnumerator
        ) InlineMethod;


        /**
         * Overloaded version with static calls.
         */
        template <typename Solver, class QInEnumeratorType, class QOutEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QFlux,
          const QOutEnumeratorType&                    QFluxEnumerator
        ) InlineMethod;


        /**
         * Plain update of flux in a finite differences scheme.
         *
         * We assume that the temp arrays hold the flux in x, y and z
         * direction in the cell centre, i.e. within the degree of freedom.
         * We therefore simply can take these flux components, scale them
         * with dt, and add them to the solution.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QOutEnumeratorType>
        static void updateSolutionWithFlux_LoopBody(
          const double* __restrict__                   tempFluxX,
          const double* __restrict__                   tempFluxY,
          const double* __restrict__                   tempFluxZ,
          const QOutEnumeratorType&                    fluxEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif
      }
    }
  }
}

#include "LoopBody.cpph"

