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

namespace exahype2::fv::rusanov::loopbodies {
  /**
   *
   * @namespace exahype2::fv::rusanov::loopbodies
   *
   * ## Specialisation
   *
   * Where it makes sense, we provide specialisations of the routines which
   * exploit that input data and output data are available as AoS. All of
   * ExaHyPE's user routines are phrased as AoS, i.e., you are given one or two
   * or a set of structs representing a volume, and then you return a struct
   * (tuple) of outputs. If the underlying data storage is not organised as AoS, then
   * we have to first gather data, pass it into the user functions, and then
   * fetch it back. If data however is organised as AoS, we can directly
   * invoke the user's functions on the data. No need to gather and scatter.
   * While I refer to these routines as specialisations, they are technically
   * realised via template overloading.
   *
   *
   * Further to the specialisation, each loop body routine is also available
   * in its static stateless counterpart, i.e., in a version which does not
   * accept a functor but directly invokes the underlying solver's routines.
   * These special cases refer to the actual static routines which are there to
   * support accelerators.
   *
   * ## SIMD
   *
   * It seems that the compilers refuse to vectorise over the loop body
   * routines. It expects them to be declared as SIMD. This is strange
   * given that we inline everything, but if the compiler wants it, we can
   * give it to the tool.
   *
   */

  /**
   * Copy solution from QIn to QOut. Typically, both fields have different
   * halo sizes, so we have to use two different enumerators. The loop body
   * copies over exactly one scalar.
   *
   * There is no specialisation for this one, as it is a plain copy anyway.
   *
   * ## Vectorisation
   *
   * One might be tempted to think of this as a plain memcopy. However, it
   * is more than that: The input data has a halo, while the output data
   * (typically) has no halo. So they are not of the same size. We therefore
   * can copy lines from the input in one rush, but we cannot copy all of it.
   * In the outer loop calling this routine, we exploit the fact that we
   * know that we have AoS for both usually, so as long as we loop over the
   * unknowns and the x-direction internally and collapse those two loops,
   * we can actually use SIMD.
   *
   * It makes no sense to provide specialisation of this routine for
   * different enumerators, as the routine works per volume per unknown
   * anyway.
   *
   * @param patchIndex: Which patch is to be updated. QIn can hold multiple
   *   patches, and this index picks one of them.
   * @param volumeIndex: Which volume is to be updated within the target
   *   patch. This index refers to the target index.
   */
  template <class QInEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void copySolution(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                    QInEnumerator,
    int                                         patchIndex,
    const ::tarch::la::Vector<Dimensions, int>& volumeIndex,
    int                                         unknown,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod {
    QOut[QOutEnumerator(patchIndex, volumeIndex, unknown)] = QIn[QInEnumerator(patchIndex, volumeIndex, unknown)];
  };


  /**
   * Copy previous solution over into new time step and add source term
   *
   * This is the first step of the Rusanov scheme, where we basically copy
   * @f$ Q^{(old)} @f$ into @f$ Q^{(new)} @f$. In the same sweep, we also
   * add the source term subject to the time step size:
   *
   * @f$ Q^{(new)} = Q^{(old)} + \Delta t \cdot S(Q^{(old)}) @f$
   *
   * We reiterate that Q holds two types of entries: The actual data
   * subject to the PDE (and fluxes and source terms) and arbitrary
   * auxiliary variables per volume that the user has specified. Auxiliary
   * parameters are coupling terms, material parameters, helper
   * variables that you use to compute some metrics, ... They are not
   * evolved as part of the PDE. As the source term affects only the PDE
   * variables, we copy these
   * entries in Q over and add the source term contribution, while the
   * material parameters remain plain copies.
   *
   *
   * No SIMD declaration here, as we can't SIMDise over functors
   * anyway.
   *
   * @see exahype2::fv::rusanov::loopbodies::copySolution() for vectorisation details.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class QInEnumeratorType, class QOutEnumeratorType>
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


  /**
   * Specialisation of the other exahype2::fv::rusanov::loopbodies::copySolutionAndAddSourceTerm()
   * which exploits knowledge that we do not have to gather input data,
   * as the input data is delivered as AoS already.
   *
   * In line with the discussion in exahype2::fv::rusanov::loopbodies::copySolution, this operation
   * can vectorise properly if and only if you make your x-loop the
   * innermost one and if and only if your input is AoS - which is what
   * this specialisation is about.
   *
   * @see exahype2::fv::rusanov::loopbodies::copySolution() for vectorisation details.
   */
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


  /**
   * Specialisation invoking the static source variant via SolverType (for GPUs).
   */
#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void copySolutionAndAddSourceTerm(
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


  /**
   * Specialisation invoking the static source variant via SolverType (for GPUs).
   */
#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void copySolutionAndAddSourceTerm(
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


  /**
   * Compute maximum eigenvalue for one voxel in one direction.
   *
   * The user's maxEigenvalue functor expects the input data of one voxel
   * as one array. We cannot assume that the input data of the input is
   * available as unknown consecutive entries in the memory. So we first
   * have to gather the unknowns in one temporary vector, and then we pass
   * this vector into the user functor. The result is written back to
   * maxEigenvalue. Again, we don't know anything about its ordering, so
   * we use the enumerator to access to output array.
   *
   * The nice thing with the max (directional) eigenvalue is that the value
   * is a scalar. So we have to gather the input into one Q vector (see
   * specialisation below which eliminate this step) but then the outcome
   * is a scalar which we can write straight back. So this routine should
   * straightforwardly vectorise.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class QInEnumeratorType, class MaxEigenvalueEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeMaxEigenvalue(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const MaxEigenvalueFunctor&                    maxEigenvalueFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ maxEigenvalue,
    const MaxEigenvalueEnumeratorType& maxEigenvalueEnumerator
  ) InlineMethod;


  /**
   * Specialisation if input data is already in AoS format. This holds for
   * all of our kernels unless you explicitly reorder the input.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void computeMaxEigenvalue(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
    const MaxEigenvalueFunctor&                    maxEigenvalueFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ maxEigenvalue,
    const enumerator::AoSLexicographicEnumerator& maxEigenvalueEnumerator
  ) InlineMethod;


  /**
   * Specialisation invoking the static eigenvalue variant (for GPUs).
   */
#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class MaxEigenvalueEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeMaxEigenvalue(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ maxEigenvalue,
    const MaxEigenvalueEnumeratorType& maxEigenvalueEnumerator
  ) InlineMethod;


