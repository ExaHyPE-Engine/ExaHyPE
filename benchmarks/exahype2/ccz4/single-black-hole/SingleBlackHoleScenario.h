// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "CCZ4Kernels.h"
#include "InitialValues.h"


#include "exahype2/RefinementControl.h"


namespace benchmarks {
  namespace exahype2 {
    namespace ccz4 {
      extern TP::TwoPunctures* twoPunctures;

      /**
       * Prepare scenario
       *
       * @Han Is this a good name for this function? If not, can we rename it
       *
       * This routine is to be called once in the beginning. It is idempotent,
       * i.e. you can call it as often as you want. Consequently, the constructor
       * of the solvers is a good place to invoke it.
       */
      void prepareTwoPunctures();

      /**
       * @todo Han This routine deserves some documentation
       */
      ::exahype2::RefinementCommand refinementCriterion(
        const tarch::la::Vector<Dimensions, double>& x,
        const tarch::la::Vector<Dimensions, double>& h,
        double                                       maxH,
        double                                       minH
      );

    }
  }
}
