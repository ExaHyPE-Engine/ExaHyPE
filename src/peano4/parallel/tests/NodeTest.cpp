#include "NodeTest.h"
#include "../Node.h"

#include "tarch/la/Vector.h"

#include "peano4/grid/Spacetree.h"



tarch::logging::Log peano4::parallel::tests::NodeTest::_log("peano4::parallel::tests::NodeTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::parallel::tests::NodeTest::NodeTest():
  TestCase( "peano4::parallel::tests::NodeTest" ) {
}


void peano4::parallel::tests::NodeTest::testTagCalculation() {
  logTraceIn( "testTagCalculation()" );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<5) {
    Node::GridDataExchangeMetaInformation metaInfoA = peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(0,5,peano4::parallel::Node::ExchangeMode::HorizontalData);
    Node::GridDataExchangeMetaInformation metaInfoB = peano4::parallel::Node::getInstance().getGridDataExchangeMetaInformation(5,0,peano4::parallel::Node::ExchangeMode::HorizontalData);
    validateWithParams6(metaInfoA.first!=metaInfoB.first,
      metaInfoA.first, metaInfoB.first,
      Node::StacksPerCommunicationPartner, peano4::parallel::Node::getInstance().getLocalTreeId(0),
      peano4::parallel::Node::getInstance().getLocalTreeId(5), Node::MaxSpacetreesPerRank
    );
  }
  logTraceOut( "testTagCalculation()" );
}


void peano4::parallel::tests::NodeTest::testGetOutputStacksForPeriodicBoundaryExchange() {
  #if Dimensions==2
  //
  // Top right corner
  //
  tarch::la::Vector<TwoPowerD,int>  flagsTopRightVertex(peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition);
  flagsTopRightVertex(0) = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > resultTopRightVertex = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flagsTopRightVertex);
  validateEqualsWithParams1( resultTopRightVertex.size(), 3, flagsTopRightVertex );

  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key0( 7,3);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key1( 8,2);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key2(10,1);

  std::stringstream resultContentTopRightVertex;
  for (auto& p: resultTopRightVertex) {
    resultContentTopRightVertex << "(" << p.first << "," << std::bitset<2*Dimensions>(p.second) << ")";
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
  tarch::la::Vector<TwoPowerD,int>  flagsBottomLeftVertex(peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition);
  flagsBottomLeftVertex(3) = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > resultBottomLeftVertex = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flagsBottomLeftVertex);
  validateEqualsWithParams1( resultBottomLeftVertex.size(), 3, flagsBottomLeftVertex );

  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key3(11,1);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key4(13,2);
  peano4::parallel::Node::PeriodicBoundaryStackIdentifier key5(14,3);

  std::stringstream resultContentBottomLeftVertex;
  for (auto& p: resultBottomLeftVertex) {
    resultContentBottomLeftVertex << "(" << p.first << "," << std::bitset<2*Dimensions>(p.second) << ")";
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
  #if Dimensions==3
  tarch::la::Vector<TwoPowerD,int>  flags(peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition);
  flags(0) = 0;

  std::set< peano4::parallel::Node::PeriodicBoundaryStackIdentifier > result = peano4::parallel::Node::getOutputStacksForPeriodicBoundaryExchange(flags);
  validateEqualsWithParams3( result.size(), 7, flags, peano4::parallel::Node::toString(result), "expected output={(stack=9,bnd=111),(stack=10,bnd=110),(stack=12,bnd=101),(stack=13,bnd=100),(stack=18,bnd=011),(stack=19,bnd=010),(stack=21,bnd=001)}" );
  #endif
}


void peano4::parallel::tests::NodeTest::testGetPeriodicBoundaryNumber() {
  #if Dimensions==2
  tarch::la::Vector<TwoPowerD,int>  flags(1);
  flags(0) = peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition;
  flags(1) = peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition;

  std::bitset<2*Dimensions> result = peano4::parallel::Node::getPeriodicBoundaryNumber(flags);
  validateEqualsWithParams1( result.to_ulong(), 2, flags );
  #endif
}


void peano4::parallel::tests::NodeTest::run() {
  logTraceIn( "run()" );
  testMethod( testGetPeriodicBoundaryNumber );
  testMethod( testTagCalculation );
  testMethod( testGetOutputStacksForPeriodicBoundaryExchange )
  logTraceOut( "run()" );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
