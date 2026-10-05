// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CurviTest.h"

#include "exahype2/Curvi.h"
#include "tarch/la/Vector.h"

exahype2::tests::CurviTest::CurviTest() :
  tarch::tests::TestCase("exahype2::tests::CurviTest") {}

void
exahype2::tests::CurviTest::run() {
  testMethod(testCreateBlock);
  testMethod(testTransverselyVaryingPerturbation);
  testMethod(testLowerFacePerturbation);
}

void
exahype2::tests::CurviTest::testCreateBlock() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {-1., 0., 1.};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {1., 10., 100.};
  const tarch::la::Vector<DIMENSIONS, int>    numElements  = {5, 7, 9};
  const tarch::la::Vector<DIMENSIONS, double> cellSize     = {
    domainSize[0] / numElements[0],
    domainSize[1] / numElements[1]
#if DIMENSIONS == 3
    ,
    domainSize[2] / numElements[2]
#endif
  };

  exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

  const exahype2::Curvi::Perturbation perturbationX =
    [domainSize]([[maybe_unused]] tarch::la::Vector<DIMENSIONS, double> x) { return domainSize[0] / 20; };

  const double splitPositionX = curvi.split(0, domainOffset[0] + 0.5 * domainSize[0], perturbationX);

  const int leftElements = static_cast<int>(std::round(static_cast<double>(numElements[0]) * 0.5));
  validateNumericalEquals(splitPositionX, (domainOffset[0] + cellSize[0] * leftElements));

  const exahype2::Curvi::Perturbation perturbationY =
    [domainSize]([[maybe_unused]] tarch::la::Vector<DIMENSIONS, double> x) { return domainSize[1] / 10; };
  curvi.applyPerturbation(1 + DIMENSIONS, perturbationY);

#if DIMENSIONS == 3
  const exahype2::Curvi::Perturbation perturbationZ = [](tarch::la::Vector<DIMENSIONS, double> x) {
    return 0.1 * (x[0] + x[1]);
  };
  const double splitPositionZ = curvi.split(2, domainOffset[2] + 0.5 * domainSize[2], perturbationZ);

  const int backElements = static_cast<int>(std::round(static_cast<double>(numElements[2]) * 0.5));
  validateNumericalEquals(splitPositionZ, (domainOffset[2] + cellSize[2] * backElements));
#endif

#if DIMENSIONS == 3
  for (int k = 0; k <= numElements[2]; k++) {
#else
  {
#endif
    for (int j = 0; j <= numElements[1]; j++) {
      for (int i = 0; i <= numElements[0]; i++) {
        const tarch::la::Vector<DIMENSIONS, double> pointPos = {
          domainOffset[0] + cellSize[0] * i,
          domainOffset[1] + cellSize[1] * j
#if DIMENSIONS == 3
          ,
          domainOffset[2] + cellSize[2] * k
#endif
        };
        const tarch::la::Vector<DIMENSIONS, double> blockstructuredPoint = curvi(pointPos);

        if (i <= leftElements) {
          const double leftCellSize = (1.1 * domainSize[0] / 2) / leftElements;
          validateNumericalEquals(blockstructuredPoint[0], (domainOffset[0] + i * leftCellSize));
        } else {
          const double rightCellSize = (0.9 * domainSize[0] / 2) / (numElements[0] - leftElements);
          validateNumericalEquals(
            blockstructuredPoint[0],
            (domainOffset[0] + 1.1 * 0.5 * domainSize[0] + (i - leftElements) * rightCellSize)
          );
        }

        // The upper face along y moves by a tenth of the domain size.
        validateNumericalEquals(blockstructuredPoint[1], (domainOffset[1] + 1.1 * j * cellSize[1]));

#if DIMENSIONS == 3
        // The interface moves by a tenth of the sum of the two transverse coordinates.
        const double perturbation = 0.1 * (blockstructuredPoint[0] + blockstructuredPoint[1]);
        if (k <= backElements) {
          const double backCellSize = (0.5 * domainSize[2] + perturbation) / backElements;
          validateNumericalEquals(blockstructuredPoint[2], (domainOffset[2] + k * backCellSize));
        } else {
          const double backCellSize = (0.5 * domainSize[2] - perturbation) / (numElements[2] - backElements);
          validateNumericalEquals(
            blockstructuredPoint[2],
            (domainOffset[2] + 0.5 * domainSize[2] + perturbation + (k - backElements) * backCellSize)
          );
        }
#endif

        validateNumericalVectorEquals(curvi.invertProjection(blockstructuredPoint), pointPos);
      }
    }
  }
}

