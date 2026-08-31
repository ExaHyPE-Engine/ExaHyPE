// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "tarch/la/Vector.h"

/**
 * @page exahype2_dg_DG_AMR Discontinuous Galerkin adaptive mesh refinement
 *
 * The AMR management is distributed among two files: The present header file
 * hosts a couple of routines which realise the steps required for AMR. We
 * write what and how things are done in C++. The routines pair up with the
 * Python file exahype2.solvers.rkdg.actionsets.DynamicAMR. This Python file
 * orchestrates the AMR operations, i.e. it decides when things are done. Most
 * of the documentation for the whole AMR business is however collected here.
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
 *     (toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces()).
 * - In the secondary grid sweep where we solve the Riemann problems and
 *   bring the solutions together:
 *   - Solve the Riemann problem on the persistent faces (action set
 *     SolveRiemannProblem).
 *   - Clear the Riemann solution on every hanging face, so the fine cells'
 *     face integrals add nothing for them: Their contribution is already in
 *     the right-hand-side estimates.
 */

namespace exahype2::dg {
  /**
   * ## Interpolation matrix
   *
   * Core ingredient to this routine is the interpolation matrix.
   * It describes how to interpolate a 1D polynomial over the unit
   * interval into the three subcell polynomials. Consequently,
   * the matrix points to three matrices. One for the left, one
   * for the middle, one for the right subcell.
   *
   * Each matrix accepts the order+1 values at the quadrature points
   * as input, i.e. each matrix has order+1 columns. Each matrix
   * returns the output for one quadrature point at the chosen
   * subcell. So each matrix has order+1 rows.
   *
   * In the example above, we work with polynomials of order three.
   * We hence have four quadrature points.
   *
   * @image html src/exahype2/dg/interpolation.png
   *
   * In the example above, the weights in c1, c2, c3 and c4 span a
   * polynomial (blue). InterpolationMatrix1d points to
   * @f$ 3 \cdot 4^2 @f$ entries. If we want to know the interpolated
   * value for f31, we have to pick the third matrix (matrix index 2
   * as we start counting at zero), then take the first row of this
   * matrix, and multiply its four entries with c1, c2, c3, c4.
   *
   * All entries in all faces are enumerated lexicographically, i.e.
   * from left to right, bottom-up, and always first along the x-axis,
   * then y, then z.
   *
   * The routine writes only the half of the hanging face that belongs to
   * the coarse neighbour; the other half carries the fine cell's own
   * projection and stays untouched.
   *
   * @param numberOfProjectedQuantities This is one if we only project the
   *   solution. It is two if we project the solution and the F-value or
   *   the solution and its gradient along the normal.
   */
  void interpolateProjectedFacePolynomial(
    const peano4::datamanagement::FaceMarker& marker,
    int                                       order,
    int                                       numberOfProjectedQuantities,
    int                                       unknowns,
    int                                       auxiliaryVariables,
    const double* NOALIAS                     InterpolationMatrix1d,
    const double* NOALIAS                     coarseGridFaceQ,
    double* NOALIAS                           fineGridFaceQ
  );

  /**
   * Assembles the Riemann solution a fine cell consumes at a hanging face.
   *
   * The routine evaluates the coarse flux polynomial at the hanging face's
   * nodes, adds the jump penalty
   * 0.5 maxEigenvalue (restriction image - fine trace) with the face's
   * orientation and writes the sum into the fine side's layer of the hanging
   * face's Riemann solution. The coarse side's layer stays untouched, and
   * the Riemann solution holds the plain flux without auxiliary variables.
   *
   * The caller solves the coarse face's Riemann problem before it touches an
   * adjacent fine cell in the same sweep, and passes one maxEigenvalue for
   * the whole coarse face. The caller runs in a cell event, so the routine
   * identifies the hanging face through the face number and the cell's
   * position within its father cell.
   */
  void interpolateRiemannSolution(
    int                                       faceNumber,
    const tarch::la::Vector<DIMENSIONS, int>& positionWithinFatherCell,
    int                                       order,
    int                                       unknowns,
    int                                       auxiliaryVariables,
    double                                    maxEigenvalue,
    const double* NOALIAS                     InterpolationMatrix1d,
    const double* NOALIAS                     coarseGridFaceRiemannSolution,
    const double* NOALIAS                     coarseGridFaceProjection,
    const double* NOALIAS                     fineGridFaceProjection,
    double* NOALIAS                           fineGridFaceRiemannSolution
  );

