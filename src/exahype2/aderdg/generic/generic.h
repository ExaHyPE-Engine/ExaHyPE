// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <vector>

#include "exahype2/aderdg/ADERDGSolver.h"
#include "exahype2/aderdg/basis/GaussLegendreBasis.h"
#include "exahype2/aderdg/basis/GaussLobattoBasis.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"

#define MbasisSize 4
#define Mvar 5
#define Mdim 3
#define f2p5(var, dim, i, j, k) (var + Mvar * dim + Mvar * Mdim * i + Mvar * Mdim * MbasisSize * j + Mvar * Mdim * MbasisSize * MbasisSize * k)
#define p2f5(var, dim, i, j, k) (dim * MbasisSize * MbasisSize * MbasisSize * Mvar + Mvar * i + Mvar * MbasisSize * j + Mvar * MbasisSize * MbasisSize * k + var)

#define Mface 6
#define f2p4(var, face, a, b) (var + Mvar * face + Mvar * Mface * a + Mvar * Mface * MbasisSize * b)
#define p2f4(var, face, a, b) (face * MbasisSize * MbasisSize * Mvar + Mvar * a + Mvar * MbasisSize * b + var)

// todo Dominic Etienne Charrier
// Possibly redundant definition of face indices
// see exahype/solvers/Solver.h
// On the other hand, the kernels should be
// more or less independent of ExaHyPE/exahype.
#define EXAHYPE_FACE_LEFT 0
#define EXAHYPE_FACE_RIGHT 1
#define EXAHYPE_FACE_FRONT 2
#define EXAHYPE_FACE_BACK 3
#define EXAHYPE_FACE_BOTTOM 4
#define EXAHYPE_FACE_TOP 5

namespace exahype2::aderdg::generic {
  template <
    bool EvaluatePointSource,
    bool EvaluateAlgebraicSource,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateMaterialParameters,
    class SolverType,
    class pCompType,
    class pStoreType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void spaceTimePredictorLinear(
    SolverType&                                  solver,
    pStoreType*                                  lQbnd,
    pStoreType*                                  lFbnd,
    pCompType*                                   lQi,
    pCompType*                                   lFi,
    pCompType*                                   gradQ,
    pCompType*                                   PSi,
    pCompType*                                   PSderivatives,
    pCompType*                                   tmp_PSderivatives,
    pCompType*                                   lQhi,
    pCompType*                                   lFhi,
    const double* const                          luh,
    const tarch::la::Vector<Dimensions, double>& cellCenter,
    const tarch::la::Vector<Dimensions, double>& invDx,
    const double                                 t,
    const double                                 dt
  ) InlineMethod;

  template <bool useSource, bool useFlux, bool useNCP, bool noTimeAveraging, class SolverType, class pCompType, class pStoreType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions int spaceTimePredictorNonlinear(
    SolverType&                                  solver,
    pStoreType*                                  lQhbnd,
    pStoreType*                                  lGradQhbnd,
    pStoreType*                                  lFhbnd,
    pCompType*                                   lQi,
    pCompType*                                   rhs,
    pCompType*                                   lFi,
    pCompType*                                   gradQ,
    pCompType*                                   lQhi,
    pCompType*                                   lFhi,
    const double* const                          luh,
    const tarch::la::Vector<Dimensions, double>& cellCenter,
    const tarch::la::Vector<Dimensions, double>& invDx,
    double                                       t,
    const double                                 dt
  ) InlineMethod;

  template <class SolverType, class pType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void solutionUpdate(SolverType& solver, double* luh, const double* const luhOld, const pType* const lduh, const double dt)
    InlineMethod;

  template <class SolverType, class pCompType, bool useSourceOrNCP, bool useFlux, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void volumeIntegralLinear(pCompType* lduh, const pCompType* const lFhi, const tarch::la::Vector<Dimensions, double>& dx)
    InlineMethod;

  template <class SolverType, class pCompType, bool useSourceOrNCP, bool useFlux, bool noTimeAveraging, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void volumeIntegralNonlinear(pCompType* lduh, const pCompType* const lFi, const tarch::la::Vector<Dimensions, double>& dx)
    InlineMethod;

  template <class SolverType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void faceIntegralNonlinear(
    double* lduh, const double* const lFhbnd, const int direction, const int orientation, const tarch::la::Vector<Dimensions, double>& dx
  ) InlineMethod;

  template <class SolverType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void faceIntegralLinear(
    double* lduh, const double* const lFhbnd, const int direction, const int orientation, const tarch::la::Vector<Dimensions, double>& dx
  ) InlineMethod;

