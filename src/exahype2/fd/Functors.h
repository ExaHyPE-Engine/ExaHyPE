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
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  gridCellX,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        double * __restrict__ 		             AlgeSrc
      ) >  Source;

      typedef std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ F
      ) >  Flux;

      typedef std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__                  deltaQ,
        const tarch::la::Vector<Dimensions,double>&  gridCellX,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        DiffSrc
      ) >   NonconservativeProduct;

      /**
       * The max eigenvalue is used if and only if you have adaptive time stepping.
       */
      typedef std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  gridCellX,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   MaxEigenvalue;
  }
}