  /**
   * Take polynomial within cell and project it onto the faces
   *
   * ## Ordering
   *
   *                           2 3
   *                           0 1
   *                     2 3 [ o o ] 2 3
   *                     0 1 [ o o ] 0 1
   *                           2 3
   *                           0 1
   *
   * Each individual face is ordered lexicographically. So we go from left to
   * right, and we always start closest to the origin of the coordinate system
   * if we interpret the cell as the unit square.
   *
   * The sketch above illustrates the ordering of the four faces for a p=1
   * discretisation where we have two dofs per direction per face per side.
   *
   * @param cellQ Pointer to cell data. The array has the size
   *   @f$ unknowns \times (order+1)^d@f$.
   *
   * @param order Integer greater or equal to 0. The number of nodes in each
   *   dimension is equal to order+1.
   *
   * @param BasisFunctionValuesLeft Array of size order+1 which clarifies (in 1D)
   *   how to project the order+1 quadrature weights onto a value on the left
   *   face. For Gauss Lobatto, this is a trivial array (you can read out the
   *   value directly from the leftmost quadrature point), but for any other
   *   basis, you need a linear combination of all the quadrature points in the
   *   cell.
   *
   * @param faceQLeft Left unknowns. This array hosts @f$ 2 \times (order+1)^(d-1) * unknowns@f$
   *   values, as it holds the (projected) values from the left side of the face
   *   and those from the right. The ordering of these values is lexicographic,
   *   i.e. the first unknowns entries are those at the bottom of the vertical
   *   face from the left adjacent cell. The next unknowns entries are those
   *   at the bottom of the face from the right adjacent cell. The next are then
   *   again left, then right, then left and so on.
   *   This function will fill the right values since the current cell is to the
   *   right of its own left face.
   *
   * @param faceQBottom Bottom unknowns. See faceQLeft. Again, we order
   *   lexicographically. This time, the first half of the entries are those
   *   from the adjacent cell below. The remaining entries are the projections
   *   from above.
   */
  void projectVolumetricDataOntoFaces(
    const double* NOALIAS       cellQ,
    int                         order,
    int                         unknowns,
    int                         auxiliaryVariables,
    const double* const NOALIAS BasisFunctionValuesLeft,
    double* const NOALIAS       faceQLeft,
    double* const NOALIAS       faceQRight,
    double* const NOALIAS       faceQBottom,
    double* const NOALIAS       faceQUp
  );

  /**
   * Set the whole solution projection on the face from left and right to zero
   *
   * @param numberOfProjectedQuantities Equals one if we project only the
   *   solution to the face, it equals two if we project both the solution
   *   and the gradient or the F-extrapolation, and so forth.
   */
  void clearSolutionProjection(
    int             order,
    int             unknowns,
    int             auxiliaryVariables,
    int             numberOfProjectedQuantities,
    double* NOALIAS faceQ
  );

  /**
   * 3D counterpart to other projectVolumetricDataOntoFaces() variant.
   */
  void projectVolumetricDataOntoFaces(
    const double* NOALIAS       cellQ,
    int                         order,
    int                         unknowns,
    int                         auxiliaryVariables,
    const double* const NOALIAS BasisFunctionValuesLeft,
    double* const NOALIAS       faceQLeft,
    double* const NOALIAS       faceQRight,
    double* const NOALIAS       faceQBottom,
    double* const NOALIAS       faceQUp,
    double* const NOALIAS       faceQFront,
    double* const NOALIAS       faceQBack
  );

