// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridTraversalObserver.h"

namespace tests::peano4::parallel {
  class CreateGridObserver;
} // namespace tests::peano4::parallel

class tests::peano4::parallel::CreateGridObserver final: public ::peano4::grid::GridTraversalObserver {
private:
  bool _doRefinement;

public:
  CreateGridObserver(bool doRefinement);
  ~CreateGridObserver() override = default;

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
