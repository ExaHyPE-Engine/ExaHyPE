// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridTraversalObserver.h"
#include "tarch/plotter/AMRPlotter.h"
#include "tarch/plotter/VTUFileWriter.h"

namespace tests::peano4::grid {
  class VTUPlotterObserver;
} // namespace tests::peano4::grid

class tests::peano4::grid::VTUPlotterObserver final: public ::peano4::grid::GridTraversalObserver {
private:
  std::string _filename;
  int         _spacetreeId;
  int         _localSpacetreeId;

  static std::unique_ptr<tarch::plotter::VTUFileWriter>        _fileWriter;
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subdomainPlotter;

  std::unique_ptr<tarch::plotter::AMRPlotter> _amrPlotter;

public:
  VTUPlotterObserver(int treeId);
  ~VTUPlotterObserver() override = default;

  static void prepareTraversal();
  static void unprepareTraversal();

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
