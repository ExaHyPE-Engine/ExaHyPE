// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTopologyTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "tarch/la/Vector.h"

peano4::grid::tests::StackTopologyTest::StackTopologyTest() :
  TestCase("peano4::grid::tests::StackTopologyTest") {}

void peano4::grid::tests::StackTopologyTest::run() {
  testMethod(testStackIdentification);
  testMethod(testPeriodicBoundaryMapping);
  testMethod(testPeriodicPartnerRoundTrip);
  testMethod(testGetPeriodicBoundaryNumber);
  testMethod(testBitsetBasedPeriodicBoundaryEnumeration);
  testMethod(testGetOutputStacksForPeriodicBoundaryExchange);
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

void peano4::grid::tests::StackTopologyTest::testGetPeriodicBoundaryNumber() {
#if DIMENSIONS == 2
  std::array<int, TWO_POWER_D> flags;
  flags.fill(1);
  flags[0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  flags[1] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  std::bitset<2 * DIMENSIONS> result = StackTopology::getPeriodicBoundaryNumber(flags);
  validateEquals(result.to_ulong(), 2);
#endif
}

void peano4::grid::tests::StackTopologyTest::testBitsetBasedPeriodicBoundaryEnumeration() {
  auto getPeriodicBoundaryNumberViaDforEnumeration =
    [](const std::array<int, TWO_POWER_D>& flags) -> std::bitset<2 * DIMENSIONS> {
    std::bitset<2 * DIMENSIONS> result;

    for (int d = 0; d < 2 * DIMENSIONS; d++) {
      bool allFlagsSet = true;
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
        tarch::la::Vector<DIMENSIONS, int> entry;
        entry                 = k;
        entry(d % DIMENSIONS) = d / DIMENSIONS;
        allFlagsSet &= flags[peano4::dLinearised(entry, 2)] == SpacetreeHelper::RankOfPeriodicBoundaryCondition;
      });
      if (allFlagsSet) {
        result.set(d);
      }
    }

    return result;
  };

  auto getPeriodicBoundaryNumberViaExplicitBitsetLoop =
    [](const std::array<int, TWO_POWER_D>& flags) -> std::bitset<2 * DIMENSIONS> {
    std::bitset<2 * DIMENSIONS> result;

    for (int d = 0; d < 2 * DIMENSIONS; d++) {
      bool allFlagsSet = true;
      for (int i = 0; i < TWO_POWER_D; i++) {
        std::bitset<DIMENSIONS>            point(static_cast<unsigned long>(i));
        tarch::la::Vector<DIMENSIONS, int> entry(point);
        entry(d % DIMENSIONS) = d / DIMENSIONS;

        const bool current = flags[peano4::dLinearised(entry, 2)] == SpacetreeHelper::RankOfPeriodicBoundaryCondition;
        allFlagsSet        = allFlagsSet and current;
      }
      if (allFlagsSet) {
        result.set(d);
      }
    }

    return result;
  };

  std::array<int, TWO_POWER_D> topRightFlags;
  topRightFlags.fill(SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  topRightFlags[0] = 0;

  const auto topRightDFor     = getPeriodicBoundaryNumberViaDforEnumeration(topRightFlags);
  const auto topRightBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(topRightFlags);
  const auto topRightTopology = StackTopology::getPeriodicBoundaryNumber(topRightFlags);

  validateEquals(topRightBitset.to_ulong(), topRightDFor.to_ulong());
  validateEquals(topRightTopology.to_ulong(), topRightDFor.to_ulong());

  std::array<int, TWO_POWER_D> periodicFaceFlags;
  periodicFaceFlags.fill(1);
  periodicFaceFlags[0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  periodicFaceFlags[1] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  const auto periodicFaceDFor     = getPeriodicBoundaryNumberViaDforEnumeration(periodicFaceFlags);
  const auto periodicFaceBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(periodicFaceFlags);
  const auto periodicFaceTopology = StackTopology::getPeriodicBoundaryNumber(periodicFaceFlags);

  validateEquals(periodicFaceBitset.to_ulong(), periodicFaceDFor.to_ulong());
  validateEquals(periodicFaceTopology.to_ulong(), periodicFaceDFor.to_ulong());

  std::array<int, TWO_POWER_D> allPeriodicFlags;
  allPeriodicFlags.fill(SpacetreeHelper::RankOfPeriodicBoundaryCondition);

  const auto allPeriodicDFor     = getPeriodicBoundaryNumberViaDforEnumeration(allPeriodicFlags);
  const auto allPeriodicBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(allPeriodicFlags);
  const auto allPeriodicTopology = StackTopology::getPeriodicBoundaryNumber(allPeriodicFlags);

  validateEquals(allPeriodicBitset.to_ulong(), allPeriodicDFor.to_ulong());
  validateEquals(allPeriodicTopology.to_ulong(), allPeriodicDFor.to_ulong());
}

/**
 * Test axial and diagonal periodic exchanges at opposite two-dimensional
 * corners. The top-right output/input pairs are (7,15), (8,16), and (10,18),
 * while the bottom-left pairs are (11,19), (13,21), and (14,22). Mirroring
 * then maps outputs 7, 8, 10, 11, 13, and 14 onto 22, 21, 19, 18, 16, and 15.
 */
void peano4::grid::tests::StackTopologyTest::testGetOutputStacksForPeriodicBoundaryExchange() {
#if DIMENSIONS == 2
  std::array<int, TWO_POWER_D> flagsTopRightVertex;
  flagsTopRightVertex.fill(SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsTopRightVertex[0] = 0;

  std::set<StackTopology::PeriodicBoundaryStackIdentifier> resultTopRightVertex =
    StackTopology::getOutputStacksForPeriodicBoundaryExchange(flagsTopRightVertex);
  validateEquals(resultTopRightVertex.size(), 3);

  StackTopology::PeriodicBoundaryStackIdentifier key0(7, 3);
  StackTopology::PeriodicBoundaryStackIdentifier key1(8, 2);
  StackTopology::PeriodicBoundaryStackIdentifier key2(10, 1);

  std::stringstream resultContentTopRightVertex;
  for (auto& p : resultTopRightVertex) {
    resultContentTopRightVertex << "(" << p.first << "," << std::bitset<2 * DIMENSIONS>(p.second) << ")";
  }
  validateWithParams3(
    resultTopRightVertex.count(key0) == 1,
    key0.first,
    key0.second,
    resultContentTopRightVertex.str()
  );
  validateWithParams3(
    resultTopRightVertex.count(key1) == 1,
    key1.first,
    key1.second,
    resultContentTopRightVertex.str()
  );
  validateWithParams3(
    resultTopRightVertex.count(key2) == 1,
    key2.first,
    key2.second,
    resultContentTopRightVertex.str()
  );

  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key0.first));
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key1.first));
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key2.first));

  std::array<int, TWO_POWER_D> flagsBottomLeftVertex;
  flagsBottomLeftVertex.fill(SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsBottomLeftVertex[3] = 0;

  std::set<StackTopology::PeriodicBoundaryStackIdentifier> resultBottomLeftVertex =
    StackTopology::getOutputStacksForPeriodicBoundaryExchange(flagsBottomLeftVertex);
  validateEquals(resultBottomLeftVertex.size(), 3);

  StackTopology::PeriodicBoundaryStackIdentifier key3(11, 1);
  StackTopology::PeriodicBoundaryStackIdentifier key4(13, 2);
  StackTopology::PeriodicBoundaryStackIdentifier key5(14, 3);

  std::stringstream resultContentBottomLeftVertex;
  for (auto& p : resultBottomLeftVertex) {
    resultContentBottomLeftVertex << "(" << p.first << "," << std::bitset<2 * DIMENSIONS>(p.second) << ")";
  }
  validateWithParams3(
    resultBottomLeftVertex.count(key3) == 1,
    key3.first,
    key3.second,
    resultContentBottomLeftVertex.str()
  );
  validateWithParams3(
    resultBottomLeftVertex.count(key4) == 1,
    key4.first,
    key4.second,
    resultContentBottomLeftVertex.str()
  );
  validateWithParams3(
    resultBottomLeftVertex.count(key5) == 1,
    key5.first,
    key5.second,
    resultContentBottomLeftVertex.str()
  );

  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key3.first));
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key4.first));
  validate(StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key5.first));

  validateEqualsWithParams2(
    StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key0.first),
    15,
    key0.first,
    key0.second
  );
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key1.first), 16);
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key2.first), 18);

  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key3.first), 19);
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key4.first), 21);
  validateEquals(StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key5.first), 22);

  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(7), 22);
  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(8), 21);
  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(10), 19);
  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(11), 18);
  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(13), 16);
  validateEquals(StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(14), 15);
#endif
#if DIMENSIONS == 3
  std::array<int, TWO_POWER_D> flags;
  flags.fill(SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flags[0] = 0;

  std::set<StackTopology::PeriodicBoundaryStackIdentifier> result =
    StackTopology::getOutputStacksForPeriodicBoundaryExchange(flags);
  validateEqualsWithParams1(
    result.size(),
    7,
    "expected output={(stack=9,bnd=111),(stack=10,bnd=110),(stack=12,bnd=101),(stack=13,bnd=100),"
    "(stack=18,bnd=011),(stack=19,bnd=010),(stack=21,bnd=001)}"
  );
#endif
}
