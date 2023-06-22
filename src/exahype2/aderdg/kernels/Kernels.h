/**
 * This file is part of the ExaHyPE project.
 * Copyright (c) 2016  http://exahype.eu
 * All rights reserved.
 *
 * The project has received funding from the European Union's Horizon
 * 2020 research and innovation programme under grant agreement
 * No 671698. For copyrights and licensing, please consult the webpage.
 *
 * Released under the BSD 3 Open Source License.
 * For the full license text, see LICENSE.txt
 **/

#pragma once

#include <string>
#include <vector>

#include "Basis/GaussLegendreBasis.h"
#include "Basis/GaussLobattoBasis.h"
#include "../solvers/ADERDGSolver.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"

#define MbasisSize 4
#define Mvar 5
#define Mdim 3
#define f2p5(var, dim, i, j, k) \
  (var + Mvar * dim + Mvar * Mdim * i + Mvar * Mdim * MbasisSize * j + Mvar * Mdim * MbasisSize * MbasisSize * k)
#define p2f5(var, dim, i, j, k) \
  (dim * MbasisSize * MbasisSize * MbasisSize * Mvar + Mvar * i + Mvar * MbasisSize * j \
   + Mvar * MbasisSize * MbasisSize * k + var)

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

namespace kernels {
  namespace aderdg {
    namespace generic {
      namespace c {

        /**
         * @param SolverType Has to be of type ADERDG Solver.
         */
        template <
          bool usePointSource,
          bool useSource,
          bool useFlux,
          bool useNCP,
          bool useMM,
          typename SolverType,
          typename pCompType,
          typename pStoreType>
        void spaceTimePredictorLinear(
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
        );

        template <
          bool useSource,
          bool useFlux,
          bool useNCP,
          bool noTimeAveraging,
          typename SolverType,
          typename pCompType,
          typename pStoreType>
        int spaceTimePredictorNonlinear(
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
        );

        template <typename SolverType, typename pType>
        void solutionUpdate(
          SolverType& solver, double* luh, const double* const luhOld, const pType* const lduh, const double dt
        );

        template <
          typename SolverType,
          typename pCompType,
          bool useSourceOrNCP,
          bool useFlux,
          int  numberOfVariables,
          int  basisSize>
        void volumeIntegralLinear(
          pCompType* lduh, const pCompType* const lFhi, const tarch::la::Vector<Dimensions, double>& dx
        );

        template <
          typename SolverType,
          typename pCompType,
          bool useSourceOrNCP,
          bool useFlux,
          bool noTimeAveraging,
          int  numberOfVariables,
          int  basisSize>
        void volumeIntegralNonlinear(
          pCompType* lduh, const pCompType* const lFi, const tarch::la::Vector<Dimensions, double>& dx
        );

        template <typename SolverType, int numberOfVariables, int basisSize>
        void faceIntegralNonlinear(
          double*                                      lduh,
          const double* const                          lFhbnd,
          const int                                    direction,
          const int                                    orientation,
          const tarch::la::Vector<Dimensions, double>& dx
        );

        template <typename SolverType, int numberOfVariables, int basisSize>
        void faceIntegralLinear(
          double*                                      lduh,
          const double* const                          lFhbnd,
          const int                                    direction,
          const int                                    orientation,
          const tarch::la::Vector<Dimensions, double>& dx
        );

        // todo 10/02/16: Dominic
        // Keep only one surfaceIntegral.
        template <typename SolverType, int numberOfVariables, int basisSize>
        void surfaceIntegralNonlinear(
          double* lduh, const double* const lFbnd, const tarch::la::Vector<Dimensions, double>& dx
        );

        template <typename SolverType, int numberOfVariables, int basisSize>
        void surfaceIntegralLinear(
          double* lduh, const double* const lFbnd, const tarch::la::Vector<Dimensions, double>& dx
        );

        /*void surfaceIntegral2(
            double* lduh,
            const double* const lFhbnd,
            const tarch::la::Vector<Dimensions,double>&  dx,
            const int numberOfVariables,
            const int basisSize
        );*/

        template <typename SolverType>
        void solutionAdjustment(
          SolverType&                                  solver,
          double*                                      luh,
          const tarch::la::Vector<Dimensions, double>& center,
          const tarch::la::Vector<Dimensions, double>& dx,
          const double                                 t,
          const double                                 dt
        );

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
        template <bool useNCP, typename SolverType>
        void riemannSolverNonlinear(
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
        );

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
        template <bool useFlux, bool useNCP, typename SolverType>
        void generalisedOsherSolomon(
          SolverType&         solver,
          double* const       FL,
          double* const       FR,
          const double* const QL,
          const double* const QR,
          const double        t,
          const double        dt,
          const int           direction
        );

        template <bool useGradientFlux, typename SolverType>
        void boundaryConditions(
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
        );

        template <typename SolverType, typename T>
        double maxScaledEigenvalue(
          SolverType&                                  solver,
          const T* const                               luh,
          const tarch::la::Vector<Dimensions, double>& cellCentre,
          const tarch::la::Vector<Dimensions, double>& dx,
          const double                                 t,
          const double                                 dt
        );

        /**
         * \note We need to consider material parameters in
         * lQhbndFine and lQhbndCoarse.
         */

