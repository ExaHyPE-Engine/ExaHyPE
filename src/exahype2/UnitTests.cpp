// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "config.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#if defined(GPUOffloadingCUDA)
#include "exahype2/tests/KernelLaunchTest.h"
#endif

#include "exahype2/dg/rusanov/tests/RiemannTest.h"
#include "exahype2/dg/tests/CellIntegralTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/fd/tests/SommerfeldBCTest.h"
#include "exahype2/fd/tests/CCZ4KernelTest.h"
#include "exahype2/fv/rusanov/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/rusanov/tests/CopyPatchTest.h"
#include "exahype2/fv/tests/InterpolationRestrictionTest.h"

tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

#if defined(GPUOffloadingCUDA)
  result->addTestCase(new exahype2::tests::KernelLaunchTest());
#endif

  // result->addTestCase( new exahype2::dg::tests::CellIntegralTest() );
  // result->addTestCase( new exahype2::dg::tests::DGUtilsTest() );
  // result->addTestCase( new exahype2::dg::tests::RiemannTest() );
  // result->addTestCase( new exahype2::dg::rusanov::tests::RiemannTest() );
  result->addTestCase(new exahype2::fd::tests::SommerfeldBCTest());
  //result->addTestCase(new exahype2::fd::tests::CCZ4KernelTest());
  // result->addTestCase(new exahype2::fv::tests::InterpolationRestrictionTest());
  result->addTestCase(new exahype2::fv::rusanov::tests::CopyPatchTest());
  result->addTestCase(new exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest());
  // result->addTestCase( new exahype2::aderdg::tests::ADERDGTest() );

  return result;
}
