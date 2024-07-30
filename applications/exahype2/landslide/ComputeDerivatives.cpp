// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ComputeDerivatives.h"

#include "peano4/utils/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/NonCriticalAssertions.h"

enum Derivatives {
  h_x = 1,
  h_y,
  u_x,
  u_y,
  u_xx,
  u_yy,
  u_xy,
  v_x,
  v_y,
  v_xx,
  v_yy,
  v_xy
};

enum Unknowns { h = 0, hu, hv };

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::landslide::computeDerivatives(
  double* __restrict__ oldQWithHalo,
  int numberOfDofs,
  int numberOfUnknowns,
  int numberOfAuxiliaryVariables
) {
  const int totalVariables = numberOfUnknowns + numberOfAuxiliaryVariables;

  for (int y = 1; y < numberOfDofs + 1; y++) {
    for (int x = 1; x < numberOfDofs + 1; x++) {
      const ::tarch::la::Vector<Dimensions, int> currentCell = {x, y};
      const ::tarch::la::Vector<Dimensions, int>
        xLeftNeighbourCell = {x - 1, y};
      const ::tarch::la::Vector<Dimensions, int>
        xRightNeighbourCell = {x + 1, y};
      const ::tarch::la::Vector<Dimensions, int>
        yLeftNeighbourCell = {x, y - 1};
      const ::tarch::la::Vector<Dimensions, int>
        yRightNeighbourCell = {x, y + 1};
      const ::tarch::la::Vector<Dimensions, int>
        xLeftYLeftNeighbourCell = {x - 1, y - 1};
      const tarch::la::Vector<Dimensions, int>
        xLeftYRightNeighbourCell = {x - 1, y + 1};
      const tarch::la::Vector<Dimensions, int>
        xRightYLeftNeighbourCell = {x + 1, y - 1};
      const tarch::la::Vector<Dimensions, int>
        xRightYRightNeighbourCell = {x + 1, y + 1};


      const int currentCellSerialized = ::peano4::utils::dLinearised(
        currentCell,
        numberOfDofs + 2
      );
      const int xLeftNeighbourCellSerialized = ::peano4::utils::dLinearised(
        xLeftNeighbourCell,
        numberOfDofs + 2
      );
      const int xRightNeighbourCellSerialized = ::peano4::utils::dLinearised(
        xRightNeighbourCell,
        numberOfDofs + 2
      );
      const int yLeftNeighbourCellSerialized = ::peano4::utils::dLinearised(
        yLeftNeighbourCell,
        numberOfDofs + 2
      );
      const int yRightNeighbourCellSerialized = ::peano4::utils::dLinearised(
        yRightNeighbourCell,
        numberOfDofs + 2
      );

      const int xLeftYLeftNeighbourCellSerialized = ::peano4::utils::
        dLinearised(xLeftYLeftNeighbourCell, numberOfDofs + 2);

      const int xLeftYRightNeighbourCellSerialized = ::peano4::utils::
        dLinearised(xLeftYRightNeighbourCell, numberOfDofs + 2);

      const int xRightYLeftNeighbourCellSerialized = ::peano4::utils::
        dLinearised(xRightYLeftNeighbourCell, numberOfDofs + 2);

      const int xRightYRightNeighbourCellSerialized = ::peano4::utils::
        dLinearised(xRightYRightNeighbourCell, numberOfDofs + 2);

      const double hCurrentCell = oldQWithHalo
        [currentCellSerialized * totalVariables + h];
      const double uCurrentCell
        = (::tarch::la::greater(oldQWithHalo[currentCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[currentCellSerialized * totalVariables + hu] / oldQWithHalo[currentCellSerialized * totalVariables + h] : 0.0);
      const double vCurrentCell
        = (::tarch::la::greater(oldQWithHalo[currentCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[currentCellSerialized * totalVariables + hv] / oldQWithHalo[currentCellSerialized * totalVariables + h] : 0.0);

      const double hXLeftNeighbourCell = oldQWithHalo
        [xLeftNeighbourCellSerialized * totalVariables + h];
      const double uXLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vXLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double hXRightNeighbourCell = oldQWithHalo
        [xRightNeighbourCellSerialized * totalVariables + h];
      const double uXRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double vXRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double hYLeftNeighbourCell = oldQWithHalo
        [yLeftNeighbourCellSerialized * totalVariables + h];
      const double uYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double hYRightNeighbourCell = oldQWithHalo
        [yRightNeighbourCellSerialized * totalVariables + h];
      const double uYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double vYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double uXLeftYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vXLeftYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double uXLeftYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vXLeftYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double uXRightYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vXRightYLeftNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

      const double uXRightYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + hu] / oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
      const double vXRightYRightNeighbourCell
        = (::tarch::la::greater(oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h], 0.0) ? oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + hv] / oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h] : 0.0);

      // Derivative calculation:
      // Backward difference for single derivatives.
      // Central difference for double derivatives.
      // Only numerator is of the finite difference scheme is calculated here.
      // The division by the denominator takes place in the ncp function after
      // retrieval from average value. For single derivatives, one can change
      // the numerical scheme to forward, central or backward by using
      // appropriate neighbour cell values defined above. For double
      // derivatives, only central difference can be used with the variables
      // defined above.

      // h_x
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + h_x]
        = (hCurrentCell - hXLeftNeighbourCell);

      // h_y
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + h_y]
        = (hCurrentCell - hYLeftNeighbourCell);

      // u_x
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + u_x]
        = (uCurrentCell - uXLeftNeighbourCell);

      // u_y
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + u_y]
        = (uCurrentCell - uYLeftNeighbourCell);

      // u_xx
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + u_xx]
        = (uXRightNeighbourCell - 2 * uCurrentCell + uXLeftNeighbourCell);

      // u_yy
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + u_yy]
        = (uYRightNeighbourCell - 2 * uCurrentCell + uYLeftNeighbourCell);

      // u_xy
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + u_xy]
        = (uXRightYRightNeighbourCell - uXLeftYRightNeighbourCell - uXRightYLeftNeighbourCell + uXLeftYLeftNeighbourCell);

      // v_x
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + v_x]
        = (vCurrentCell - vXLeftNeighbourCell);

      // v_y
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + v_y]
        = (vCurrentCell - vYLeftNeighbourCell);

      // v_xx
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + v_xx]
        = (vXRightNeighbourCell - 2 * vCurrentCell + vXLeftNeighbourCell);

      // v_yy
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + v_yy]
        = (vYRightNeighbourCell - 2 * vCurrentCell + vYLeftNeighbourCell);

      // v_xy
      oldQWithHalo
        [currentCellSerialized * totalVariables + numberOfUnknowns + v_xy]
        = (vXRightYRightNeighbourCell - vXLeftYRightNeighbourCell - vXRightYLeftNeighbourCell + vXLeftYLeftNeighbourCell);
    }
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