/**
 * The round trip returns every point for a face perturbation that varies along an axis an earlier
 * transformation stretched.
 */
void
exahype2::tests::CurviTest::testTransverselyVaryingPerturbation() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0., 0., 0.};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {10., 10., 10.};
  const int                                   numElements  = 5;
  const tarch::la::Vector<DIMENSIONS, double> cellSize     = {
    domainSize[0] / numElements,
    domainSize[1] / numElements
#if DIMENSIONS == 3
    ,
    domainSize[2] / numElements
#endif
  };

  exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

  // The split makes the first coordinate differ between the cartesian and the curvilinear point.
  curvi.split(0, 4.0, []([[maybe_unused]] tarch::la::Vector<DIMENSIONS, double> x) { return 1.0; });

  // The upper face along the second axis moves by an amount that depends on the first coordinate.
  curvi.applyPerturbation(1 + DIMENSIONS, [](tarch::la::Vector<DIMENSIONS, double> x) { return 0.05 * x[0]; });

#if DIMENSIONS == 3
  for (int k = 0; k <= numElements; k++) {
#else
  {
#endif
    for (int j = 0; j <= numElements; j++) {
      for (int i = 0; i <= numElements; i++) {
        const tarch::la::Vector<DIMENSIONS, double> pointPos = {
          domainOffset[0] + cellSize[0] * i,
          domainOffset[1] + cellSize[1] * j
#if DIMENSIONS == 3
          ,
          domainOffset[2] + cellSize[2] * k
#endif
        };

        validateNumericalVectorEquals(curvi.invertProjection(curvi(pointPos)), pointPos);
      }
    }
  }
}

/**
 * A lower-face perturbation moves the face by its offset, and the round trip returns every point.
 */
void
exahype2::tests::CurviTest::testLowerFacePerturbation() {
  const tarch::la::Vector<DIMENSIONS, double> domainOffset = {0., 0., 0.};
  const tarch::la::Vector<DIMENSIONS, double> domainSize   = {10., 10., 10.};
  const int                                   numElements  = 5;
  const tarch::la::Vector<DIMENSIONS, double> cellSize     = {
    domainSize[0] / numElements,
    domainSize[1] / numElements
#if DIMENSIONS == 3
    ,
    domainSize[2] / numElements
#endif
  };

  exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

  // A face index below DIMENSIONS denotes the lower face along that axis.
  constexpr double Lift = 2.0;
  curvi.applyPerturbation(0, []([[maybe_unused]] tarch::la::Vector<DIMENSIONS, double> x) { return Lift; });

#if DIMENSIONS == 3
  for (int k = 0; k <= numElements; k++) {
#else
  {
#endif
    for (int j = 0; j <= numElements; j++) {
      for (int i = 0; i <= numElements; i++) {
        const tarch::la::Vector<DIMENSIONS, double> pointPos = {
          domainOffset[0] + cellSize[0] * i,
          domainOffset[1] + cellSize[1] * j
#if DIMENSIONS == 3
          ,
          domainOffset[2] + cellSize[2] * k
#endif
        };
        const tarch::la::Vector<DIMENSIONS, double> curvilinearPoint = curvi(pointPos);

        // The lower face moves to Lift, and the cells divide the remaining extent evenly.
        const double stretchedCellSize = (domainSize[0] - Lift) / numElements;
        validateNumericalEquals(curvilinearPoint[0], (domainOffset[0] + Lift + i * stretchedCellSize));

        // The transverse coordinates stay unchanged.
        validateNumericalEquals(curvilinearPoint[1], pointPos[1]);

        validateNumericalVectorEquals(curvi.invertProjection(curvilinearPoint), pointPos);
      }
    }
  }
}
