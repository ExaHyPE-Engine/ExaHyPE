// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CreateGrid.h"

#include "Constants.h"

void tests::peano4::grid::CreateGrid::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::CreateGrid::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::CreateGrid::loadCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::CreateGrid::storeCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::CreateGrid::enterCell(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::CreateGrid::leaveCell(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::TraversalObserver* tests::peano4::grid::CreateGrid::clone(int spacetreeId) { return new CreateGrid(); }

std::vector<::peano4::grid::GridControlEvent> tests::peano4::grid::CreateGrid::getGridControlEvents() const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;
  ::peano4::grid::GridControlEvent              newEvent;
  newEvent.setRefinementControl(::peano4::grid::GridControlEvent::RefinementControl::Refine);
#if DIMENSIONS == 2
  newEvent.setOffset({0.0, 0.0});
  newEvent.setWidth({1.0, 1.0});
  newEvent.setH({MinH, MinH});
#else
  newEvent.setOffset({0.0, 0.0, 0.0});
  newEvent.setWidth({1.0, 1.0, 1.0});
  newEvent.setH({MinH, MinH, MinH});
#endif
  controlEvents.push_back(newEvent);
  return controlEvents;
}
