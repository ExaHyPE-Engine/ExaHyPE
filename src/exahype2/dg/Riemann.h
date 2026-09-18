// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/FaceMarker.h"
#include "tarch/la/Vector.h"

/**
 * @page exahype2_dg_DG_AMR Discontinuous Galerkin adaptive mesh refinement
 *
 * The routines below realise the steps of adaptive mesh refinement for a
 * discontinuous Galerkin discretisation. The Python file
 * exahype2.solvers.rkdg.actionsets.DynamicAMR calls them and decides when each
 * step runs.
 *
 * AMR for DG is a Riemann problem and can be illustrated in 1D.
 * Let there be two levels called coarse and fine as illustrated below:
 *
 * @image html src/exahype2/dg/adaptive-mesh-refinement.png
 *
 * Every face holds the polynomial value left and right of it. So the green
 * face (it is a vertex here as we illustrate things in 1D, but it is really
 * a face) holds the left solution after each initial step of the Runge Kutta
 * scheme. A face is unique through its position and its level. So there are
 * two faces at the same location: The green one and the red one.
 *
 * ## Terminology
 *
 * - A cell is refined if one of its adjacent vertices holds a refinement
 *   flag. This is called or-based refinement.
 * - A face therefore is called a refined face if one of its adjacent
 *   vertices holds the refinement flag.
 * - A hanging vertex is a vertex with less than 2^d adjacent cells on the
 *   same level. All other vertices are persistent.
 * - A hanging face is a face where no vertex is persistent, i.e. all
 *   adjacent vertices are hanging.
 * - Non-persistent data is not held between two grid traversals.
 *
 * Every side of the transition solves the Riemann problems it integrates at
 * its own resolution:
 *
 * - The fine cells solve the Riemann problems on their hanging faces. The
 *   fine trace is their own projection; the coarse trace is the coarse
 *   face's projection interpolated down. Each fine trace therefore receives
 *   the full interface dissipation of the Riemann solver.
 * - The coarse cell solves the Riemann problem on the coarse face, whose
 *   far half holds the restriction of the fine traces.
 *
 * Hanging faces hold no data between grid sweeps, so the fine-side Riemann
 * problems have to be solved in the sweep in which the fine cells project
 * their traces, and their face integral is accumulated into the persistent
 * right-hand-side estimates right away.
 *
 * ## Algorithm
 *
 * - In the primary grid sweep where we project onto the face and solve the
 *   volume integral:
 *   - Clear the solution on every persistent face, so there are zeroes in
 *     there.
 *   - Project the linear combination onto the faces (action set
 *     ProjectLinearCombinationOfEstimates). This includes hanging faces.
 *   - On every hanging face, interpolate the coarse face's near half onto
 *     the half the fine cells cannot supply
 *     (interpolateProjectedFacePolynomial()).
 *   - Every fine cell with hanging faces solves the Riemann problems on
 *     these faces and accumulates their face integral into its
 *     right-hand-side estimates (action set
 *     SolveRiemannProblemOnHangingFaces).
 *   - When a hanging face is left, stage its projected fine trace; the
 *     persistent coarse face reduces the staged subfaces at its last touch
 *     (exahype2::amr::restrictHaloLayerFromStagedSubfaces()).
 * - In the secondary grid sweep where we solve the Riemann problems and
 *   bring the solutions together:
 *   - Solve the Riemann problem on the persistent faces (action set
 *     SolveRiemannProblem).
 *   - Clear the Riemann solution on every hanging face, so the fine cells'
 *     face integrals add nothing for them: Their contribution is already in
 *     the right-hand-side estimates.
 */

namespace exahype2::dg {
  void interpolateProjectedFacePolynomial(
    const peano4::dataview::FaceMarker& marker,
    int                                 order,
    int                                 numberOfProjectedQuantities,
    int                                 unknowns,
    int                                 auxiliaryVariables,
    const double* NOALIAS               interpolationMatrix1D,
    const double* NOALIAS               coarseGridFaceQ,
    double* NOALIAS                     fineGridFaceQ
  );

