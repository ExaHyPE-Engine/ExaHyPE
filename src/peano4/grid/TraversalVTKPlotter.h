// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TraversalObserver.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter.h"

namespace peano4::grid {
  class TraversalVTKPlotter;
} // namespace peano4::grid

class peano4::grid::TraversalVTKPlotter: public peano4::grid::TraversalObserver {
protected:
  static tarch::logging::Log _log;

  const std::string _filename;
  const int         _spacetreeId;

  /**
   * Does the actual plotting, i.e. all checks/decision making is already done before
   */
  void plotCell(const GridTraversalEvent& event);

private:
  tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter*                 _writer;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexWriter*   _vertexWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellWriter*     _cellWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter* _spacetreeIdWriter;
  tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter* _coreWriter;

  static tarch::mpi::BooleanSemaphore _sempahore;

public:
  /**
   * You have to invoke startNewSnapshot() if you wanna have a pvd file
   * immediately after you've created this observer in the main code.
   *
   * If this guy is ran on the global master,
   */
  TraversalVTKPlotter(const std::string& filename, int treeId = -1);
  virtual ~TraversalVTKPlotter();

  virtual void beginTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  virtual void endTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  virtual void loadCell(const GridTraversalEvent& event) override;

  virtual void storeCell(const GridTraversalEvent& event) override;

  virtual void enterCell(const GridTraversalEvent& event) override;

  virtual void leaveCell(const GridTraversalEvent& event) override;

  virtual TraversalObserver* clone(int spacetreeId) override;

  /**
   * Obviously empty for this particular observer.
   */
  virtual std::vector<GridControlEvent> getGridControlEvents() const override;
};
