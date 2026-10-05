// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "peano4/communication/tests/CommunicationManagerTest.h"
#include "peano4/communication/tests/HaloExchangeKernelsTest.h"
#include "peano4/communication/tests/MigrationExchangeKernelsTest.h"
#include "peano4/grid/tests/AutomatonStateTest.h"
#include "peano4/grid/tests/GridVertexTest.h"
#include "peano4/grid/tests/LatticeIndexMapTest.h"
#include "peano4/sfc/tests/GrammarTest.h"
#include "peano4/sfc/tests/SpaceFillingCurveTest.h"
#include "peano4/tests/HaloExchangeTest.h"
#include "peano4/tests/LoopTest.h"
#include "peano4/tests/MigrationExchangeTest.h"
#include "peano4/tests/PartitionTest.h"
#include "peano4/tests/RefinementRequestTest.h"
#include "peano4/tests/SpacetreeTest.h"
#include "peano4/topology/tests/ChunkedTasksTest.h"
#include "peano4/topology/tests/TreeTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase*
peano4::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new peano4::tests::LoopTest());
  result->addTestCase(new peano4::tests::RefinementRequestTest());
  result->addTestCase(new peano4::tests::SpacetreeTest());
  result->addTestCase(new peano4::tests::PartitionTest());
  result->addTestCase(new peano4::tests::HaloExchangeTest());
  result->addTestCase(new peano4::tests::MigrationExchangeTest());

  result->addTestCase(new peano4::grid::tests::AutomatonStateTest());
  result->addTestCase(new peano4::grid::tests::GridVertexTest());
  result->addTestCase(new peano4::grid::tests::LatticeIndexMapTest());

  result->addTestCase(new peano4::communication::tests::CommunicationManagerTest());
  result->addTestCase(new peano4::communication::tests::HaloExchangeKernelsTest());
  result->addTestCase(new peano4::communication::tests::MigrationExchangeKernelsTest());

  result->addTestCase(new peano4::sfc::tests::GrammarTest());
  result->addTestCase(new peano4::sfc::tests::SpaceFillingCurveTest());

  result->addTestCase(new peano4::topology::tests::ChunkedTasksTest());
  result->addTestCase(new peano4::topology::tests::TreeTest());

  return result;
}
