#include "NodeTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/Node.h"
#include "tarch/la/Vector.h"

peano4::parallel::tests::NodeTest::NodeTest() :
  TestCase("peano4::parallel::tests::NodeTest") {}

void peano4::parallel::tests::NodeTest::testTagCalculation() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int senderTree    = 0;
  const int receiverTree0 = 0;
  const int receiverTree1 = numberOfRanks;

  Node::GridDataExchangeMetaInformation horizontal0 =
    peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(
      senderTree,
      receiverTree0,
      peano4::parallel::Node::ExchangeMode::HorizontalData
    );
  Node::GridDataExchangeMetaInformation vertical0 =
    peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(
      senderTree,
      receiverTree0,
      peano4::parallel::Node::ExchangeMode::VerticalData
    );
  Node::GridDataExchangeMetaInformation horizontal1 =
    peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(
      senderTree,
      receiverTree1,
      peano4::parallel::Node::ExchangeMode::HorizontalData
    );
  Node::GridDataExchangeMetaInformation vertical1 =
    peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(
      senderTree,
      receiverTree1,
      peano4::parallel::Node::ExchangeMode::VerticalData
    );

  validateWithParams4(
    horizontal0.first != vertical0.first,
    horizontal0.first,
    vertical0.first,
    receiverTree0,
    numberOfRanks
  );
  validateWithParams4(
    vertical0.first != horizontal1.first,
    vertical0.first,
    horizontal1.first,
    receiverTree1,
    numberOfRanks
  );
  validateWithParams4(
    horizontal1.first != vertical1.first,
    horizontal1.first,
    vertical1.first,
    receiverTree1,
    numberOfRanks
  );
}

void peano4::parallel::tests::NodeTest::testGetOutputStacksForPeriodicBoundaryExchange() {
#if DIMENSIONS == 2
  //
  // Top right corner
  //
  std::array<int, TWO_POWER_D> flagsTopRightVertex;
  flagsTopRightVertex.fill(grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsTopRightVertex[0] = 0;

  std::set<grid::StackTopology::PeriodicBoundaryStackIdentifier> resultTopRightVertex =
    grid::StackTopology::getOutputStacksForPeriodicBoundaryExchange(flagsTopRightVertex);
  validateEquals(resultTopRightVertex.size(), 3);

  grid::StackTopology::PeriodicBoundaryStackIdentifier key0(7, 3);
  grid::StackTopology::PeriodicBoundaryStackIdentifier key1(8, 2);
  grid::StackTopology::PeriodicBoundaryStackIdentifier key2(10, 1);

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

  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key0.first));
  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key1.first));
  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key2.first));

  //
  // bottom left vertex
  //
  std::array<int, TWO_POWER_D> flagsBottomLeftVertex;
  flagsBottomLeftVertex.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsBottomLeftVertex[3] = 0;

  std::set<grid::StackTopology::PeriodicBoundaryStackIdentifier> resultBottomLeftVertex =
    grid::StackTopology::getOutputStacksForPeriodicBoundaryExchange(flagsBottomLeftVertex);
  validateEquals(resultBottomLeftVertex.size(), 3);

  grid::StackTopology::PeriodicBoundaryStackIdentifier key3(11, 1);
  grid::StackTopology::PeriodicBoundaryStackIdentifier key4(13, 2);
  grid::StackTopology::PeriodicBoundaryStackIdentifier key5(14, 3);

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

  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key3.first));
  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key4.first));
  validate(grid::StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(key5.first));

  validateEqualsWithParams2(
    grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key0.first),
    15,
    key0.first,
    key0.second
  );
  validateEquals(grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key1.first), 16);
  validateEquals(grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key2.first), 18);

  validateEquals(grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key3.first), 19);
  validateEquals(grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key4.first), 21);
  validateEquals(grid::StackTopology::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key5.first), 22);

  //
  // Third step
  //
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(7), 22);
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(8), 21);
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(10), 19);
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(11), 18);
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(13), 16);
  validateEquals(grid::StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(14), 15);
