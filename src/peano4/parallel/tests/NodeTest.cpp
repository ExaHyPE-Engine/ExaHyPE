#include "NodeTest.h"
#include "peano4/parallel/Node.h"

#include "peano4/utils/Linearised.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include "peano4/grid/SpacetreeHelper.h"

peano4::parallel::tests::NodeTest::NodeTest():
  TestCase( "peano4::parallel::tests::NodeTest" ) {
}

void peano4::parallel::tests::NodeTest::testTagCalculation() {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<5) {
    Node::GridDataExchangeMetaInformation metaInfoA = peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(0,5,peano4::parallel::Node::ExchangeMode::HorizontalData);
    Node::GridDataExchangeMetaInformation metaInfoB = peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(5,0,peano4::parallel::Node::ExchangeMode::HorizontalData);
    validateWithParams6(metaInfoA.first!=metaInfoB.first,
      metaInfoA.first, metaInfoB.first,
      Node::StacksPerCommunicationPartner, peano4::parallel::Node::getInstance().getLocalTreeId(0),
      peano4::parallel::Node::getInstance().getLocalTreeId(5), Node::MaxSpacetreesPerRank
    );
  }
}

void peano4::parallel::tests::NodeTest::testGetOutputStacksForPeriodicBoundaryExchange() {
  #if DIMENSIONS==2
  //
  // Top right corner
  //
  std::array<int, TWO_POWER_D>  flagsTopRightVertex;
  flagsTopRightVertex.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsTopRightVertex[0] = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > resultTopRightVertex = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flagsTopRightVertex);
  validateEquals( resultTopRightVertex.size(), 3 );

  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key0( 7,3);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key1( 8,2);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key2(10,1);

  std::stringstream resultContentTopRightVertex;
  for (auto& p: resultTopRightVertex) {
    resultContentTopRightVertex << "(" << p.first << "," << std::bitset<2*DIMENSIONS>(p.second) << ")";
  }
  validateWithParams3( resultTopRightVertex.count(key0)==1, key0.first, key0.second, resultContentTopRightVertex.str() );
  validateWithParams3( resultTopRightVertex.count(key1)==1, key1.first, key1.second, resultContentTopRightVertex.str() );
  validateWithParams3( resultTopRightVertex.count(key2)==1, key2.first, key2.second, resultContentTopRightVertex.str() );

  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key0.first) );
  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key1.first) );
  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key2.first) );

  //
  // bottom left vertex
  //
  std::array<int, TWO_POWER_D> flagsBottomLeftVertex;
  flagsBottomLeftVertex.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flagsBottomLeftVertex[3] = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > resultBottomLeftVertex = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flagsBottomLeftVertex);
  validateEquals( resultBottomLeftVertex.size(), 3 );

  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key3(11,1);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key4(13,2);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key5(14,3);

  std::stringstream resultContentBottomLeftVertex;
  for (auto& p: resultBottomLeftVertex) {
    resultContentBottomLeftVertex << "(" << p.first << "," << std::bitset<2*DIMENSIONS>(p.second) << ")";
  }
  validateWithParams3( resultBottomLeftVertex.count(key3)==1, key3.first, key3.second, resultContentBottomLeftVertex.str() );
  validateWithParams3( resultBottomLeftVertex.count(key4)==1, key4.first, key4.second, resultContentBottomLeftVertex.str() );
  validateWithParams3( resultBottomLeftVertex.count(key5)==1, key5.first, key5.second, resultContentBottomLeftVertex.str() );

  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key3.first) );
  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key4.first) );
  validate( peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(key5.first) );

  validateEqualsWithParams2( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key0.first), 15, key0.first, key0.second );
  validateEquals( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key1.first), 16 );
  validateEquals( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key2.first), 18 );

  validateEquals( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key3.first), 19 );
  validateEquals( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key4.first), 21 );
  validateEquals( peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(key5.first), 22 );

  //
  // Third step
  //
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack( 7), 22 );
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack( 8), 21 );
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(10), 19 );
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(11), 18 );
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(13), 16 );
  validateEquals( peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(14), 15 );
  #endif
  #if DIMENSIONS==3
  std::array<int, TWO_POWER_D> flags;
  flags.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);
  flags[0] = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > result = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flags);
  validateEqualsWithParams2( result.size(), 7, peano4::parallel::Node::toString(result), "expected output={(stack=9,bnd=111),(stack=10,bnd=110),(stack=12,bnd=101),(stack=13,bnd=100),(stack=18,bnd=011),(stack=19,bnd=010),(stack=21,bnd=001)}" );
  #endif
}

