// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopologyTest.h"

#include "peano4/grid/StackTopology.h"

peano4::grid::tests::StackTopologyTest::StackTopologyTest():
  TestCase("peano4::grid::tests::StackTopologyTest") {}

void peano4::grid::tests::StackTopologyTest::run() {
  testMethod(testStackIdentification);
  testMethod(testPeriodicBoundaryMapping);
}

void peano4::grid::tests::StackTopologyTest::testStackIdentification() {
  validate(not StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Call)));
  validate(StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Input)));
  validate(StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Output)));

  int firstFaceStack = StackTopology::FaceExchangeStackOffset;
  validate(StackTopology::isFaceExchangeStack(firstFaceStack));
  validate(StackTopology::isFaceExchangeStack(firstFaceStack + 1));
  validate(not StackTopology::isFaceExchangeStack(firstFaceStack - 1));

  int lastLocalStack = StackTopology::TotalNumberOfLocalExchangeStacks - 1;
  validate(StackTopology::isLocalExchangeStack(0));
  validate(StackTopology::isLocalExchangeStack(lastLocalStack));
  validate(not StackTopology::isLocalExchangeStack(lastLocalStack + 1));
}

void peano4::grid::tests::StackTopologyTest::testPeriodicBoundaryMapping() {
  const int start         = StackTopology::PeriodicBoundaryExchangeStackOffset;
  const int totalPeriodic = StackTopology::TotalNumberOfPeriodicBoundaryExchangeStacks;
  const int half          = totalPeriodic / 2;

  // Verify Output vs Input range for periodic boundaries
  // First half are Outputs, Second half are Inputs
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(start));
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(start + half - 1));
  validate(not StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(start + half));

  // Verify Output -> Input mapping (Standard Exchange)
  // Input stack is offset by half the total periodic stacks
  int outStack        = start;
  int expectedInStack = start + half;
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(outStack), expectedInStack);

  // Verify Mirror Mapping (Face Exchange Logic)
  // mapPeriodicBoundaryExchangeOutputStackOntoInputStack maps the first output to the last input (mirrored)
  // mirroredStack = Total - (output - first) - 1 + first
  int mirroredInput    = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outStack);
  int expectedMirrored = start + totalPeriodic - 1;
  validateEquals(mirroredInput, expectedMirrored);

  // Test the last output stack
  int lastOutStack = start + half - 1;
  int mirroredLast = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(lastOutStack);
  // Should map to the start of the input range (which is 'half' away from start)
  validateEquals(mirroredLast, start + half);
}