  void projectVolumetricDataAndGradientOntoFaces(
    const double* NOALIAS       cellQ,
    int                         order,
    int                         unknowns,
    int                         auxiliaryVariables,
    const double* const NOALIAS BasisFunctionValuesLeft,
    double* const NOALIAS       faceQLeft,
    double* const NOALIAS       faceQRight,
    double* const NOALIAS       faceQBottom,
    double* const NOALIAS       faceQUp
  );

  /**
   * 3D counterpart to other projectVolumetricDataOntoFaces() variant.
   */
  void projectVolumetricDataAndGradientOntoFaces(
    const double* NOALIAS       cellQ,
    int                         order,
    int                         unknowns,
    int                         auxiliaryVariables,
    const double* const NOALIAS BasisFunctionValuesLeft,
    double* const NOALIAS       faceQLeft,
    double* const NOALIAS       faceQRight,
    double* const NOALIAS       faceQBottom,
    double* const NOALIAS       faceQUp,
    double* const NOALIAS       faceQFront,
    double* const NOALIAS       faceQBack
  );

  /**
   * Given a numerical flux at the various faces, this computes and adds the Riemann
   * integral of this flux to the nodes of a 2-dimensional cell.
   *
   * @param faceQLeft Numerical flux at each node along the left face. This is
   *   typically computed by a Riemann solver such as a Rusanov solver. The same
   *   holds for faceQRight, faceQBottom and faceQUp, which carry the fluxes of
   *   the remaining three faces.
   *
   * @param cellSize Extent of the cell along each coordinate axis. It scales the
   *   face integral against the cell volume.
   *
   * @param BasisFunctionValuesLeft Numerical value of the basis functions evaluated
   *   at the left node in one dimension. It determines how strongly the value at the
   *   boundary influences the value at a given node. The basis function values on the
   *   right side are symmetric to those on the left side, so this array serves both.
   *
   * @param MassMatrixDiagonal1d Quadrature weight of each quadrature node in one
   *   dimension.
   *
   * @param cellQ The current solution, expected to be the solution already updated
   *   with local contributions (see Euler.h). The face integral is added to it.
   *
   * @see projectVolumetricDataOntoFaces()
   */
  void integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
    const double* const NOALIAS         faceQLeft,
    const double* const NOALIAS         faceQRight,
    const double* const NOALIAS         faceQBottom,
    const double* const NOALIAS         faceQUp,
    int                                 order,
    int                                 unknowns,
    const int                           auxiliaryVariables,
    const tarch::la::Vector<2, double>& cellSize,
    const double* const NOALIAS         BasisFunctionValuesLeft,
    const double* NOALIAS               MassMatrixDiagonal1d,
    double* NOALIAS                     cellQ
  );

  /**
   * 3D counterpart of integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre()
   *
   * The routine relies on a left-handed coordinate system. So we have a cube
   * with a left and a right face. Those are the faces 0 and 3
   * (=0+DIMENSIONS). Their normal points along the x axis. Next, we have
   * bottom and top face, as they have a normal along the y axis and y is the
   * second coordinate axis. Finally, the front and the back face follow.
   * They are the faces 2 and 2+DIMENSIONS=5. The front face comes first, as
   * we work with a left-handed coordinate system, i.e. the normal points
   * into the screen.
   */
  void integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
    const double* const NOALIAS         faceQLeft,
    const double* const NOALIAS         faceQRight,
    const double* const NOALIAS         faceQBottom,
    const double* const NOALIAS         faceQUp,
    const double* const NOALIAS         faceQFront,
    const double* const NOALIAS         faceQBack,
    int                                 order,
    int                                 unknowns,
    const int                           auxiliaryVariables,
    const tarch::la::Vector<3, double>& cellSize,
    const double* const NOALIAS         BasisFunctionValuesLeft,
    const double* NOALIAS               MassMatrixDiagonal1d,
    double* NOALIAS                     cellQ
  );
} // namespace exahype2::dg
