// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EmptyTraversalObserver.h"

#include "Constants.h"

void tests::peano4::parallel::EmptyTraversalObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::parallel::EmptyTraversalObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::parallel::EmptyTraversalObserver::enterNode(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::parallel::EmptyTraversalObserver::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::GridTraversalObserver* tests::peano4::parallel::EmptyTraversalObserver::clone(int spacetreeId) {
  return new EmptyTraversalObserver();
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::parallel::EmptyTraversalObserver::
  getGridControlEvents() const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;
  return controlEvents;
}
