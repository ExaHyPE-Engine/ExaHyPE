// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_FUNCTORS_H_
#define _EXAHYPE2_FV_RUSANOV_FUNCTORS_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      typedef std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  volumeX,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        double * __restrict__ S
      ) >  Source;

      typedef std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ F
      ) >  Flux;

      typedef std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__     deltaQ,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ BgradQ
      ) >   NonconservativeProduct;

      typedef std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   MaxEigenvalue;
    }
  }
}

#endif
