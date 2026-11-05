// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/Loop.h"

#include "peano4/Constants.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Returns the sum over the axes of the entry of point times extent to the power of the axis.
   */
  int getIndexWithFirstAxisFastest(const tarch::la::Vector<DIMENSIONS, int>& point, const int extent) {
    int index  = 0;
    int factor = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      index += point(axis) * factor;
      factor *= extent;
    }
    return index;
  }
} // namespace

UNIT_TEST("A loop over a cube visits every point once in linearised order, with the first axis running fastest") {
  GIVEN("The extents from zero to five") {
    constexpr int SmallestExtent = 0;
    constexpr int LargestExtent  = 5;

    WHEN("A loop over the cube of every extent lists the points it visits") {
      std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>> pointsOfExtent;
      for (int extent = SmallestExtent; extent <= LargestExtent; extent++) {
        std::vector<tarch::la::Vector<DIMENSIONS, int>> points;
        peano4::dForEach(extent, [&points](const tarch::la::Vector<DIMENSIONS, int>& point) {
          points.push_back(point);
        });
        pointsOfExtent.push_back(points);
      }

      THEN("Every loop visits the points of its cube in linearised order, one visit per point of the cube") {
        for (int extent = SmallestExtent; extent <= LargestExtent; extent++) {
          const auto& points = pointsOfExtent[static_cast<std::size_t>(extent - SmallestExtent)];
          for (std::size_t visit = 0; visit < points.size(); visit++) {
            VALIDATE_EQUALS(
              getIndexWithFirstAxisFastest(points[visit], extent),
              static_cast<int>(visit),
              points[visit]
            );
          }
          VALIDATE_EQUALS(static_cast<int>(points.size()), peano4::power(extent, DIMENSIONS));
        }
      }
    }
  }
}

UNIT_TEST("A loop over a box with one extent per axis visits every point once, also with an axis of extent one") {
  GIVEN("The boxes of extent two plus the axis number per axis with zero or one flat axis and their sizes") {
    std::vector<tarch::la::Vector<DIMENSIONS, int>> boxes;
    std::vector<long>                               numberOfPointsOfBox;
    for (int flatAxis = -1; flatAxis < DIMENSIONS; flatAxis++) {
      tarch::la::Vector<DIMENSIONS, int> box;
      long                               numberOfPoints = 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        box(axis) = (axis == flatAxis) ? 1 : (2 + axis);
        numberOfPoints *= box(axis);
      }
      boxes.push_back(box);
      numberOfPointsOfBox.push_back(numberOfPoints);
    }

    WHEN("A loop over every box lists the points it visits") {
      std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>> pointsOfBox;
      for (const auto& box : boxes) {
        std::vector<tarch::la::Vector<DIMENSIONS, int>> points;
        peano4::dForEach(box, [&points](const tarch::la::Vector<DIMENSIONS, int>& point) { points.push_back(point); });
        pointsOfBox.push_back(points);
      }

      THEN("Every loop visits the points of its box in linearised order, one visit per point of the box") {
        for (std::size_t boxNumber = 0; boxNumber < boxes.size(); boxNumber++) {
          const auto& box    = boxes[boxNumber];
          const auto& points = pointsOfBox[boxNumber];
          for (std::size_t visit = 0; visit < points.size(); visit++) {
            long remainder = static_cast<long>(visit);
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              VALIDATE_EQUALS(points[visit](axis), static_cast<int>(remainder % box(axis)), points[visit], box);
              remainder /= box(axis);
            }
          }
          VALIDATE_EQUALS(static_cast<long>(points.size()), numberOfPointsOfBox[boxNumber], box);
        }
      }
    }
  }
}

UNIT_TEST("The linearised index of a point sums its entries times the extent to the power of their axis") {
  GIVEN("The extent four") {
    constexpr int Extent = 4;

    WHEN("The test linearises every point of the cube of this extent") {
      std::vector<tarch::la::Vector<DIMENSIONS, int>> points;
      std::vector<int>                                indices;
      peano4::dForEach(Extent, [&points, &indices](const tarch::la::Vector<DIMENSIONS, int>& point) {
        points.push_back(point);
        indices.push_back(peano4::dLinearised(point, Extent));
      });

      THEN("Every index equals the sum of the entries of its point times the powers of the extent") {
        for (std::size_t visit = 0; visit < points.size(); visit++) {
          VALIDATE_EQUALS(indices[visit], getIndexWithFirstAxisFastest(points[visit], Extent), points[visit]);
        }
      }
    }
  }
}

UNIT_TEST("The delinearisation of the linearised index of a point returns the point") {
  GIVEN("The extents from one to four") {
    constexpr int SmallestExtent = 1;
    constexpr int LargestExtent  = 4;

    WHEN("The test linearises and delinearises every point of the cube of every extent") {
      std::vector<int>                                extentOfVisit;
      std::vector<tarch::la::Vector<DIMENSIONS, int>> pointOfVisit;
      std::vector<int>                                indexOfVisit;
      std::vector<tarch::la::Vector<DIMENSIONS, int>> restoredPointOfVisit;
      for (int extent = SmallestExtent; extent <= LargestExtent; extent++) {
        peano4::dForEach(extent, [&](const tarch::la::Vector<DIMENSIONS, int>& point) {
          const int index = peano4::dLinearised(point, extent);
          extentOfVisit.push_back(extent);
          pointOfVisit.push_back(point);
          indexOfVisit.push_back(index);
          restoredPointOfVisit.push_back(peano4::dDelinearised(index, extent));
        });
      }

      THEN("Every restored point equals its point") {
        for (std::size_t visit = 0; visit < pointOfVisit.size(); visit++) {
          VALIDATE_EQUALS(restoredPointOfVisit[visit], pointOfVisit[visit], extentOfVisit[visit], indexOfVisit[visit]);
        }
      }
    }
  }
}

