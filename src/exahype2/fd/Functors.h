// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"

#include <functional>

namespace exahype2 {
  namespace fd {
      typedef std::function< void(
        const double * NOALIAS Q,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellX,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        double * NOALIAS 		             AlgeSrc
      ) >  Source;

      typedef std::function< void(
        const double * NOALIAS Q,
        const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * NOALIAS F
      ) >  Flux;

      typedef std::function< void(
        const double * NOALIAS                  Q,
        const double * NOALIAS                  deltaQ,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellX,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * NOALIAS                        DiffSrc
      ) >   NonconservativeProduct;

      /**
       * The max eigenvalue is used if and only if you have adaptive time stepping.
       */
      typedef std::function< double(
        const double * NOALIAS Q,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellX,
        const tarch::la::Vector<DIMENSIONS,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   MaxEigenvalue;
  }
}
