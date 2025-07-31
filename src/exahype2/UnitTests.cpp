// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "dg/rusanov/tests/RiemannTest.h"
#include "dg/tests/CellIntegralTest.h"
#include "dg/tests/DGUtilsTest.h"
#include "dg/tests/RiemannTest.h"
#include "fd/tests/CCZ4KernelTest.h"
#include "fd/tests/SommerfeldBCTest.h"
#include "fv/tests/ApplySplit1DRiemannToPatchTest.h"
#include "fv/tests/CopyPatchTest.h"
#include "fv/tests/InterpolationRestrictionTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"
#include "tests/EnclaveBookkeepingTest.h"

tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new tests::EnclaveBookkeepingTest());

  result->addTestCase(new dg::tests::CellIntegralTest());
  result->addTestCase(new dg::tests::DGUtilsTest());
  result->addTestCase(new dg::tests::RiemannTest());
  result->addTestCase(new dg::rusanov::tests::RiemannTest());

  // result->addTestCase(new fd::tests::CCZ4KernelTest());
  result->addTestCase(new fd::tests::SommerfeldBCTest());

  result->addTestCase(new fv::tests::CopyPatchTest());
  result->addTestCase(new fv::tests::InterpolationRestrictionTest());
  result->addTestCase(new fv::tests::ApplySplit1DRiemannToPatchTest());

  return result;
}
