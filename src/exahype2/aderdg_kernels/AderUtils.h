// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADER_ADER_UTILS_H_
#define _EXAHYPE2_ADER_ADER_UTILS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


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

  }
}

#endif
