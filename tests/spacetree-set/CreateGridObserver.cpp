// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CreateGridObserver.h"

#include "Constants.h"

tests::peano4::parallel::CreateGridObserver::CreateGridObserver(bool doRefinement):
  _doRefinement(doRefinement) {}

void tests::peano4::parallel::CreateGridObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::parallel::CreateGridObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}


void tests::peano4::parallel::CreateGridObserver::enterNode(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::parallel::CreateGridObserver::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::GridTraversalObserver* tests::peano4::parallel::CreateGridObserver::clone(int spacetreeId) {
  return new CreateGridObserver(_doRefinement);
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::parallel::CreateGridObserver::
  getGridControlEvents() const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;

  if (not _doRefinement) {
    return controlEvents;
  }

  ::peano4::grid::GridControlEvent newEvent;
  newEvent.setRefinementControl(::peano4::grid::GridControlEvent::RefinementControl::Refine);
#if DIMENSIONS == 2
  newEvent.offset = {0.0, 0.0};
  newEvent.width  = {1.0, 1.0};
  newEvent.h      = {MinH, MinH};
#else
  newEvent.offset = {0.0, 0.0, 0.0};
  newEvent.width  = {1.0, 1.0, 1.0};
  newEvent.h      = {MinH, MinH, MinH};
#endif
  controlEvents.push_back(std::move(newEvent));
  return controlEvents;
}
