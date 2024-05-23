// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


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
         * Boundary vertices
         *
         * Along the same lines, we don't have to care about special treatment
         * for boundary vertices, i.e. some marker semantics there. If a vertex
         * has less than @f$ 2^d @f$ adjacent CG patches, we can assume that it
         * is a boundary vertex (or outer as sketched above).
         *
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
          bool   restrictRhsAndInjectSolution
        );
      }
    }
  }
}


#include "DGCGCoupling.cpph"