  // todo 10/02/16: Dominic
  // Keep only one surfaceIntegral.
  template <class SolverType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void surfaceIntegralNonlinear(double* lduh, const double* const lFbnd, const tarch::la::Vector<Dimensions, double>& dx)
    InlineMethod;

  template <class SolverType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void surfaceIntegralLinear(double* lduh, const double* const lFbnd, const tarch::la::Vector<Dimensions, double>& dx)
    InlineMethod;

  /*void surfaceIntegral2(
      double* lduh,
      const double* const lFhbnd,
      const tarch::la::Vector<Dimensions,double>&  dx,
      const int numberOfVariables,
      const int basisSize
  );*/

  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void solutionAdjustment(
    SolverType& solver, double* luh, const tarch::la::Vector<Dimensions, double>& center, const tarch::la::Vector<Dimensions, double>& dx, const double t, const double dt
  ) InlineMethod;

  // @todo Dominic Etienne Charrier
  // Inconsistent ordering of inout and in arguments
  // template argument functions and non-template argument function.
  /**
   * Implements a Rusanov Riemann solver.
   *
   * @param solver
   * @param FL
   * @param FR
   * @param QL
   * @param QR
   * @param t
   * @param dt
   * @param faceCentre
   * @param dx
   * @param direction
   */
  template <bool useFlux, bool useNCP, bool useMM, typename SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void riemannSolverLinear(
    SolverType&                                  solver,
    double*                                      FL,
    double*                                      FR,
    const double* const                          QL,
    const double* const                          QR,
    const double                                 t,
    const double                                 dt,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& dx,
    const int                                    direction
  ) InlineMethod;

  template <bool useNCP, class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void riemannSolverNonlinear(
    SolverType&                                  solver,
    double*                                      FL,
    double*                                      FR,
    const double* const                          QL,
    const double* const                          QR,
    const double                                 t,
    const double                                 dt,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& dx,
    const int                                    direction
  ) InlineMethod;

  /**
   * Implements a generalised osher type flux.
   *
   * @note Requires @p solver to implement a nonconservative product and an eigenvectors function which returns
   * the eigenvalues and eigenvectors. The kernel supplies the solver with reference coordinate indices.
   *
   * References:
   *
   * [1] M. Dumbser and E. F. Toro, “On Universal Osher-Type Schemes for General Nonlinear Hyperbolic Conservation
   * Laws,” Communications in Computational Physics, vol. 10, no. 03, pp. 635–671, Sep. 2011. [2] M. Dumbser and
   * E. F. Toro, “A Simple Extension of the Osher Riemann Solver to Non-conservative Hyperbolic Systems,” Journal
   * of Scientific Computing, vol. 48, no. 1–3, pp. 70–88, Jul. 2011.
   *
   * @note Currently, no viscous flux is supported.
   *
   * @tparam numQuadPoints the number of quadrature points the Legendre quadrature should use. 3 is chosen in
   * paper [1].
   *
   * @param solver    solver implementing an eigenvectors function (plus a nonconservative product) if required.
   * @param FL        "left"/"-" normal flux of size [0,nVar]^2.
   * @param FR        "right"/"+"normal flux of size [0,nVar]^2.
   * @param QL        "left"/"-" state variables (plus parameters); range: [0,nVar+nPar]^2.
   * @param QR        "right"/"+"state variables (plus parameters); range: [0,nVar+nPar]^2.
   * @param t         time stamp
   * @param dt        time step size
   * @param direction normal direction
   */
  template <bool useFlux, bool useNCP, class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void generalisedOsherSolomon(
    SolverType& solver, double* const FL, double* const FR, const double* const QL, const double* const QR, const double t, const double dt, const int direction
  ) InlineMethod;

  template <bool useGradientFlux, class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void boundaryConditions(
    SolverType&                                  solver,
    double*                                      fluxOut,
    double*                                      stateOut,
    const double* const                          fluxIn,
    const double* const                          stateIn,
    const double* const                          gradStateIn,
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize,
    const double                                 t,
    const double                                 dt,
    const int                                    faceIndex,
    const int                                    direction
  ) InlineMethod;

  template <class SolverType, class T>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions double maxScaledEigenvalue(
    SolverType& solver, const T* const luh, const tarch::la::Vector<Dimensions, double>& cellCentre, const tarch::la::Vector<Dimensions, double>& dx, const double t, const double dt
  ) InlineMethod;

  /**
   * \note We need to consider material parameters in
   * lQhbndFine and lQhbndCoarse.
   */

  template <class SolverType, class pStoreType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void singleLevelFaceUnknownsProlongation(
    pStoreType* lQhbndFine, const pStoreType* lQhbndCoarse, const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
  ) InlineMethod;

