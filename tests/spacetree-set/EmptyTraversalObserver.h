// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridTraversalObserver.h"

namespace tests::peano4::parallel {
  class EmptyTraversalObserver;
} // namespace tests::peano4::parallel

class tests::peano4::parallel::EmptyTraversalObserver final: public ::peano4::grid::GridTraversalObserver {
public:
  EmptyTraversalObserver()           = default;
  ~EmptyTraversalObserver() override = default;

  void beginTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  void endTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  void enterNode(const ::peano4::grid::GridTraversalEvent& event) override;
  void leaveNode(const ::peano4::grid::GridTraversalEvent& event) override;

  GridTraversalObserver* clone(int spacetreeId) override;

  std::vector<::peano4::grid::GridControlEvent> getGridControlEvents() const override;
};