        template <typename SolverType, typename pStoreType, int numberOfVariables, int basisSize>
        void singleLevelFaceUnknownsProlongation(
          pStoreType*                                   lQhbndFine,
          const pStoreType*                             lQhbndCoarse,
          const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
        );

        template <typename SolverType, typename pStoreType, int numberOfVariables, int basisSize>
        void singleLevelFaceUnknownsRestriction(
          pStoreType*                                   lQhbndCoarse,
          const pStoreType*                             lQhbndFine,
          const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
        );

        template <typename SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
        void faceUnknownsProlongation(
          double*                                       lQhbndFine,
          double*                                       lFhbndFine,
          const double*                                 lQhbndCoarse,
          const double*                                 lFhbndCoarse,
          const int                                     coarseGridLevel,
          const int                                     fineGridLevel,
          const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
        );

        template <typename SolverType, int numberOfVariables, int basisSize>
        void faceUnknownsRestriction(
          double* const                                 lFhbndCoarse,
          const double* const                           lFhbndFine,
          const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex,
          const int                                     levelDelta
        );

        /**
         * \note We need to consider material parameters in
         * lQhbndFine and lQhbndCoarse.
         *
         * @\deprecated
         */
        template <typename SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
        void faceUnknownsRestriction(
          double*                                       lQhbndCoarse,
          double*                                       lFhbndCoarse,
          const double*                                 lQhbndFine,
          const double*                                 lFhbndFine,
          const int                                     coarseGridLevel,
          const int                                     fineGridLevel,
          const tarch::la::Vector<Dimensions - 1, int>& subfaceIndex
        );

        /**
         * \note We need to consider material parameters in
         * luhCoarse and luhFine.
         */
        template <typename SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
        void volumeUnknownsProlongation(
          double*                                   luhFine,
          const double*                             luhCoarse,
          const int                                 coarseGridLevel,
          const int                                 fineGridLevel,
          const tarch::la::Vector<Dimensions, int>& subcellIndex
        );

        /**
         * \note We need to consider material parameters in
         * luhCoarse and luhFine.
         */
        template <typename SolverType, int numberOfVariables, int numberOfParameters, int basisSize>
        void volumeUnknownsRestriction(
          double*                                   luhCoarse,
          const double*                             luhFine,
          const int                                 coarseGridLevel,
          const int                                 fineGridLevel,
          const tarch::la::Vector<Dimensions, int>& subcellIndex
        );

        template <typename SolverType>
        std::vector<int>* getPointSources(
          SolverType&                                  solver,
          const tarch::la::Vector<Dimensions, double>& center,
          const tarch::la::Vector<Dimensions, double>& dx
        );

        template <typename SolverType>
        void deltaDistribution(
          SolverType&                                  solver,
          const double* const                          luh,
          const double                                 t,
          const double                                 dt,
          const tarch::la::Vector<Dimensions, double>& center,
          const tarch::la::Vector<Dimensions, double>& dx,
          std::vector<int>*                            pointSources, // will be deleted in the end
          double*                                      PSi
        );

      } // namespace c
    }   // namespace generic
  }     // namespace aderdg
} // namespace kernels

#include "Aderdg/generalisedOsherSolomon.cpph"

#if Dimensions == 2
#include "Aderdg/2d/amrRoutines.cpph"
#include "Aderdg/2d/boundaryConditions.cpph"
#include "Aderdg/2d/deltaDistribution.cpph"
#include "Aderdg/2d/faceIntegralLinear.cpph"
#include "Aderdg/2d/faceIntegralNonlinear.cpph"
#include "Aderdg/2d/maxScaledEigenvalue.cpph"
#include "Aderdg/2d/riemannSolverLinear.cpph"
#include "Aderdg/2d/riemannSolverNonlinear.cpph"
#include "Aderdg/2d/solutionAdjustment.cpph"
#include "Aderdg/2d/solutionUpdate.cpph"
#include "Aderdg/2d/spaceTimePredictorLinear.cpph"
#include "Aderdg/2d/spaceTimePredictorNonlinear.cpph"
#include "Aderdg/2d/surfaceIntegralLinear.cpph"
#include "Aderdg/2d/surfaceIntegralNonlinear.cpph"
#include "Aderdg/2d/volumeIntegralLinear.cpph"
#include "Aderdg/2d/volumeIntegralNonlinear.cpph"
#elif Dimensions == 3
#include "Aderdg/3d/amrRoutines.cpph"
#include "Aderdg/3d/boundaryConditions.cpph"
#include "Aderdg/3d/deltaDistribution.cpph"
#include "Aderdg/3d/faceIntegralLinear.cpph"
#include "Aderdg/3d/faceIntegralNonlinear.cpph"
#include "Aderdg/3d/maxScaledEigenvalue.cpph"
#include "Aderdg/3d/riemannSolverLinear.cpph"
#include "Aderdg/3d/riemannSolverNonlinear.cpph"
#include "Aderdg/3d/solutionAdjustment.cpph"
#include "Aderdg/3d/solutionUpdate.cpph"
#include "Aderdg/3d/spaceTimePredictorLinear.cpph"
#include "Aderdg/3d/spaceTimePredictorNonlinear.cpph"
#include "Aderdg/3d/surfaceIntegralLinear.cpph"
#include "Aderdg/3d/surfaceIntegralNonlinear.cpph"
#include "Aderdg/3d/volumeIntegralLinear.cpph"
#include "Aderdg/3d/volumeIntegralNonlinear.cpph"
#endif
