// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExtrapolateHalo.h"

#include "peano4/utils/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/NonCriticalAssertions.h"

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::landslide::extrapolateHalo(
  double* __restrict__ oldQWithHalo,
  int numberOfDofs,
  int numberOfUnknowns,
  int numberOfAuxiliaryVariables
) {
  ::tarch::la::Vector<Dimensions, int> topLeftCell = {0, numberOfDofs + 1};
  ::tarch::la::Vector<Dimensions, int>
    topRightCell = {numberOfDofs + 1, numberOfDofs + 1};
  ::tarch::la::Vector<Dimensions, int> bottomLeftCell  = {0, 0};
  ::tarch::la::Vector<Dimensions, int> bottomRightCell = {numberOfDofs + 1, 0};

  ::tarch::la::Vector<Dimensions, int> topLeftCellN1 = topLeftCell;
  topLeftCellN1(0)++;
  ::tarch::la::Vector<Dimensions, int> topLeftCellN2 = topLeftCell;
  topLeftCellN2(1)--;

  ::tarch::la::Vector<Dimensions, int> topRightCellN1 = topRightCell;
  topRightCellN1(0)--;
  ::tarch::la::Vector<Dimensions, int> topRightCellN2 = topRightCell;
  topRightCellN2(1)--;

  ::tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = bottomLeftCell;
  bottomLeftCellN1(0)++;
  ::tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = bottomLeftCell;
  bottomLeftCellN2(1)++;

  ::tarch::la::Vector<Dimensions, int> bottomRightCellN1 = bottomRightCell;
  bottomRightCellN1(0)--;
  ::tarch::la::Vector<Dimensions, int> bottomRightCellN2 = bottomRightCell;
  bottomRightCellN2(1)++;

  const int topLeftCellSerialised = ::peano4::utils::dLinearised(
    topLeftCell,
    numberOfDofs + 2
  );
  const int topRightCellSerialised = ::peano4::utils::dLinearised(
    topRightCell,
    numberOfDofs + 2
  );

  const int bottomLeftCellSerialised = ::peano4::utils::dLinearised(
    bottomLeftCell,
    numberOfDofs + 2
  );
  const int bottomRightCellSerialised = ::peano4::utils::dLinearised(
    bottomRightCell,
    numberOfDofs + 2
  );

  const int topLeftCellN1Serialised = ::peano4::utils::dLinearised(
    topLeftCellN1,
    numberOfDofs + 2
  );
  const int topLeftCellN2Serialised = ::peano4::utils::dLinearised(
    topLeftCellN2,
    numberOfDofs + 2
  );

  const int topRightCellN1Serialised = ::peano4::utils::dLinearised(
    topRightCellN1,
    numberOfDofs + 2
  );
  const int topRightCellN2Serialised = ::peano4::utils::dLinearised(
    topRightCellN2,
    numberOfDofs + 2
  );

  const int bottomLeftCellN1Serialised = ::peano4::utils::dLinearised(
    bottomLeftCellN1,
    numberOfDofs + 2
  );
  const int bottomLeftCellN2Serialised = ::peano4::utils::dLinearised(
    bottomLeftCellN2,
    numberOfDofs + 2
  );

  const int bottomRightCellN1Serialised = ::peano4::utils::dLinearised(
    bottomRightCellN1,
    numberOfDofs + 2
  );
  const int bottomRightCellN2Serialised = ::peano4::utils::dLinearised(
    bottomRightCellN2,
    numberOfDofs + 2
  );

  for (int i = 0; i < numberOfUnknowns + numberOfAuxiliaryVariables; i++) {
    oldQWithHalo
      [(topLeftCellSerialised) * (numberOfUnknowns + numberOfAuxiliaryVariables)
       + i]
      = 0.5
        * (oldQWithHalo[(topLeftCellN1Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i] + oldQWithHalo[(topLeftCellN2Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i]);
    oldQWithHalo
      [(topRightCellSerialised
       ) * (numberOfUnknowns + numberOfAuxiliaryVariables)
       + i]
      = 0.5
        * (oldQWithHalo[(topRightCellN1Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i] + oldQWithHalo[(topRightCellN2Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i]);

    oldQWithHalo
      [(bottomLeftCellSerialised
       ) * (numberOfUnknowns + numberOfAuxiliaryVariables)
       + i]
      = 0.5
        * (oldQWithHalo[(bottomLeftCellN1Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i] + oldQWithHalo[(bottomLeftCellN2Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i]);

    oldQWithHalo
      [(bottomRightCellSerialised
       ) * (numberOfUnknowns + numberOfAuxiliaryVariables)
       + i]
      = 0.5
        * (oldQWithHalo[(bottomRightCellN1Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i] + oldQWithHalo[(bottomRightCellN2Serialised) * (numberOfUnknowns + numberOfAuxiliaryVariables) + i]);
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
