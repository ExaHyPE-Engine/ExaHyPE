// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::amr::tests {
  class RestrictionTest;
} // namespace exahype2::amr::tests

class exahype2::amr::tests::RestrictionTest final : public tarch::tests::TestCase {
public:
  RestrictionTest();
  ~RestrictionTest() override = default;

  void run() override;

private:
  void testRestrictHaloLayer();
  void testRestrictCell();
  void testRestrictCellWithMatrixReproducesPolynomials();
  void testOrderInvariantProduct();
  void testRestrictCellFromChildrenReproducesRamp();
  void testRestrictCellFromChildrenWithMatrixReproducesPolynomials();
  void testRestrictCellFromChildrenIsSymmetryEquivariant();
  void testApplyTensorProductOrderInvariant();
  void testInterpolateFaceFromCoarse();
  void testRestrictFaceFromChildren();
  void testGatheredHaloRestrictionAveragesTheSubfaceRamp();
  void testGatheredHaloRestrictionIsStagingOrderInvariant();
  void testGatheredHaloRestrictionScalesAPartialSubfaceSet();
  void testGatheredHaloRestrictionRunsInSinglePrecision();
};
