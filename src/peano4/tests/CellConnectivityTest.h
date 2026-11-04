// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class CellConnectivityTest;
} // namespace peano4::tests

class peano4::tests::CellConnectivityTest final : public tarch::tests::TestCase {
public:
  CellConnectivityTest();
  CellConnectivityTest(const CellConnectivityTest& other)            = delete;
  CellConnectivityTest(CellConnectivityTest&& other)                 = delete;
  CellConnectivityTest& operator=(const CellConnectivityTest& other) = delete;
  CellConnectivityTest& operator=(CellConnectivityTest&& other)      = delete;
  ~CellConnectivityTest() override                                   = default;

  void run() override;

private:
  void testTheRegularGridLinksEveryFaceToItsCells();
  void testTheCentresOfMirrorCellsAndFacesAreNegated();
  void testEveryFaceOfThePeriodicGridIsInterior();
  void testTheRefinedBlockBuildsOneLevelJumpPerCoarseFace();
  void testTheCellCentreBesideEveryFaceMatchesItsCellBitwise();
  void testTheListsGroupByAdjacencyLevelAndKind();
  void testARegridKeepsTheSlotsOfSurvivingCellsAndReusesFreedOnes();
  void testTheExchangedFacesMatchAcrossRanks();
  void testTheCurveIndicesNumberTheCellsAlongTheCurve();
  void testTheDeviceTwinReadsTheSameGeometry();
  void testTheBytesCountEveryArray();
};