  /**
   * Specialisation for static eigenvalue call plus AoS input data.
   */
#pragma omp declare simd
  template <class SolverType, class MaxEigenvalueEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeMaxEigenvalue(
    const double* __restrict__ QIn,
    const enumerator::AoSLexicographicEnumerator&  QInEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ maxEigenvalue,
    const MaxEigenvalueEnumeratorType& maxEigenvalueEnumerator
  ) InlineMethod;


  /**
   * This is the actual eigenvalue loop which reduces the directional
   * eigenvalues.
   *
   * @param QOut The new (updated) solution over which we reduce the
   *   eigenvalues.
   *
   * @param QOutEnumerator Identificator how the output array is
   *   enumerated. Will be AoS most of the time, as this is Peano's
   *   default data ordering.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions double reduceMaxEigenvalue(
    const double* __restrict__ QOut,
    const QOutEnumeratorType&                      QOutEnumerator,
    const MaxEigenvalueFunctor&                    maxEigenvalueFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt
  ) InlineMethod;


  /**
   * Specialisation anticipating that the ordering of the output
   * data will be AoS in almost all the cases. So it makes sense
   * to exploit this knowledge performance-wisely.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions double reduceMaxEigenvalue(
    const double* __restrict__ QOut,
    const enumerator::AoSLexicographicEnumerator&  QOutEnumerator,
    const MaxEigenvalueFunctor&                    maxEigenvalueFunctor,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt
  ) InlineMethod;


  /**
   * Specialised version for GPU. See generic variant for documentation.
   */
#pragma omp declare simd
  template <class SolverType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double reduceMaxEigenvalue(
    const double* __restrict__ QOut,
    const QOutEnumeratorType&                      QOutEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt
  ) InlineMethod;


  /**
   * Optimised version for the GPU which is furthermore optimised
   * towards one specific unknown enumeration. This is the enumeration
   * we encounter in almost all of the cases, so it makes sense to
   * specialise for this one.
   */
#pragma omp declare simd
  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double reduceMaxEigenvalue(
    const double* __restrict__ QOut,
    const enumerator::AoSLexicographicEnumerator&  QOutEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    double                                         t,
    double                                         dt
  ) InlineMethod;


  /**
   * Compute the flux in one direction for a volume and store it.
   * Computing the fluxes requires us to gather the input data, invoke
   * the flux function and then to scatter the result again, as the
   * user signatures require an AoS data format whereas the loop body
   * itself is generic.
   */
#pragma omp declare simd
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class QInEnumeratorType, class FluxEnumeratorType>
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


