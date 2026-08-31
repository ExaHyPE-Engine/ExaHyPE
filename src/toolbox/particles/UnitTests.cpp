// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "toolbox/particles/assignmentchecks/tests/TestHelpers.h"
#include "toolbox/particles/tests/MultiscaleTransitionsTest.h"
#include "toolbox/particles/tests/ParticlesTest.h"
#include "toolbox/particles/tests/TestHelpers.h"
#include "toolbox/particles/tests/TrajectoryDatabaseTest.h"

tarch::tests::TestCase* toolbox::particles::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new toolbox::particles::tests::MultiscaleTransitionsTest());
  result->addTestCase(new toolbox::particles::tests::ParticlesTest());
  result->addTestCase(new toolbox::particles::tests::TestHelpers());
  result->addTestCase(new toolbox::particles::tests::TrajectoryDatabaseTest());
  result->addTestCase(new toolbox::particles::assignmentchecks::tests::TestHelpers());

  return result;
}
