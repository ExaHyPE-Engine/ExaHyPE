#include "GridControlEventTest.h"
#include "../grid.h"
#include "../GridControlEvent.h"


tarch::logging::Log peano4::grid::tests::GridControlEventTest::_log("peano4::grid::tests::GridControlEventTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::grid::tests::GridControlEventTest::GridControlEventTest():
  TestCase( "peano4::grid::tests::GridControlEventTest" ) {
}



void peano4::grid::tests::GridControlEventTest::testMerge1() {
  #if Dimensions==2
  std::vector< GridControlEvent > events;

  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {-0.0333333,-0.0333333},  {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {-0.0333333,0.3},         {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {-0.0333333,0.633333},    {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.3,-0.0333333},         {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.633333,-0.0333333},    {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.633333,0.3},           {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.3,0.3},                {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.3,0.633333},           {0.4,0.4}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.633333,0.633333},      {0.4,0.4}, {0.111111,0.111111} ));

  const double tolerance = 0.1;
  events = peano4::grid::merge( events, tolerance );

  validateEquals( events.size(), 1 );
  validateWithParams1( tarch::la::equals( events[0].getOffset(), -0.0333333, 0.001 ), events[0].toString() );
  validateWithParams1( tarch::la::equals( events[0].getWidth(),   1.06667,   0.001 ), events[0].toString() );

  #endif
}



void peano4::grid::tests::GridControlEventTest::testMerge2() {
  #if Dimensions==2
  std::vector< GridControlEvent > events;

  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.316667,0.316667},     {0.366667,0.366667}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.65,0.316667},         {0.366667,0.366667}, {0.111111,0.111111} ));

  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.65,-0.0166667},       {0.366667,0.366667}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.316667,-0.0166667},   {0.366667,0.366667}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {-0.0166667,-0.0166667}, {0.366667,0.366667}, {0.111111,0.111111} ));

  events = peano4::grid::merge( events, 0.4 );

  validateEquals( events.size(), 2 );
  #endif
}


void peano4::grid::tests::GridControlEventTest::testMerge3() {
  #if Dimensions==2
  std::vector< GridControlEvent > events;

  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.65,0.65},         {0.366667,0.366667}, {0.111111,0.111111} ));
  events.push_back( GridControlEvent( GridControlEvent::RefinementControl::Refine, {0.883333,0.883333}, {0.122222,0.122222}, {0.037037,0.037037} ));

  events = peano4::grid::merge( events, 0.4 );

  validateEquals( events.size(), 2 );
  #endif
}


void peano4::grid::tests::GridControlEventTest::run() {
  testMethod( testMerge1 );
  testMethod( testMerge2 );
  testMethod( testMerge3 );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
