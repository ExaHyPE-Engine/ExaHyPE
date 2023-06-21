#include "GridControlEventTest.h"

#include "../grid.h"
#include "../GridControlEvent.h"

tarch::logging::Log peano4::grid::tests::GridControlEventTest::_log("peano4::grid::tests::GridControlEventTest");

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", off)
#endif

peano4::grid::tests::GridControlEventTest::GridControlEventTest():
  TestCase("peano4::grid::tests::GridControlEventTest") {}

void peano4::grid::tests::GridControlEventTest::testMerge1() {
#if Dimensions == 2
  std::vector<GridControlEvent> events;

  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {-0.0333333, -0.0333333}, {0.4, 0.4}, {0.111111, 0.111111}
  ));
  events.push_back(
    GridControlEvent(GridControlEvent::RefinementControl::Refine, {-0.0333333, 0.3}, {0.4, 0.4}, {0.111111, 0.111111})
  );
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {-0.0333333, 0.633333}, {0.4, 0.4}, {0.111111, 0.111111}
  ));
  events.push_back(
    GridControlEvent(GridControlEvent::RefinementControl::Refine, {0.3, -0.0333333}, {0.4, 0.4}, {0.111111, 0.111111})
  );
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {0.633333, -0.0333333}, {0.4, 0.4}, {0.111111, 0.111111}
  ));
  events.push_back(
    GridControlEvent(GridControlEvent::RefinementControl::Refine, {0.633333, 0.3}, {0.4, 0.4}, {0.111111, 0.111111})
  );
  events.push_back(
    GridControlEvent(GridControlEvent::RefinementControl::Refine, {0.3, 0.3}, {0.4, 0.4}, {0.111111, 0.111111})
  );
  events.push_back(
    GridControlEvent(GridControlEvent::RefinementControl::Refine, {0.3, 0.633333}, {0.4, 0.4}, {0.111111, 0.111111})
  );
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {0.633333, 0.633333}, {0.4, 0.4}, {0.111111, 0.111111}
  ));

  const double tolerance = 0.1;
  events                 = peano4::grid::merge(events, tolerance);

  validateEquals(events.size(), 1);
  validateWithParams1(tarch::la::equals(events[0].getOffset(), -0.0333333, 0.001), events[0].toString());
  validateWithParams1(tarch::la::equals(events[0].getWidth(), 1.06667, 0.001), events[0].toString());

#endif
}

void peano4::grid::tests::GridControlEventTest::testMerge2() {
#if Dimensions == 2
  const double tolerance = 0.1;

  GridControlEvent event0(
    GridControlEvent::RefinementControl::Refine, {0.316667, 0.316667}, {0.366667, 0.366667}, {0.111111, 0.111111}
  );
  GridControlEvent event1(
    GridControlEvent::RefinementControl::Refine, {0.65, 0.316667}, {0.366667, 0.366667}, {0.111111, 0.111111}
  );
  GridControlEvent event2(
    GridControlEvent::RefinementControl::Refine, {0.65, -0.0166667}, {0.366667, 0.366667}, {0.111111, 0.111111}
  );
  GridControlEvent event3(
    GridControlEvent::RefinementControl::Refine, {0.316667, -0.0166667}, {0.366667, 0.366667}, {0.111111, 0.111111}
  );
  GridControlEvent event4(
    GridControlEvent::RefinementControl::Refine, {-0.0166667, -0.0166667}, {0.366667, 0.366667}, {0.111111, 0.111111}
  );

  validateWithParams2(
    peano4::grid::internal::twoEventsAreAdjacent(event0, event1, tolerance), event0.toString(), event1.toString()
  );
  validateWithParams2(
    peano4::grid::internal::twoEventsAreAdjacent(event1, event2, tolerance), event1.toString(), event2.toString()
  );
  validateWithParams2(
    not peano4::grid::internal::twoEventsAreAdjacent(event0, event2, tolerance), event0.toString(), event2.toString()
  );

  std::vector<GridControlEvent> events;
  events.push_back(event0);
  events.push_back(event1);
  events.push_back(event2);
  events.push_back(event3);
  events.push_back(event4);

  events = peano4::grid::merge(events, 0.1);

  validate(events.size() > 1);
#endif
}

void peano4::grid::tests::GridControlEventTest::testMerge3() {
#if Dimensions == 2
  std::vector<GridControlEvent> events;

  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {0.65, 0.65}, {0.366667, 0.366667}, {0.111111, 0.111111}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine, {0.883333, 0.883333}, {0.122222, 0.122222}, {0.037037, 0.037037}
  ));

  events = peano4::grid::merge(events, 0.4);

  validateEquals(events.size(), 2);
#endif
}

// Der erste Fehler ist, dass der Merge in 3d net geht
// Der zweite Fehler ist, dass offensichtlich Merge Events nicht ausgetauscht werden oder nicht lange genug ueberleben

void peano4::grid::tests::GridControlEventTest::testSortWithinMerge() {
#if Dimensions == 3
  std::list<GridControlEvent> events;

  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.0611111, 0.05, 0.272222},
    {0.122222, 0.122222, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.172222, -0.0611111, 0.272222},
    {0.122222, 0.233333, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.172222, -0.0611111, 0.383333},
    {0.122222, 0.455556, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.172222, 0.161111, 0.272222},
    {0.233333, 0.233333, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.505556, 0.383333, 0.383333},
    {0.455556, 0.122222, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.0611111, 0.05, 0.383333},
    {0.122222, 0.455556, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.505556, 0.383333, 0.272222},
    {0.566667, 0.122222, 0.122222},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.0611111, -0.0611111, 0.272222},
    {0.566667, 0.122222, 0.233333},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {-0.505556, -0.0611111, 0.272222},
    {0.344444, 0.455556, 0.233333},
    {0.037037, 0.037037, 0.037037}
  ));
  events.push_back(GridControlEvent(
    GridControlEvent::RefinementControl::Refine,
    {0.272222, -0.505556, 0.272222},
    {0.233333, 0.455556, 0.233333},
    {0.037037, 0.037037, 0.037037}
  ));

  peano4::grid::internal::sort(events);

  validateEqualsWithParams1(events.size(), 10, peano4::grid::toString(events));

  std::list<GridControlEvent>::const_iterator p = events.begin();
  validateNumericalEqualsWithParams1(p->getOffset(0), -0.505556, peano4::grid::toString(events));
  p++;
  validateNumericalEqualsWithParams1(p->getOffset(0), -0.505556, peano4::grid::toString(events));
#endif
}

void peano4::grid::tests::GridControlEventTest::run() {
  testMethod(testMerge1);
  testMethod(testMerge2);
  testMethod(testMerge3);
  testMethod(testSortWithinMerge);
}

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("", on)
#endif
