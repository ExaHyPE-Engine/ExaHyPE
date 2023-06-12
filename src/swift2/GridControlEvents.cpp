#include "GridControlEvents.h"

#include "peano4/grid/grid.h"

std::vector<peano4::grid::GridControlEvent> swift2::committedGridControlEvents;

void swift2::commitGridControlEvents(const std::list<peano4::grid::GridControlEvent>& events) {

  committedGridControlEvents.clear();
  committedGridControlEvents.reserve(events.size());

  for (const auto& p : events) {
    committedGridControlEvents.push_back(p);
  }

  committedGridControlEvents = peano4::grid::merge(committedGridControlEvents, 0.4);
}
