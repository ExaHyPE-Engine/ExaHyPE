// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/Curvi.h"

#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * A lattice point of a block of cells, its projection and the inverse projection of that projection.
   */
  struct ProjectedPoint final {
    tarch::la::Vector<DIMENSIONS, int>    latticeIndex{};
    tarch::la::Vector<DIMENSIONS, double> cartesianPoint{};
    tarch::la::Vector<DIMENSIONS, double> curvilinearPoint{};
    tarch::la::Vector<DIMENSIONS, double> invertedPoint{};
  };

  /**
   * Projects every lattice point of the block of cells at the offset and projects each projection back.
   */
  std::vector<ProjectedPoint> getProjectedLatticePoints(
    const exahype2::Curvi&                       curvi,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const tarch::la::Vector<DIMENSIONS, int>&    numberOfElements
  ) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      ASSERTION(numberOfElements(axis) > 0, numberOfElements);
    }
    const auto latticePointsPerAxis = numberOfElements + tarch::la::Vector<DIMENSIONS, int>(1);

    std::vector<ProjectedPoint> projectedPoints;
    peano4::dForEach(latticePointsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) {
      ProjectedPoint& point = projectedPoints.emplace_back();
      point.latticeIndex    = latticeIndex;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        point.cartesianPoint(axis) = domainOffset(axis) + cellSize(axis) * latticeIndex(axis);
      }
      point.curvilinearPoint = curvi(point.cartesianPoint);
      point.invertedPoint    = curvi.invertProjection(point.curvilinearPoint);
    });
    return projectedPoints;
  }
} // namespace

