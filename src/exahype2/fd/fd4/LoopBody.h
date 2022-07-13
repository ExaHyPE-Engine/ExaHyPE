// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FD_FD4_LOOP_BODY_H_
#define _EXAHYPE2_FD_FD4_LOOP_BODY_H_



#include "exahype2/fd/Functors.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"


namespace exahype2 {
  namespace fd {
    namespace fd4 {
      constexpr int PickAllEntriesFromOutputVector = -1;
      namespace internal {
        /**
         * This function calculates the source term that involves B_i\nabla_iQ where
         * i is the spatial dimension. These involve first gradients of quantitiets 
         * which are now calculated using a standard fourth-order finite difference  
         * scheme with a five-point stancil. This requires one to have access to at 
         * least two halo layers on each side of a given patch.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
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
          int                                            pick = PickAllEntriesFromOutputVector
        ) InlineMethod;

        /**
         * Add the difference source contributions to one volume.
         *
         * The differential source is defined at the centres of volumes, i.e. we know 
	 * that QDiffSrcX, QDiffSrcY, QDiffSrcZ hold one (vector) entry per volume and 
	 * QDiffSrcEnumerator, which is identical to QOutEnumerator, is aware that we
         * work with volume centres.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
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
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
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
          const QOutEnumeratorType&                      QKODspEnumerator,
          int                                            pick = PickAllEntriesFromOutputVector
        ) InlineMethod;

        /**
         * Add the Kreiss Oliger dissipation contributions to one volume.
         *
         * The KO dissipation term is defined at the centres of volumes, i.e. we know 
	 * that QKODspX, QKODspY, QKODspZ hold one (vector) entry per volume and 
	 * QKODspEnumerator, which is identical to QOutEnumerator, is aware that we
         * work with volume centres.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
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
         * @todo Plain copy of the FV solver. Not sure if it is the right one for
         * FD. Anyway, it is not optimised at all, even if we use the AoS data format,
         * i.e. do no re-arrangement at all.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QFluxEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                QIn,
          QInEnumeratorType                   QInEnumerator,
          exahype2::fd::Flux                           flux,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QFlux,
          QFluxEnumeratorType                 QFluxEnumerator
        ) InlineMethod;


        /**
         * @todo Plain copy of the FV solver. Not sure if it is the right one for
         * FD.
         */
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QFluxEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithFlux_LoopBody(
          const double* __restrict__                   tempFluxX,
          const double* __restrict__                   tempFluxY,
          const double* __restrict__                   tempFluxZ,
          const QFluxEnumeratorType&                   fluxEnumerator,
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

#endif