  template <class SolverType, class pStoreType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void singleLevelFaceUnknownsRestriction(
    pStoreType* lQhbndCoarse, const pStoreType* lQhbndFine, const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
  ) InlineMethod;

  template <class SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void faceUnknownsProlongation(
    double*                                       lQhbndFine,
    double*                                       lFhbndFine,
    const double*                                 lQhbndCoarse,
    const double*                                 lFhbndCoarse,
    const int                                     coarseGridLevel,
    const int                                     fineGridLevel,
    const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
  ) InlineMethod;

  template <class SolverType, int numberOfVariables, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void faceUnknownsRestriction(
    double* const lFhbndCoarse, const double* const lFhbndFine, const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex, const int levelDelta
  ) InlineMethod;

  /**
   * \note We need to consider material parameters in
   * lQhbndFine and lQhbndCoarse.
   *
   * @\deprecated
   */
  template <class SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void faceUnknownsRestriction(
    double*                                       lQhbndCoarse,
    double*                                       lFhbndCoarse,
    const double*                                 lQhbndFine,
    const double*                                 lFhbndFine,
    const int                                     coarseGridLevel,
    const int                                     fineGridLevel,
    const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
  ) InlineMethod;

  /**
   * \note We need to consider material parameters in
   * luhCoarse and luhFine.
   */
  template <class SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void volumeUnknownsProlongation(
    double* luhFine, const double* luhCoarse, const int coarseGridLevel, const int fineGridLevel, const tarch::la::Vector<Dimensions, int>& subcellIndex
  ) InlineMethod;

  /**
   * \note We need to consider material parameters in
   * luhCoarse and luhFine.
   */
  template <class SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void volumeUnknownsRestriction(
    double* luhCoarse, const double* luhFine, const int coarseGridLevel, const int fineGridLevel, const tarch::la::Vector<Dimensions, int>& subcellIndex
  ) InlineMethod;

  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions std::vector<int>* getPointSources(
    SolverType& solver, const tarch::la::Vector<Dimensions, double>& center, const tarch::la::Vector<Dimensions, double>& dx
  ) InlineMethod;

  template <class SolverType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void deltaDistribution(
    SolverType&                                  solver,
    const double* const                          luh,
    const double                                 t,
    const double                                 dt,
    const tarch::la::Vector<Dimensions, double>& center,
    const tarch::la::Vector<Dimensions, double>& dx,
    std::vector<int>*                            pointSources, // will be deleted in the end
    double*                                      PSi
  ) InlineMethod;
} // namespace exahype2::aderdg::generic

#include "GeneralisedOsherSolomon.cpph"

#if Dimensions == 2
/*
#include "2d/AMRRoutines.cpph"
#include "2d/BoundaryConditions.cpph"
#include "2d/DeltaDistribution.cpph"
#include "2d/FaceIntegralLinear.cpph"
#include "2d/FaceIntegralNonlinear.cpph"
#include "2d/MaxScaledEigenvalue.cpph"
#include "2d/RiemannSolverLinear.cpph"
#include "2d/RiemannSolverNonlinear.cpph"
#include "2d/SolutionAdjustment.cpph"
#include "2d/SolutionUpdate.cpph"
#include "2d/SpaceTimePredictorLinear.cpph"
#include "2d/SpaceTimePredictorNonlinear.cpph"
#include "2d/SurfaceIntegralLinear.cpph"
#include "2d/SurfaceIntegralNonlinear.cpph"
#include "2d/VolumeIntegralLinear.cpph"
#include "2d/VolumeIntegralNonlinear.cpph"
*/
#elif Dimensions == 3
#include "3d/AMRRoutines.cpph"
// #include "3d/BoundaryConditions.cpph"
// #include "3d/DeltaDistribution.cpph"
#include "3d/FaceIntegralLinear.cpph"
// #include "3d/FaceIntegralNonlinear.cpph"
#include "3d/MaxScaledEigenvalue.cpph"
#include "3d/RiemannSolverLinear.cpph"
// #include "3d/RiemannSolverNonlinear.cpph"
// #include "3d/SolutionAdjustment.cpph"
#include "3d/SolutionUpdate.cpph"
#include "3d/SpaceTimePredictorLinear.cpph"
// #include "3d/SpaceTimePredictorNonlinear.cpph"
// #include "3d/SurfaceIntegralLinear.cpph"
// #include "3d/SurfaceIntegralNonlinear.cpph"
#include "3d/VolumeIntegralLinear.cpph"
// #include "3d/VolumeIntegralNonlinear.cpph"
#endif
