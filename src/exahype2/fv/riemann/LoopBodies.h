// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/enumerator/enumerator.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/Solver.h"
#include "Functors.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"

namespace exahype2::fv::riemann::loopbodies {
#pragma omp declare simd
  template <class QInEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    copySolution(
      const double* __restrict__ QIn,
      const QInEnumeratorType&                    QInEnumerator,
      int                                         patchIndex,
      const ::tarch::la::Vector<Dimensions, int>& volumeIndex,
      int                                         unknown,
      double* __restrict__ QOut,
      const QOutEnumeratorType& QOutEnumerator
    ) InlineMethod {
    QOut[QOutEnumerator(patchIndex, volumeIndex, unknown)] = QIn
      [QInEnumerator(patchIndex, volumeIndex, unknown)];
  };


#pragma omp declare simd
  template <
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    class QInEnumeratorType,
    class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void copySolutionAndAddSourceTerm(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const SourceFunctor&                           sourceFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void copySolutionAndAddSourceTerm(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
    const SourceFunctor&                           sourceFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    double* __restrict__ QOut,
    const enumerator::AoSLexicographicEnumerator& QOutEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    copySolutionAndAddSourceTerm(
      const double* __restrict__ QIn,
      const QInEnumeratorType&                       QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      double* __restrict__ QOut,
      const QOutEnumeratorType& QOutEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    copySolutionAndAddSourceTerm(
      const double* __restrict__ QIn,
      const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      double* __restrict__ QOut,
      const enumerator::AoSLexicographicEnumerator& QOutEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    class QInEnumeratorType,
    class EigenvaluesEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeEigenvalues(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const EigenvaluesFunctor&                      eigenvaluesFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ eigenvalues,
    const EigenvaluesEnumeratorType& eigenvaluesEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeEigenvalues(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
    const EigenvaluesFunctor&                      eigenvaluesFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ eigenvalues,
    const enumerator::AoSLexicographicEnumerator& eigenvaluesEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <
    class SolverType,
    class QInEnumeratorType,
    class EigenvaluesEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeEigenvalues(
      const double* __restrict__ QIn,
      const QInEnumeratorType&                       QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ eigenvalues,
      const EigenvaluesEnumeratorType& eigenvaluesEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeEigenvalues(
      const double* __restrict__ QIn,
      const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ eigenvalues,
      const enumerator::AoSLexicographicEnumerator& eigenvaluesEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    class QInEnumeratorType,
    class FluxEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeFlux(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const FluxFunctor&                             fluxFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ flux,
    const FluxEnumeratorType& fluxEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeFlux(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
    const FluxFunctor&                             fluxFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ flux,
    const enumerator::AoSLexicographicEnumerator& fluxEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class FluxEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeFlux(
      const double* __restrict__ QIn,
      const QInEnumeratorType&                       QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ flux,
      const FluxEnumeratorType& fluxEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class SolverType, class FluxEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeFlux(
      const double* __restrict__ QIn,
      const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ flux,
      const FluxEnumeratorType& fluxEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeFlux(
      const double* __restrict__ QIn,
      const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ flux,
      const enumerator::AoSLexicographicEnumerator& fluxEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    class QInEnumeratorType,
    class NCPFaceEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeNonconservativeFlux(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const NonconservativeProductFunctor&           ncpFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ ncp,
    const NCPFaceEnumeratorType& ncpEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <
    class SolverType,
    class QInEnumeratorType,
    class NCPFaceEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeNonconservativeFlux(
      const double* __restrict__ QIn,
      const QInEnumeratorType&                       QInEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ ncp,
      const NCPFaceEnumeratorType& ncpEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class NCPFaceEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    updateSolutionWithNonconservativeFlux(
      const double* __restrict__ ncpX,
      const double* __restrict__ ncpY,
      const double* __restrict__ ncpZ,
      const NCPFaceEnumeratorType&                   ncpEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      int                                            unknown,
      double                                         dt,
      double* __restrict__ QOut,
      const QOutEnumeratorType& QOutEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class NCPFaceEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    updateSolutionWithNonconservativeFlux(
      const double* __restrict__ ncp,
      const NCPFaceEnumeratorType&                   ncpEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    faceIndex,
      int                                            unknown,
      double                                         dt,
      int                                            normal,
      double* __restrict__ QOut,
      const QOutEnumeratorType& QOutEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeRiemannSolution(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator& QInEnumerator,
    const RiemannFunctor&                         riemannFunctor,
    const double* __restrict__ flux,
    const enumerator::AoSLexicographicEnumerator& fluxEnumerator,
    const double* __restrict__ eigenvalues,
    const enumerator::AoSLexicographicEnumerator&  eigenvaluesEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ APDQ,
    double* __restrict__ AMDQ,
    const enumerator::AoSLexicographicEnumerator& DQEnumerator,
    double* __restrict__ maxEigenvalue,
    const enumerator::AoSLexicographicEnumerator& maxEigenvalueEnumerator
  ) InlineMethod;


#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void
    computeRiemannSolution(
      const double* __restrict__ QIn,
      const enumerator::AoSLexicographicEnumerator& QInEnumerator,
      const double* __restrict__ flux,
      const enumerator::AoSLexicographicEnumerator& fluxEnumerator,
      const double* __restrict__ eigenvalues,
      const enumerator::AoSLexicographicEnumerator&  eigenvaluesEnumerator,
      const ::tarch::la::Vector<Dimensions, double>& patchCentre,
      const ::tarch::la::Vector<Dimensions, double>& patchSize,
      int                                            patchIndex,
      const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ APDQ,
      double* __restrict__ AMDQ,
      const enumerator::AoSLexicographicEnumerator& DQEnumerator,
      double* __restrict__ maxEigenvalue,
      const enumerator::AoSLexicographicEnumerator& maxEigenvalueEnumerator
    ) InlineMethod;


#pragma omp declare simd
  template <class RiemannEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void updateSolutionWithRiemannSolution(
    const double* __restrict__ leftUpdatesX,
    const double* __restrict__ rightUpdatesX,
    const double* __restrict__ belowUpdatesY,
    const double* __restrict__ aboveUpdatesY,
    const double* __restrict__ backwardUpdatesZ,
    const double* __restrict__ forwardUpdatesZ,
    const RiemannEnumeratorType&                 riemannEnumerator,
    const tarch::la::Vector<Dimensions, double>& patchCentre,
    const tarch::la::Vector<Dimensions, double>& patchSize,
    int                                          patchIndex,
    const tarch::la::Vector<Dimensions, int>&    volumeIndex,
    int                                          unknown,
    double                                       dt,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


#pragma omp declare simd
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions double reduceMaxEigenvalue(
    const double* __restrict__ maxEigenvalueX,
    const double* __restrict__ maxEigenvalueY,
    const double* __restrict__ maxEigenvalueZ,
    enumerator::AoSLexicographicEnumerator    maxEigenvalueEnumerator,
    int                                       patchIndex,
    const tarch::la::Vector<Dimensions, int>& volumeIndex
  ) InlineMethod;
} // namespace exahype2::fv::riemann::loopbodies

#include "LoopBodies.cpph"
