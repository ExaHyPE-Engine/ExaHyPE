// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_FD4_LOOP_BODY_H_
#define _EXAHYPE2_FV_FD4_LOOP_BODY_H_



#include "Functors.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"


namespace exahype2 {
  namespace fv {
    namespace fd4 {
      constexpr int PickAllEntriesFromOutputVector = -1;
      namespace internal {
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolution_LoopBody(
          const double* __restrict__                QIn,
          const QInEnumeratorType&                  QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* __restrict__                      QOut,
          const QOutEnumeratorType&                 QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif


        /**
         * Copy previous solution over into new time step and add source term
         *
         * This is the first step of the method of line, where we basically copy
         * @f$ Q^{(old)} @f$ into @f$ Q^{(new)} @f$. In the same sweep, we also
         * add the algebraic source term subject to the time step size:
         *
         * @f$ Q^{(new)} = Q^{(old)} + \Delta t \cdot S(Q^{(old)}) @f$
         *
         * We reiterate that Q holds two types of entries: The actual data subject
         * to the PDE (and algebraic and differential source terms) and arbitrary
         * auxiliary variables per volume that the user has specified. Auxiliary
         * parameters are coupling terms, material parameters, helper
         * variables that you use to compute some metrics, ... They are not
         * evolved as part of the PDE. As the source term affects only the PDE
         * variables, we copy these entries in Q over and add the source term 
	 * contribution, while the auxilibary parameters remain plain copies.
         *
         *
         * No SIMD declaration here, as we can't SIMDise over functors anyway.
         *
         * @see copySolution_LoopBody() for vectorisation details.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolutionAndAddAlgebraicSourceTerm_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          exahype2::fv::fd4::Source                    AlgebraicSource,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;


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
          exahype2::fv::fd4::NonconservativeProduct      DifferentialSource,
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


        template <class QOutEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          exahype2::fv::fd4::MaxEigenvalue             maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;


      }
    }
  }
}


#include "LoopBody.cpph"

#endif