#endif
#if DIMENSIONS == 3
  std::array<int, TWO_POWER_D> flags;
  flags.fill(grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flags[0] = 0;

  std::set<grid::StackTopology::PeriodicBoundaryStackIdentifier> result =
    grid::StackTopology::getOutputStacksForPeriodicBoundaryExchange(flags);
  validateEqualsWithParams1(
    result.size(),
    7,
    "expected output={(stack=9,bnd=111),(stack=10,bnd=110),(stack=12,bnd=101),(stack=13,bnd=100),(stack=18,bnd=011),(stack=19,bnd=010),(stack=21,bnd=001)}"
  );
#endif
}

void peano4::parallel::tests::NodeTest::testGetPeriodicBoundaryNumber() {
#if DIMENSIONS == 2
  std::array<int, TWO_POWER_D> flags;
  flags.fill(1);
  flags[0] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  flags[1] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  std::bitset<2 * DIMENSIONS> result = grid::StackTopology::getPeriodicBoundaryNumber(flags);
  validateEquals(result.to_ulong(), 2);
#endif
}

void peano4::parallel::tests::NodeTest::testBitsetBasedPeriodicBoundaryEnumeration() {
  auto getPeriodicBoundaryNumberViaDforEnumeration =
    [](const std::array<int, TWO_POWER_D>& flags) -> std::bitset<2 * DIMENSIONS> {
    std::bitset<2 * DIMENSIONS> result;

    for (int d = 0; d < 2 * DIMENSIONS; d++) {
      bool allFlagsSet = true;
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
        tarch::la::Vector<DIMENSIONS, int> entry;
        entry                 = k;
        entry(d % DIMENSIONS) = d / DIMENSIONS;
        allFlagsSet &=
          flags[peano4::dLinearised(entry, 2)] == peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
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

        const bool current =
          flags[peano4::dLinearised(entry, 2)] == peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
        allFlagsSet = allFlagsSet && current;
      }
      if (allFlagsSet) {
        result.set(d);
      }
    }

    return result;
  };

  std::array<int, TWO_POWER_D> topRightFlags;
  topRightFlags.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  topRightFlags[0] = 0;

  const auto topRightLegacy   = getPeriodicBoundaryNumberViaDforEnumeration(topRightFlags);
  const auto topRightBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(topRightFlags);
  const auto topRightTopology = grid::StackTopology::getPeriodicBoundaryNumber(topRightFlags);

  validateEquals(topRightBitset.to_ulong(), topRightLegacy.to_ulong());
  validateEquals(topRightTopology.to_ulong(), topRightLegacy.to_ulong());

  std::array<int, TWO_POWER_D> periodicFaceFlags;
  periodicFaceFlags.fill(1);
  periodicFaceFlags[0] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  periodicFaceFlags[1] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  const auto periodicFaceLegacy   = getPeriodicBoundaryNumberViaDforEnumeration(periodicFaceFlags);
  const auto periodicFaceBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(periodicFaceFlags);
  const auto periodicFaceTopology = grid::StackTopology::getPeriodicBoundaryNumber(periodicFaceFlags);

  validateEquals(periodicFaceBitset.to_ulong(), periodicFaceLegacy.to_ulong());
  validateEquals(periodicFaceTopology.to_ulong(), periodicFaceLegacy.to_ulong());

  std::array<int, TWO_POWER_D> allPeriodicFlags;
  allPeriodicFlags.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);

  const auto allPeriodicLegacy   = getPeriodicBoundaryNumberViaDforEnumeration(allPeriodicFlags);
  const auto allPeriodicBitset   = getPeriodicBoundaryNumberViaExplicitBitsetLoop(allPeriodicFlags);
  const auto allPeriodicTopology = grid::StackTopology::getPeriodicBoundaryNumber(allPeriodicFlags);

  validateEquals(allPeriodicBitset.to_ulong(), allPeriodicLegacy.to_ulong());
  validateEquals(allPeriodicTopology.to_ulong(), allPeriodicLegacy.to_ulong());
}

void peano4::parallel::tests::NodeTest::run() {
  testMethod(testGetPeriodicBoundaryNumber);
  testMethod(testBitsetBasedPeriodicBoundaryEnumeration);
  testMethod(testTagCalculation);
  testMethod(testGetOutputStacksForPeriodicBoundaryExchange)
}
