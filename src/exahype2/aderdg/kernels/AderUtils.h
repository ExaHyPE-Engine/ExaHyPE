// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"


namespace exahype2 {
  namespace aderdg {
    /**
     * In ExaHyPE's Finite Volume setup, a cell hosts a patch of Finite Volumes.
     * When we iterate over these volumes, we typically have to know the centre
     * of the volume.
     *
     * @param quadraturePoints 1d quadrature points over unit interval. This array
     *                         has polynomialOrder+1 entries.
     */
    tarch::la::Vector<2,double>  getQuadraturePoint(
      const tarch::la::Vector<2,double>&  cellCentre,
      const tarch::la::Vector<2,double>&  cellSize,
      const tarch::la::Vector<2,int>&     index,
      int                                 polynomialOrder,
      const double* __restrict__          quadraturePoints
    );


    tarch::la::Vector<3,double>  getQuadraturePoint(
      const tarch::la::Vector<3,double>&  cellCentre,
      const tarch::la::Vector<3,double>&  cellSize,
      const tarch::la::Vector<3,int>&     index,
      int                                 polynomialOrder,
      const double* __restrict__          quadraturePoints
    );

    template<typename T>
    void computeCellErrorIntegral(
      std::function< void(
        const tarch::la::Vector<Dimensions,double>&   position,
        const double                                  t,
        const double                                  dt,
        double*                                       sol
      ) >   exactSolution,
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          Order,
      const double* __restrict__                   quadratureWeights,
      const double* __restrict__                   quadraturePoints,
      const int                                    unknowns,
      const int                                    auxiliary_variables,
      T* __restrict__                              Q,
      double                                       errors[3]
    );

  }
}

#include "AderUtils.cpph"