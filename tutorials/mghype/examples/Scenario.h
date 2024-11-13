#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


namespace tutorials {
  namespace mghype {
    namespace examples {
      double getPointWiseRhs(
        const tarch::la::Vector<Dimensions, double>&  x
      );
    }
  }
}

