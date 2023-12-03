// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Functors.h"

#include "exahype2/CellData.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"

namespace exahype2 {
  namespace fd {
      constexpr int PickAllEntriesFromOutputVector = -1;

      /**
       *
       */
      void reduceMaxEigenvalue_patchwise_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     overlap,
        int                     unknowns,
        int                     auxiliaryVariables,
        MaxEigenvalue           maxEigenvalue
      );

      namespace internal {
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
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
         * The complicated way to write =0.
         */
        #if defined(SharedOMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <class QOutEnumeratorType>
        static void clearSolution_LoopBody(
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
         * No SIMD declaration here, as we can't SIMDise over functors anyway.
         *
         * @see copySolution_LoopBody() for vectorisation details.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          exahype2::fd::Source                         AlgebraicSource,
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
         * Overloaded version which uses static invocation instead of a
         * functor.
         */
        template <typename Solver, class QInEnumeratorType, class QOutEnumeratorType>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
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
         * Specialised version.
         */
        template <typename Solver>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* __restrict__                   QIn,
          const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double* __restrict__                         QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator
        ) InlineMethod;

        template <class QOutEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          exahype2::fd::MaxEigenvalue                  maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;

        /**
         * Specialisation without the gather.
         */
        template <>
        double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator,
          exahype2::fd::MaxEigenvalue                  maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;

        /**
         * Overloaded version with static function calls.
         */
        template <typename Solver, class QOutEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;

        /**
         * Overloaded version with static function calls.
         */
        template <typename Solver>
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator,
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

#include "LoopBody.cpph"