  void interpolateRiemannSolution(
    int                                       faceNumber,
    const tarch::la::Vector<DIMENSIONS, int>& positionWithinFatherCell,
    int                                       order,
    int                                       unknowns,
    int                                       auxiliaryVariables,
    double                                    maxEigenvalue,
    const double* NOALIAS                     interpolationMatrix1D,
    const double* NOALIAS                     coarseGridFaceRiemannSolution,
    const double* NOALIAS                     coarseGridFaceProjection,
    const double* NOALIAS                     fineGridFaceProjection,
    double* NOALIAS                           fineGridFaceRiemannSolution
  );

  void projectVolumetricDataOntoFaces(
    const double* NOALIAS cellQ,
    int                   order,
    int                   unknowns,
    int                   auxiliaryVariables,
    const double* NOALIAS basisFunctionValuesLeft,
    double* NOALIAS       faceQLeft,
    double* NOALIAS       faceQRight,
    double* NOALIAS       faceQBottom,
    double* NOALIAS       faceQUp
  );

  void clearSolutionProjection(
    int             order,
    int             unknowns,
    int             auxiliaryVariables,
    int             numberOfProjectedQuantities,
    double* NOALIAS faceQ
  );

  void projectVolumetricDataOntoFaces(
    const double* NOALIAS cellQ,
    int                   order,
    int                   unknowns,
    int                   auxiliaryVariables,
    const double* NOALIAS basisFunctionValuesLeft,
    double* NOALIAS       faceQLeft,
    double* NOALIAS       faceQRight,
    double* NOALIAS       faceQBottom,
    double* NOALIAS       faceQUp,
    double* NOALIAS       faceQFront,
    double* NOALIAS       faceQBack
  );

  void projectVolumetricDataAndGradientOntoFaces(
    const double* NOALIAS cellQ,
    int                   order,
    int                   unknowns,
    int                   auxiliaryVariables,
    const double* NOALIAS basisFunctionValuesLeft,
    double* NOALIAS       faceQLeft,
    double* NOALIAS       faceQRight,
    double* NOALIAS       faceQBottom,
    double* NOALIAS       faceQUp
  );

  void projectVolumetricDataAndGradientOntoFaces(
    const double* NOALIAS cellQ,
    int                   order,
    int                   unknowns,
    int                   auxiliaryVariables,
    const double* NOALIAS basisFunctionValuesLeft,
    double* NOALIAS       faceQLeft,
    double* NOALIAS       faceQRight,
    double* NOALIAS       faceQBottom,
    double* NOALIAS       faceQUp,
    double* NOALIAS       faceQFront,
    double* NOALIAS       faceQBack
  );

  void integrateOverRiemannSolutionsAndAddToVolumeGaussLegendre(
    const double* NOALIAS               faceQLeft,
    const double* NOALIAS               faceQRight,
    const double* NOALIAS               faceQBottom,
    const double* NOALIAS               faceQUp,
    int                                 order,
    int                                 unknowns,
    int                                 auxiliaryVariables,
    const tarch::la::Vector<2, double>& cellSize,
    const double* NOALIAS               basisFunctionValuesLeft,
    const double* NOALIAS               massMatrixDiagonal1D,
    double* NOALIAS                     cellQ
  );

  void integrateOverRiemannSolutionsAndAddToVolumeGaussLegendre(
    const double* NOALIAS               faceQLeft,
    const double* NOALIAS               faceQRight,
    const double* NOALIAS               faceQBottom,
    const double* NOALIAS               faceQUp,
    const double* NOALIAS               faceQFront,
    const double* NOALIAS               faceQBack,
    int                                 order,
    int                                 unknowns,
    int                                 auxiliaryVariables,
    const tarch::la::Vector<3, double>& cellSize,
    const double* NOALIAS               basisFunctionValuesLeft,
    const double* NOALIAS               massMatrixDiagonal1D,
    double* NOALIAS                     cellQ
  );
} // namespace exahype2::dg
