#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


namespace tests {
  namespace multigrid {
    namespace matrixfree {
      namespace poisson {
        double getPointWiseRhs(
          const tarch::la::Vector<Dimensions, double>&  x
        );
      }
    }
  }
}

