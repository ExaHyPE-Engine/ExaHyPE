// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CreateGridObserver.h"

#include "Constants.h"

void tests::peano4::grid::CreateGridObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::CreateGridObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::CreateGridObserver::enterNode(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::CreateGridObserver::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::GridTraversalObserver* tests::peano4::grid::CreateGridObserver::clone(int spacetreeId) {
  return new CreateGridObserver();
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::grid::CreateGridObserver::getGridControlEvents() const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;
  ::peano4::grid::GridControlEvent              newEvent;

  newEvent.setRefinementControl(::peano4::grid::GridControlEvent::RefinementControl::Refine);

#if DIMENSIONS == 2
  newEvent.offset   = {0.0, 0.0};
  newEvent.width    = {1.0, 1.0};
  newEvent.targetH  = {MinH, MinH};
#else
  newEvent.offset   = {0.0, 0.0, 0.0};
  newEvent.width    = {1.0, 1.0, 1.0};
  newEvent.targetH  = {MinH, MinH, MinH};
#endif

  controlEvents.push_back(std::move(newEvent));
  return controlEvents;
}