UNIT_TEST("The exclusive delinearisation enumerates the points of a face, with the entry of the fixed axis at zero") {
  GIVEN("The extent three and the number of points of a face of the cube of this extent") {
    constexpr int Extent        = 3;
    constexpr int PointsPerFace = peano4::power(Extent, DIMENSIONS - 1);

    WHEN("The test delinearises every index of a face with every axis as the fixed axis") {
      std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>> pointsOfFixedAxis(DIMENSIONS);
      for (int fixedAxis = 0; fixedAxis < DIMENSIONS; fixedAxis++) {
        for (int index = 0; index < PointsPerFace; index++) {
          pointsOfFixedAxis[fixedAxis].push_back(peano4::dDelinearisedExclusive(index, Extent, fixedAxis));
        }
      }

      THEN("The delinearisation returns distinct points of the cube per face with the entry zero on the fixed axis") {
        for (int fixedAxis = 0; fixedAxis < DIMENSIONS; fixedAxis++) {
          std::set<std::vector<int>> visited;
          for (int index = 0; index < PointsPerFace; index++) {
            const auto& point = pointsOfFixedAxis[fixedAxis][index];
            VALIDATE_EQUALS(point(fixedAxis), 0, index, fixedAxis);
            std::vector<int> entries;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              VALIDATE(point(axis) >= 0 and point(axis) < Extent);
              entries.push_back(point(axis));
            }
            visited.insert(entries);
          }
          VALIDATE_EQUALS(static_cast<int>(visited.size()), PointsPerFace, fixedAxis, Extent);
        }
      }
    }
  }
}

UNIT_TEST("The exclusive linearisation returns the index of a point of a face for every value of the fixed axis") {
  GIVEN("The extent three and the number of points of a face of the cube of this extent") {
    constexpr int Extent        = 3;
    constexpr int PointsPerFace = peano4::power(Extent, DIMENSIONS - 1);

    WHEN("The test linearises every point of a face as delinearised and with the last position on the fixed axis") {
      std::vector<std::vector<int>> indexOfPoint(DIMENSIONS);
      std::vector<std::vector<int>> indexOfPointAtTheLastPosition(DIMENSIONS);
      for (int fixedAxis = 0; fixedAxis < DIMENSIONS; fixedAxis++) {
        for (int index = 0; index < PointsPerFace; index++) {
          auto point = peano4::dDelinearisedExclusive(index, Extent, fixedAxis);
          indexOfPoint[fixedAxis].push_back(peano4::dLinearisedExclusive(point, Extent, fixedAxis));
          point(fixedAxis) = Extent - 1;
          indexOfPointAtTheLastPosition[fixedAxis].push_back(peano4::dLinearisedExclusive(point, Extent, fixedAxis));
        }
      }

      THEN("Both linearisations return the index from which the delinearisation derived the point") {
        for (int fixedAxis = 0; fixedAxis < DIMENSIONS; fixedAxis++) {
          for (int index = 0; index < PointsPerFace; index++) {
            VALIDATE_EQUALS(indexOfPoint[fixedAxis][index], index, fixedAxis, Extent);
            VALIDATE_EQUALS(indexOfPointAtTheLastPosition[fixedAxis][index], index, fixedAxis, Extent);
          }
        }
      }
    }
  }
}

UNIT_TEST("A loop over the corners of a cube visits every corner once, with its linearised index as scalar") {
  GIVEN("The number of corners of a cube") {
    constexpr int NumberOfCorners = peano4::VerticesPerCell;

    WHEN("A loop over the corners lists every corner and its scalar argument") {
      std::vector<tarch::la::Vector<DIMENSIONS, int>> corners;
      std::vector<int>                                scalarArguments;
      peano4::dForEachCorner(
        [&corners, &scalarArguments](const tarch::la::Vector<DIMENSIONS, int>& corner, const int scalarArgument) {
          corners.push_back(corner);
          scalarArguments.push_back(scalarArgument);
        }
      );

      THEN("The loop passes every corner once with the entries zero and one and its linearised index as scalar") {
        std::set<int> visited;
        for (std::size_t visit = 0; visit < corners.size(); visit++) {
          const auto& corner = corners[visit];
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE(corner(axis) == 0 or corner(axis) == 1, corner);
          }
          VALIDATE_EQUALS(scalarArguments[visit], peano4::dLinearised(corner, 2), corner);
          visited.insert(scalarArguments[visit]);
        }
        VALIDATE_EQUALS(static_cast<int>(corners.size()), NumberOfCorners);
        VALIDATE_EQUALS(static_cast<int>(visited.size()), NumberOfCorners);
      }
    }
  }
}
