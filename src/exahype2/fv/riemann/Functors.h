#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>


namespace exahype2 {
  namespace fv {
    namespace riemann {
      typedef std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        lambda
      ) >   Eigenvalues;

      typedef std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      ) >  Flux;

      typedef std::function< double(
        const double * __restrict__                  QR,
        const double * __restrict__                  QL,
        const double * __restrict__                  FR,
        const double * __restrict__                  FL,
        const double * __restrict__                  LR,
        const double * __restrict__                  LL,
        const tarch::la::Vector<Dimensions,double>&  xR,
        const tarch::la::Vector<Dimensions,double>&  xL,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        QSFlux
      ) > SolveRiemannProblem;
    }
  }
}