  /**
   * Specialisation invoking the static flux variant via SolverType (for GPUs).
   */
#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class FluxEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeFlux(
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeFlux(
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
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeFlux(
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


  /**
   * Update one volume with the flux contribution
   *
   * For each dimension: Take the flux left and in the actual volume and the one
   * to the right and combine them.
   *
   * @todo Remove flux in the centre, as it is eliminated.
   */
#pragma omp declare simd
  template <class FluxEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithFlux(
    const double* __restrict__ tempFluxX,
    const double* __restrict__ tempFluxY,
    const double* __restrict__ tempFluxZ,
    const FluxEnumeratorType&                      fluxEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    int                                            unknown,
    double                                         dt,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


  /**
   * Constructs the average of the flux and adds it to the left and right
   * neighbour cell with inverted sign. This is the counterpart of the other
   * exahype2::fv::rusanov::loopbodies::updateSolutionWithFlux() routine which works cell-wisely.
   *
   * We always enumerate right looking, i.e., the right face of cell i is
   * face i. The leftmost cell consequently has no left face (it is cell no
   * 0 and 0 is its right face). For N cells in a row, there's a face N-1
   * but no face N anymore.
   */
#pragma omp declare simd
  template <class FluxEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithFlux(
    const double* __restrict__ fluxL,
    const double* __restrict__ fluxR,
    const FluxEnumeratorType&                      fluxEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    int                                            unknown,
    double                                         dt,
    int                                            normal,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


  /**
   * Uses the eigenvalues to damp the solution update.
   *
   * ## Rationale for alternative realisations
   *
   * I originally thought the following:
   *
   * We could damp along the individual coordinate axis (normal). That is, if normal
   * equals zero, we take the left and right neighbour of a cell into account,
   * if it equals one, we look at the lower and upper face-connected neighbour.
   *
   * To allow the compiler to vectorise over this routine, it is essential that
   * you make the loop over normal the outer loop, and then you can collapse
   * and vectorise over the patch. Yet, even if you do so, there is no guarantee
   * that the compiler will manage to vectorise. It really depends upon the
   * QOutEnumerator, i.e., the ordering of the image. If the image is stored as
   * AoS (default ordering for output data), then the loops over this routine
   * cannot vectorise properly. To make a long story short: This routine will
   * vectorise if and only if you explicitly re-sort input and output values of
   * a patch prior to the update such that they are in SoA format. Obviously,
   * it is up to you to decide if the overhead for this re-sorting is worth it.
   *
   * Anyway, such a damping is a fundamental rewrite, as we then have to pick
   * either tempMaxEigenvalueX, Y or Z, so we might end up with three routines.
   *
   * Along the same lines, all of these arguments motivate why we do not specialise
   * the template further. We need to do this scattering/gathering anyway, so
   * what's the point of specialising for particular data formats?
   *
   */
#pragma omp declare simd
  template <class QInEnumeratorType, class MaxEigenvalueEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithEigenvalueDamping(
    const double* __restrict__ QIn,
    const QInEnumeratorType& QInEnumerator,
    const double* __restrict__ tempMaxEigenvalueX,
    const double* __restrict__ tempMaxEigenvalueY,
    const double* __restrict__ tempMaxEigenvalueZ,
    const MaxEigenvalueEnumeratorType&             eigenvalueEnumerator,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    int                                            unknown,
    double                                         dt,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


  /**
   * Face-wise version of routine, i.e., it takes the eigenvalues and
   * updates the left and right adjacent volume of it.
   */
#pragma omp declare simd
  template <class QInEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithEigenvalueDamping(
    const double* __restrict__ QIn,
    const QInEnumeratorType&                       QInEnumerator,
    double                                         maxEigenvalueL,
    double                                         maxEigenvalueR,
    const ::tarch::la::Vector<Dimensions, double>& patchCentre,
    const ::tarch::la::Vector<Dimensions, double>& patchSize,
    int                                            patchIndex,
    const ::tarch::la::Vector<Dimensions, int>&    volumeIndex,
    int                                            unknown,
    double                                         dt,
    int                                            normal,
    double* __restrict__ QOut,
    const QOutEnumeratorType& QOutEnumerator
  ) InlineMethod;


  /**
   * Compute non-conservative flux over one face along normal normal
   *
   * We count faces starting with zero, i.e., effectively shift the indices
   * not by one to the right: Let the leftmost volume have
   * the index i. i is typically 0 or -1 (if we have halos).
   *
   * If we call this routine for the index i, we first take the volume
   * i and then we average it with i+1. So we look to the right. The
   * result is stored in i. That means, all the indices are kind of
   * shifted by 1/2 to the left. Index (4,3,2) is actually the face
   * located at (4.5, 3, 2).
   *
   * See updateSolutionWithNonconservativeFlux() for some fancy ASCII art
   * illustrating our enumeration rules. As this routine is used over faces,
   * we work with slightly squeued indices. Let us work with a patch size of
   * 5. That means, we have (logically) an array of [-1,5]x[-1,5]x[-1,5] for
   * the patch. The diagonal values such as (-1,-1,4) are not initialised
   * properly. For the ncp along the x-direction, we have to befill an array
   * of the dimensions [-1,4]x[0,4]x[0,4]. This is the range we have to
   * invoke this routine for.
   */
  template <int NumberOfUnknowns, int NumberOfAuxiliaryVariables, class QInEnumeratorType, class NCPFaceEnumeratorType>
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


  /**
   * Specialisation invoking the static NCP variant via SolverType (for GPUs).
   */
#pragma omp declare simd
  template <class SolverType, class QInEnumeratorType, class NCPFaceEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void computeNonconservativeFlux(
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


  /**
   * Add the non-conservative flux contributions to one volume.
   *
   * The NCP is defined over faces, i.e., we know that ncpX, ncpY, ncpZ
   * hold one (vector) entry per face and ncpEnumerator is aware that we
   * work with faces. So we compute the left and the right face and we get
   * the corresponding non-conservative flux from there. This flux is now
   * scaled with the time step size, as we do an explicit Euler step, i.e.,
   * the right-hand side of the PDE
   *
   * @f$ \partial _t Q = ncp(Q) @f$
   *
   * is discretised as @f$ \partial _t Q \approx \frac{Q^{(new)}-Q^{(old)}}{dt}@f$
   * which gives us a scaling of dt for the update. We also have a volume
   * integral on the left-hand side and a face integral on the right-hand
   * side, and therefore a further scaling by one over the volume size.
   *
   * ## Enumeration
   *
   * We enumerate all data lexicographically, although the ncpEnumerator might
   * remap and permute this ordering. The routine updates a given volume
   * identified by volumeIndex with the fluxes over all d dimensions. So we
   * need the flux over the left face, right face, bottom face, top face, ...
   *
   * The flux over the left face is stored in the x-flux array with exactly
   * the same x index as the volume. For the right face, we have to increase
   * this index by one along the coordinate axis. This enumeration is
   * encoded within computeNonconservativeFlux(), too.
   *
   * Here's some ASCII art illustrating the enumeration. If we update volume
   * (2,1) along the x-axis
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * -------------------------
   * | 0,3 | 1,3 | 2,3 | 3,3 |
   * -------------------------
   * | 0,2 | 1,2 | 2,2 | 3,2 |
   * -------------------------
   * | 0,1 | 1,1 | 2,1 | 3,1 |
   * -------------------------
   * | 0,0 | 1,0 | 2,0 | 3,0 |
   * -------------------------
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * then the ncp values that matter are stored as follows:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * -------------------------------
   * | -1,3 | 0,3 | 1,3 | 2,3 | 3,3 |
   * -------------------------------
   * | -1,2 | 0,2 | 1,2 | 2,2 | 3,2 |
   * -------------------------------
   * | -1,1 | 0,1 | 1,1 | 2,1 | 3,1 |
   * -------------------------------
   * | -1,0 | 0,0 | 1,0 | 2,0 | 3,0 |
   * -------------------------------
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * where (0,1) for example holds the flux over the right face of volume
   * (0,1) in the upper block. So index (0,1) in ncpX holds the ncp between
   * volume (0,1) and (1,1).
   */
#pragma omp declare simd
  template <class NCPFaceEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithNonconservativeFlux(
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


  /**
   * Face-oriented flavour of NCP
   *
   * Update a volume due to the NCP, but only the flux along one direction,
   * i.e. along normal. This is a specialisation of the other
   * updateSolutionWithNonconservativeFlux() routine. Please consult this one
   * for more in-depth explanations.
   */
#pragma omp declare simd
  template <class NCPFaceEnumeratorType, class QOutEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void updateSolutionWithNonconservativeFlux(
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
} // namespace exahype2::fv::rusanov::loopbodies

#include "LoopBodies.cpph"