void peano4::parallel::tests::NodeTest::testGetPeriodicBoundaryNumber() {
  #if DIMENSIONS==2
  std::array<int, TWO_POWER_D> flags;
  flags.fill(1);
  flags[0] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  flags[1] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  std::bitset<2*DIMENSIONS> result = peano4::parallel::Node::getPeriodicBoundaryNumber(flags);
  validateEquals( result.to_ulong(), 2 );
  #endif
}

void peano4::parallel::tests::NodeTest::testBitsetBasedPeriodicBoundaryEnumeration() {
  auto getPeriodicBoundaryNumberViaDforEnumeration =
    [](const std::array<int, TWO_POWER_D>& flags) -> std::bitset<2 * DIMENSIONS> {
    std::bitset<2 * DIMENSIONS> result;

    for (int d = 0; d < 2 * DIMENSIONS; d++) {
      bool                                        allFlagsSet = true;
      dfor2(k) tarch::la::Vector<DIMENSIONS, int> entry;
      entry                 = k;
      entry(d % DIMENSIONS) = d / DIMENSIONS;
      allFlagsSet &= flags[peano4::utils::dLinearised(entry, 2)] == peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
      enddforx if (allFlagsSet) { result.set(d); }
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

        const bool current = flags[peano4::utils::dLinearised(entry, 2)] == peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
        allFlagsSet        = allFlagsSet && current;
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

  const auto topRightLegacy = getPeriodicBoundaryNumberViaDforEnumeration(topRightFlags);
  const auto topRightBitset = getPeriodicBoundaryNumberViaExplicitBitsetLoop(topRightFlags);
  const auto topRightNode   = peano4::parallel::Node::getPeriodicBoundaryNumber(topRightFlags);

  validateEquals(topRightBitset.to_ulong(), topRightLegacy.to_ulong());
  validateEquals(topRightNode.to_ulong(), topRightLegacy.to_ulong());

  std::array<int, TWO_POWER_D> periodicFaceFlags;
  periodicFaceFlags.fill(1);
  periodicFaceFlags[0] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  periodicFaceFlags[1] = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  const auto periodicFaceLegacy = getPeriodicBoundaryNumberViaDforEnumeration(periodicFaceFlags);
  const auto periodicFaceBitset = getPeriodicBoundaryNumberViaExplicitBitsetLoop(periodicFaceFlags);
  const auto periodicFaceNode   = peano4::parallel::Node::getPeriodicBoundaryNumber(periodicFaceFlags);

  validateEquals(periodicFaceBitset.to_ulong(), periodicFaceLegacy.to_ulong());
  validateEquals(periodicFaceNode.to_ulong(), periodicFaceLegacy.to_ulong());

  std::array<int, TWO_POWER_D> allPeriodicFlags;
  allPeriodicFlags.fill(peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition);

  const auto allPeriodicLegacy = getPeriodicBoundaryNumberViaDforEnumeration(allPeriodicFlags);
  const auto allPeriodicBitset = getPeriodicBoundaryNumberViaExplicitBitsetLoop(allPeriodicFlags);
  const auto allPeriodicNode   = peano4::parallel::Node::getPeriodicBoundaryNumber(allPeriodicFlags);

  validateEquals(allPeriodicBitset.to_ulong(), allPeriodicLegacy.to_ulong());
  validateEquals(allPeriodicNode.to_ulong(), allPeriodicLegacy.to_ulong());
}


void peano4::parallel::tests::NodeTest::run() {
  testMethod( testGetPeriodicBoundaryNumber );
  testMethod( testBitsetBasedPeriodicBoundaryEnumeration );
  testMethod( testTagCalculation );
  testMethod( testGetOutputStacksForPeriodicBoundaryExchange )
}
