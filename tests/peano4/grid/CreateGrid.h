// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/TraversalObserver.h"

namespace tests::peano4::grid {
  class CreateGrid;
} // namespace tests::peano4::grid

class tests::peano4::grid::CreateGrid: public ::peano4::grid::TraversalObserver {
public:
  CreateGrid()                   = default;
  virtual ~CreateGrid() override = default;

  void beginTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  void endTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  void loadCell(const ::peano4::grid::GridTraversalEvent& event) override;
  void storeCell(const ::peano4::grid::GridTraversalEvent& event) override;

  void enterCell(const ::peano4::grid::GridTraversalEvent& event) override;
  void leaveCell(const ::peano4::grid::GridTraversalEvent& event) override;

  TraversalObserver* clone(int spacetreeId) override;

  std::vector<::peano4::grid::GridControlEvent> getGridControlEvents() const override;
};
