// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridTraversalObserver.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter.h"

namespace tests::peano4::grid {
  class VTKPlotterObserver;
} // namespace tests::peano4::grid

class tests::peano4::grid::VTKPlotterObserver: public ::peano4::grid::GridTraversalObserver {
private:
  const std::string _filename;
  const int         _spacetreeId;

  tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter*                 _writer;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexWriter*   _vertexWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellWriter*     _cellWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter* _spacetreeIdWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter* _coreWriter;

  static tarch::mpi::BooleanSemaphore _semaphore;

  void plot(const ::peano4::grid::GridTraversalEvent& event);

public:
  VTKPlotterObserver(const std::string& filename, int treeId = -1);
  ~VTKPlotterObserver() override = default;

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
