// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_TEST_TESTUTILS_CPP_
#define _EXAHYPE2_DG_TEST_TESTUTILS_CPP_

#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

namespace exahype2 {
  namespace dg {
    namespace tests{

    /*
     * Contains implementations of the functions required to test the dg implementation
     * for euler equations.
     */
    void eulerFlux(
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double * __restrict__                        F
          );

    void eulerSource(
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          double * __restrict__                        S
        );

    double eulerEigenvalue(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                       dt,
        int                                         normal
        );

    void eulerBoundaryConditions(
            const double * __restrict__                  Qinside,
            double * __restrict__                         Qoutside,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            int                                          normal
          );

    void eulerInitial(
      double * __restrict__  Q,
      int                    node=0
          );

    void elasticInitial(
      double * __restrict__                        Q
          );

    void elasticFlux(
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double * __restrict__                        F
          );

    void elasticSource(
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          double * __restrict__                        S
        );

    double elasticEigenvalue(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        );

    void elasticBoundaryConditions(
            const double * __restrict__                  Qinside,
            double * __restrict__                        Qoutside,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            int                                          normal
          );

    void elasticNonConservativeProduct(  const double * __restrict__  Q,
        const double * __restrict__                                   deltaQ,
        const tarch::la::Vector<Dimensions,double>&                   faceCentre,
        double                                                        t,
        double                                                        dt,
        int                                                           normal,
        double * __restrict__                                         BgradQ
      );


    // test boundary condition function to check whether applying boundary conditions works
    void testBoundaryConditions(
            const double * __restrict__                  Qinside,
            double * __restrict__                        Qoutside,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            int                                          normal
          );


    double testEigenvalue(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        );

    }
  }
}

#endif
