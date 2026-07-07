// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopologyTest.h"

#include "peano4/grid/StackTopology.h"

peano4::grid::tests::StackTopologyTest::StackTopologyTest() :
  TestCase("peano4::grid::tests::StackTopologyTest") {}

void peano4::grid::tests::StackTopologyTest::run() {
  testMethod(testStackIdentification);
  testMethod(testPeriodicBoundaryMapping);
  testMethod(testPeriodicPartnerRoundTrip);
}

void peano4::grid::tests::StackTopologyTest::testStackIdentification() {
  validate(not StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Call)));
  validate(StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Input)));
  validate(StackTopology::isInOutStack(static_cast<int>(StackTopology::BaseStack::Output)));

  const int firstFaceStack = StackTopology::FaceExchangeStackOffset;
  validate(StackTopology::isFaceExchangeStack(firstFaceStack));
  validate(StackTopology::isFaceExchangeStack(firstFaceStack + 1));
  validate(not StackTopology::isFaceExchangeStack(firstFaceStack - 1));

  const int lastLocalStack = StackTopology::TotalNumberOfLocalExchangeStacks - 1;
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
  const int outStack        = start;
  const int expectedInStack = start + half;
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(outStack), expectedInStack);

  // Verify the mirror mapping: it maps the first output to the last input.
  const int mirroredInput    = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outStack);
  const int expectedMirrored = start + totalPeriodic - 1;
  validateEquals(mirroredInput, expectedMirrored);

  // Test the last output stack
  const int lastOutStack = start + half - 1;
  const int mirroredLast = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(lastOutStack);
  // Should map to the start of the input range (which is 'half' away from start)
  validateEquals(mirroredLast, start + half);
}

/**
 * The periodic exchange deposits an entry pushed to output stack s onto the
 * partner tree's getPeriodicBoundaryExchangeInputStackNumberForOutputStack(s),
 * while the partner reads its own mirrored direction via
 * mapPeriodicBoundaryExchangeOutputStackOntoInputStack. These two maps are
 * deliberately non-equivalent and only compose across mirrored directions,
 * so unifying them would silently break the exchange. Pinned for every
 * stencil direction, faces and corners alike.
 */
void peano4::grid::tests::StackTopologyTest::testPeriodicPartnerRoundTrip() {
  // The deposit stack for output stack s is
  // getPeriodicBoundaryExchangeInputStackNumberForOutputStack(s); the wrapped
  // partner reads from mapPeriodicBoundaryExchangeOutputStackOntoInputStack of
  // its own, mirrored direction. These must address the same stack for every
  // stencil direction (mirroring means inverting the direction, i.e. the
  // reversed linearised stencil position).
  constexpr int Centre = (THREE_POWER_D - 1) / 2;
  for (int linearised = 0; linearised < THREE_POWER_D; linearised++) {
    if (linearised == Centre) {
      continue;
    }
    const int mirroredLinearised = THREE_POWER_D - 1 - linearised;

    const int outputIndex         = linearised < Centre ? linearised : linearised - 1;
    const int mirroredOutputIndex = mirroredLinearised < Centre ? mirroredLinearised : mirroredLinearised - 1;

    const int outputStack         = StackTopology::PeriodicBoundaryExchangeStackOffset + outputIndex;
    const int mirroredOutputStack = StackTopology::PeriodicBoundaryExchangeStackOffset + mirroredOutputIndex;

    validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(outputStack));
    validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(mirroredOutputStack));

    const int depositStack = StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(outputStack);
    const int readStack    = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(mirroredOutputStack);
    validateEqualsWithParams2(depositStack, readStack, linearised, mirroredLinearised);
  }

  // Cross-check via the face-number API: face f and its geometric opposite
  // f + DIMENSIONS use exactly these mirrored directions.
  for (int f = 0; f < DIMENSIONS; f++) {
    const int outF         = StackTopology::getOutputStackForPeriodicBoundaryExchange(f);
    const int outFOpposite = StackTopology::getOutputStackForPeriodicBoundaryExchange(f + DIMENSIONS);

    validateEqualsWithParams1(
      StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(outF),
      StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outFOpposite),
      f
    );
    validateEqualsWithParams1(
      StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(outFOpposite),
      StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outF),
      f
    );
  }
}
