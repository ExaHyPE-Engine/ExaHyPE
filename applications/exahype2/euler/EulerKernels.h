// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


namespace applications {
  namespace exahype2 {
    namespace euler {
      /**
       * Apply Neumann BCs to all quantities. This includes the material
       * parameters (auxiliary ones).
       */
      void NeumannBoundaryConditions(
        const double * __restrict__                  Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables
      ) InlineMethod;


      /**
       * Dimension-generic implementation of the max eigenvalue computation
       *
       * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
       * return one scalar over the numberOfUnknowns PDE equations.
       */
      double maxEigenvalue(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double                                       gamma = 1.4
      ) InlineMethod;


      /**
       * GPU variant of the max eigenvalue computation
       *
       * It differs from the vanilla version in two ways:
       *
       * - It does not host any assertions or validity checks, as those are not
       *   available on an accelerator.
       * - It is all in the header to force/enable the compiler to inline stuff
       *   directly and, hence, to generate GPU code.
       */
      KeywordToAvoidDuplicateSymbolsForInlinedFunctions double maxEigenvalueOnGPU(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double                                       gamma = 1.4
      ) InlineMethod {
        const double irho = 1./Q[0];
        #if Dimensions==3
        const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
        #else
        const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
        #endif

        const double c   = std::sqrt(gamma * p * irho);

        const double u_n = Q[normal + 1] * irho;
        double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );

        return result;
      }


      /**
       * Dimension-generic implementation of the flux computation
       *
       * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
       * return F[numberOfUnknowns] fluxes.
       */
      void flux(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double * __restrict__                        F,
        double                                       gamma = 1.4
      ) InlineMethod;


      /**
       * GPU variant of the flux computation
       *
       * It differs from the vanilla version in two ways:
       *
       * - It does not host any assertions or validity checks, as those are not
       *   available on an accelerator.
       * - It is all in the header to force/enable the compiler to inline stuff
       *   directly and, hence, to generate GPU code.
       */
      KeywordToAvoidDuplicateSymbolsForInlinedFunctions void fluxOnGPU(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double * __restrict__                        F,
        double                                       gamma = 1.4
      ) InlineMethod {
        const double irho = 1./Q[0];
        #if Dimensions==3
        const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
        #else
        const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
        #endif

        const double coeff = irho*Q[normal+1];
        F[0] = coeff*Q[0];
        F[1] = coeff*Q[1];
        F[2] = coeff*Q[2];
        F[normal+1] += p;

        #if Dimensions==3
        F[3] = coeff*Q[3];
        F[4] = coeff*Q[4] + coeff*p;
        #else
        F[3] = coeff*Q[3] + coeff*p;
        #endif
      }
    }
  }
}

