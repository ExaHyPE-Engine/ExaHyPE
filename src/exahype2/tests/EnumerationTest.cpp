// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnumerationTest.h"

#include "exahype2/Enumeration.h"
#include "peano4/Loop.h"

exahype2::tests::EnumerationTest::EnumerationTest() :
  TestCase("exahype2::tests::EnumerationTest") {}

void exahype2::tests::EnumerationTest::run() { testMethod(testSerialiseVoxelIndexInOverlap); }

void exahype2::tests::EnumerationTest::testSerialiseVoxelIndexInOverlap() {
  for (int overlap = 1; overlap <= 2; overlap++) {
    for (int numberOfDofsPerAxis = 2; numberOfDofsPerAxis <= 4; numberOfDofsPerAxis++) {
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        // The overlap layer is a box with extent 2*overlap along the normal and
        // numberOfDofsPerAxis along every other axis, enumerated lexicographically
        // with axis 0 running fastest.
        tarch::la::Vector<DIMENSIONS, int> range;
        int                                totalSize = 1;
        for (int d = 0; d < DIMENSIONS; d++) {
          range(d) = (d == normal) ? 2 * overlap : numberOfDofsPerAxis;
          totalSize *= range(d);
        }

        std::set<int> visited;
        peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& overlapCell) {
          const auto index = exahype2::serialiseVoxelIndexInOverlap(overlapCell, numberOfDofsPerAxis, overlap, normal);
          int expected = 0;
          int base     = 1;
          for (int d = 0; d < DIMENSIONS; d++) {
            expected += overlapCell(d) * base;
            base *= range(d);
          }
          validateEqualsWithParams2(index, expected, overlapCell, normal);
          validateWithParams1(visited.find(index) == visited.end(), overlapCell);
          visited.insert(index);
        });
        validateEqualsWithParams2(static_cast<int>(visited.size()), totalSize, numberOfDofsPerAxis, overlap);
      }
    }
  }
}
