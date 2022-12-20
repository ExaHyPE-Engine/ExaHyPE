// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>
#include <vector>


namespace exahype2 {
  namespace dg {
    struct PointSource {
      const tarch::la::Vector<Dimensions,double>&  x;
      const double* __restrict__                   values;
    };

    /**
     * Flux functor
     *
     * This functor defines the signature of the flux evaluation.
     *
     * If you use a template compute kernel,
     *
     * ## Comparison to Finite Volume flux
     *
     * The finite volume flux accepts a volume size h as well. For DG, we do not
     * pass an h, as we actually work point-wisely: the flux is evaluated in integration
     * points and then scales an underlying shape function. So there's no need for any
     * spatial averaging, e.g.
     */
    typedef std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F
    ) >  Flux;

    typedef std::function< void(
      const double * __restrict__                  Q,
      const double * __restrict__                  dQdx,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F
    ) >   NonConservativeProduct;

    /**
     * Source functor
     *
     * This functor defines the signature of the source evaluation.
     *
     * ## Comparison to Finite Volume source
     *
     * The finite volume source accepts a volume size h as well. For DG, we do not
     * pass an h, as we actually work point-wisely: the source is evaluated in integration
     * points and then scales an underlying shape function. So there's no need for any
     * spatial averaging, e.g.
     *
     */
    typedef std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__                        S
    ) >   Source;

    /**
     * This is the only routine within the DG framework which accepts the dimensions
     * of the underlying cell rather than only a point.
     */
    typedef std::function< std::vector<PointSource>(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  h,
      double                                       t,
      double                                       dt
    ) >   PointSources;

    typedef std::function< double(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) >   MaxEigenvalue;



    /*
     * This is only used within DGUtils in order to apply the boundary conditions
     * to a given face.
     */
    typedef std::function< void(
      const double * __restrict__                  Qinside,
      double * __restrict__                        Qoutside,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal
    ) >   BoundaryConditions;

  }
}


