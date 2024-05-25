// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


namespace mghype {
  namespace matrixfree {
    namespace solvers {
      namespace dgcgcoupling {
        /**
         * Apply updates from previous mesh sweep in multigrid sense and prepare for next correction step
         *
         * This routine runs through a couple of steps which are required for
         * any correction mesh sweep. We assume that a vertex holds the
         * following fields:
         *
         * - SumOfInjectedValues. Each vertex is adjacent to up to @f$ 2^d @f$
         *   DG patches. Each patch has (potentially) a different value within
         *   the vertex. This field holds the average.
         * - NumberOfRestrictions. See description of SumOfInjectedValues. This
         *   is the number of times we've injected data into the sum.
         * - NewRestrictedRhs. We will work with a given right-hand side.
         *   However, we also might need a (separate) place to restrict the next
         *   right-hand side. This is what this variable is made for.
         *
         *
         * First and foremost, we need a case distinction: If a vertex is
         * outside or if we don't have any valid restricted data, then we cannot
         * realise any data flow from fine to coarse, coarse to fine or only
         * from one iteration to the other. It makes no sense. So, in this
         * case, we just set everything zero. We rely on the fact that the
         * counter NumberOfRestrictions will hold invalid values for the
         * outer vertices - typically 0.
         *
         * Boundary vertices do not really require special treatment. The only
         * important thing in a multigrid sense is that the delta at the
         * boundary point is zero. In a correction scheme, we'd furthermore fix
         * them to homogeneous Dirichlet values, but as we typically favour
         * FAS we take the injected value. We can identify boundary values as
         * the number of restrictions there is smaller than @f$ 2^ @f$.
         *
         * In the very first sweep, we reset the right-hand side, as we don't
         * know its value yet.
         *
         * To prepare of for the next mesh sweep, we reset the restriction and
         * accumulation variables:
         *
         * ~~~~~~~~~~~~~~~~~~~~~~~
         *   vertex.setSumOfInjectedValues( 0.0 );
         *   vertex.setNumberOfRestrictions( 0 );
         *   vertex.setNewRestrictedRhs( 0.0 );
         * ~~~~~~~~~~~~~~~~~~~~~~~
         *
         * It does not hurt if we do this for outer or boundary vertices, so we
         * simply do it everywhere.
         *
         * @param restrictRhsAndInjectSolution Flag to control the behaviour
         *   that allows us to use this routine in different context such as
         *   additive vs multiplicative MG or schemes with multiple coarse
         *   grid sweeps.
         */
        void rollOverAndPrepareCGVertex(
          auto&  vertex,
          bool   restrictRhsAndInjectSolution,
          bool   fas
        );


        /**
         * Restrict counters
         *
         * This routine is typically invoked by touchCellLastTime(). We run
         * through the @f$ 2^d @f$ adjacent vertices of a cell and increment
         * each of its restriction counters by one. Some sanity checks ensure
         * they hold valid data.
         *
         * The standard invocation of this routine resembles
         *
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~
         * mghype::matrixfree::solvers::dgcgcoupling::updateRestrictionCounters(
         *   fineGridVertices{{CG_SOLVER_NAME}}
         * );
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~
         *
         *
         * @param vertices Instance of Peano's VertexEnumerator.
         */
        void updateRestrictionCounters(
          auto&  vertices
        );

        /**
         *
         *      constexpr int Rows = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
      constexpr int Cols = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
         *
         *
         * @param injectionMatrix: Of type tarch::la::Matrix< Rows, Cols, double >
         *   where the constraints above are given on Rows and Cols, i.e. they
         *   have to match the other types.
         */
        template <class DGSolver, class CGSolver>
        void injectSolution(
          const tarch::la::Matrix< TwoPowerD * CGSolver::VertexUnknowns, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  injectionMatrix,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgSolution,
          auto&           cgVertices
        );

        template <class DGSolver, class CGSolver>
        tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double > prolongate(
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, TwoPowerD * CGSolver::VertexUnknowns, double >&  prolongationMatrix,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgSolution,
          const auto&           cgVertices
        );

        template <class DGSolver, class CGSolver>
        void computeAndRestrictResidual(
          const tarch::la::Matrix< TwoPowerD * CGSolver::VertexUnknowns, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  restrictionMatrix,
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  massMatrix,
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  systemMatrix,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgSolution,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgRhs,
          auto&           cgVertices
        );

        template <class DGSolver, class CGSolver>
        void computeAndRestrictHierarchicalResidual(
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, TwoPowerD * CGSolver::VertexUnknowns, double >&  prolongationMatrix,
          const tarch::la::Matrix< TwoPowerD * CGSolver::VertexUnknowns, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  restrictionMatrix,
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  massMatrix,
          const tarch::la::Matrix< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&  systemMatrix,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgSolution,
          const tarch::la::Vector< DGSolver::NodesPerCell * DGSolver::UnknownsPerCellNode, double >&                                        dgRhs,
          auto&           cgVertices
        );

        /**
         * Get the vector of course grid solutions
         */
        template <class CGSolver>
        tarch::la::Vector< TwoPowerD * CGSolver::VertexUnknowns, double > getCoarseGridSolution(
          auto&           cgVertices
        );
      }
    }
  }
}


#include "DGCGCoupling.cpph"