UNIT_TEST("Splits and a face perturbation move the lattice points linearly and the inverse projection returns them") {
  GIVEN("A block of 5, 7 and 9 cells along the axes at the offset (-1, 0, 1) with the size (1, 10, 100)") {
    const tarch::la::Vector<DIMENSIONS, double> domainOffset     = {-1., 0., 1.};
    const tarch::la::Vector<DIMENSIONS, double> domainSize       = {1., 10., 100.};
    const tarch::la::Vector<DIMENSIONS, int>    numberOfElements = {5, 7, 9};
    const tarch::la::Vector<DIMENSIONS, double> cellSize         = {
      domainSize[0] / numberOfElements[0],
      domainSize[1] / numberOfElements[1]
#if DIMENSIONS == 3
      ,
      domainSize[2] / numberOfElements[2]
#endif
    };
    exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

    WHEN("Curvi splits the first and in 3D the third axis in the middle and lifts the upper face of the second axis") {
      const exahype2::Curvi::Perturbation perturbationX =
        [domainSize]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double> x) { return domainSize[0] / 20; };
      const double splitPositionX = curvi.split(0, domainOffset[0] + 0.5 * domainSize[0], perturbationX);

      const exahype2::Curvi::Perturbation perturbationY =
        [domainSize]([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double> x) { return domainSize[1] / 10; };
      curvi.applyPerturbation(1 + DIMENSIONS, perturbationY);

#if DIMENSIONS == 3
      const exahype2::Curvi::Perturbation perturbationZ = [](const tarch::la::Vector<DIMENSIONS, double> x) {
        return 0.1 * (x[0] + x[1]);
      };
      const double splitPositionZ = curvi.split(2, domainOffset[2] + 0.5 * domainSize[2], perturbationZ);
#endif

      AND_WHEN("Curvi maps every lattice point and maps the result back") {
        const std::vector<ProjectedPoint> projectedPoints =
          getProjectedLatticePoints(curvi, domainOffset, cellSize, numberOfElements);

        THEN("Every split lands on a cell boundary and every lattice point moves linearly and maps back") {
          const int leftElements = static_cast<int>(std::round(static_cast<double>(numberOfElements[0]) * 0.5));
          VALIDATE_NUMERICAL_EQUALS(splitPositionX, (domainOffset[0] + cellSize[0] * leftElements));

#if DIMENSIONS == 3
          const int backElements = static_cast<int>(std::round(static_cast<double>(numberOfElements[2]) * 0.5));
          VALIDATE_NUMERICAL_EQUALS(splitPositionZ, (domainOffset[2] + cellSize[2] * backElements));
#endif

          const double leftCellSize  = (1.1 * domainSize[0] / 2) / leftElements;
          const double rightCellSize = (0.9 * domainSize[0] / 2) / (numberOfElements[0] - leftElements);
          for (const ProjectedPoint& point : projectedPoints) {
            const tarch::la::Vector<DIMENSIONS, int>&    latticeIndex     = point.latticeIndex;
            const tarch::la::Vector<DIMENSIONS, double>& curvilinearPoint = point.curvilinearPoint;
            if (latticeIndex(0) <= leftElements) {
              VALIDATE_NUMERICAL_EQUALS(
                curvilinearPoint[0],
                (domainOffset[0] + latticeIndex(0) * leftCellSize),
                latticeIndex
              );
            } else {
              VALIDATE_NUMERICAL_EQUALS(
                curvilinearPoint[0],
                (domainOffset[0] + 1.1 * 0.5 * domainSize[0] + (latticeIndex(0) - leftElements) * rightCellSize),
                latticeIndex
              );
            }

            // The perturbation of the upper face stretches the second axis by a tenth.
            VALIDATE_NUMERICAL_EQUALS(
              curvilinearPoint[1],
              (domainOffset[1] + 1.1 * latticeIndex(1) * cellSize[1]),
              latticeIndex
            );

#if DIMENSIONS == 3
            // The interface moves by a tenth of the sum of the two transverse coordinates.
            const double perturbation = 0.1 * (curvilinearPoint[0] + curvilinearPoint[1]);
            if (latticeIndex(2) <= backElements) {
              const double backCellSize = (0.5 * domainSize[2] + perturbation) / backElements;
              VALIDATE_NUMERICAL_EQUALS(
                curvilinearPoint[2],
                (domainOffset[2] + latticeIndex(2) * backCellSize),
                latticeIndex
              );
            } else {
              const double backCellSize = (0.5 * domainSize[2] - perturbation) / (numberOfElements[2] - backElements);
              VALIDATE_NUMERICAL_EQUALS(
                curvilinearPoint[2],
                (domainOffset[2] + 0.5 * domainSize[2] + perturbation
                 + (latticeIndex(2) - backElements) * backCellSize),
                latticeIndex
              );
            }
#endif

            VALIDATE(
              tarch::la::equals(point.invertedPoint, point.cartesianPoint),
              point.invertedPoint,
              point.cartesianPoint
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The round trip returns every point for a face perturbation that varies along an axis a split stretched") {
  GIVEN("A block of five cells per axis at the origin with the size ten") {
    constexpr int                               NumberOfElements = 5;
    const tarch::la::Vector<DIMENSIONS, double> domainOffset     = {0., 0., 0.};
    const tarch::la::Vector<DIMENSIONS, double> domainSize       = {10., 10., 10.};
    const tarch::la::Vector<DIMENSIONS, double> cellSize         = {
      domainSize[0] / NumberOfElements,
      domainSize[1] / NumberOfElements
#if DIMENSIONS == 3
      ,
      domainSize[2] / NumberOfElements
#endif
    };
    exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

    WHEN("Curvi splits the first axis and lifts the upper face of the second axis with the first coordinate") {
      curvi.split(0, 4.0, []([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double> x) { return 1.0; });

      // The face index 1 + DIMENSIONS denotes the upper face along the second axis.
      curvi.applyPerturbation(1 + DIMENSIONS, [](const tarch::la::Vector<DIMENSIONS, double> x) {
        return 0.05 * x[0];
      });

      AND_WHEN("Curvi maps every lattice point and maps the result back") {
        const std::vector<ProjectedPoint> projectedPoints = getProjectedLatticePoints(
          curvi,
          domainOffset,
          cellSize,
          tarch::la::Vector<DIMENSIONS, int>(NumberOfElements)
        );

        THEN("The inverse projection returns every lattice point") {
          for (const ProjectedPoint& point : projectedPoints) {
            VALIDATE(
              tarch::la::equals(point.invertedPoint, point.cartesianPoint),
              point.invertedPoint,
              point.cartesianPoint
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A lower face moves where its perturbation puts it and the round trip returns every point") {
  GIVEN("A block of five cells per axis at the origin with the size ten and the lift two") {
    constexpr int                               NumberOfElements = 5;
    constexpr double                            Lift             = 2.0;
    const tarch::la::Vector<DIMENSIONS, double> domainOffset     = {0., 0., 0.};
    const tarch::la::Vector<DIMENSIONS, double> domainSize       = {10., 10., 10.};
    const tarch::la::Vector<DIMENSIONS, double> cellSize         = {
      domainSize[0] / NumberOfElements,
      domainSize[1] / NumberOfElements
#if DIMENSIONS == 3
      ,
      domainSize[2] / NumberOfElements
#endif
    };
    exahype2::Curvi curvi(domainOffset, domainSize, cellSize);

    WHEN("The lower face of the first axis rises by the lift") {
      // A face index below DIMENSIONS denotes the lower face along that axis.
      curvi.applyPerturbation(0, []([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double> x) { return Lift; });

      AND_WHEN("Curvi maps every lattice point and maps the result back") {
        const std::vector<ProjectedPoint> projectedPoints = getProjectedLatticePoints(
          curvi,
          domainOffset,
          cellSize,
          tarch::la::Vector<DIMENSIONS, int>(NumberOfElements)
        );

        THEN("The projection spaces the first axis evenly from the lift, keeps the second and returns every point") {
          // The lower face sits at Lift. The elements divide the remaining extent evenly.
          const double stretchedCellSize = (domainSize[0] - Lift) / NumberOfElements;
          for (const ProjectedPoint& point : projectedPoints) {
            VALIDATE_NUMERICAL_EQUALS(
              point.curvilinearPoint[0],
              (domainOffset[0] + Lift + point.latticeIndex(0) * stretchedCellSize),
              point.latticeIndex
            );
            VALIDATE_NUMERICAL_EQUALS(point.curvilinearPoint[1], point.cartesianPoint[1], point.latticeIndex);
            VALIDATE(
              tarch::la::equals(point.invertedPoint, point.cartesianPoint),
              point.invertedPoint,
              point.cartesianPoint
            );
          }
        }
      }
    }
  }
}
