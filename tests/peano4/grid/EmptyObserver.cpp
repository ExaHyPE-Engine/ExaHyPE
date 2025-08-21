// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EmptyObserver.h"

#include "Constants.h"

void tests::peano4::grid::EmptyObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::EmptyObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::EmptyObserver::loadCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::EmptyObserver::storeCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::EmptyObserver::enterCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::EmptyObserver::leaveCell(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::TraversalObserver* tests::peano4::grid::EmptyObserver::clone(int spacetreeId) {
  return new EmptyObserver();
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::grid::EmptyObserver::getGridControlEvents() const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;
  return controlEvents;
}
