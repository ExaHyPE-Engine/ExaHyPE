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
    void eulerFlux(
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            int                                          normal,
            double * __restrict__                        F
          );

    void eulerSource(
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double * __restrict__                        S
        );

    double eulerEigenvalue(
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                          normal
        );

    void eulerBoundaryConditions(
            const double * __restrict__                  Qinside,
            double * __restrict__                         Qoutside,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            int                                          normal
          );
    }
  }
}

#endif
