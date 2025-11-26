// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKPlotterObserver.h"

#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/utils/Loop.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

tarch::mpi::BooleanSemaphore tests::peano4::grid::VTKPlotterObserver::_semaphore;

tests::peano4::grid::VTKPlotterObserver::VTKPlotterObserver(const std::string& filename, int treeId):
  _filename(filename),
  _spacetreeId(treeId),
  _writer(nullptr),
  _vertexWriter(nullptr),
  _cellWriter(nullptr),
  _spacetreeIdWriter(nullptr),
  _coreWriter(nullptr) {}

void tests::peano4::grid::VTKPlotterObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << _filename << "-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 0) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::mpi::Lock lock(_semaphore);

  _writer = new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter(
    snapshotFileName.str(),
    _filename,
    "",
    tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::NoIndexFile,
    0.0
  );

  _vertexWriter      = _writer->createVertexWriter();
  _cellWriter        = _writer->createCellWriter();
  _spacetreeIdWriter = _writer->createCellDataWriter("tree-id", 1);
  _coreWriter        = _writer->createCellDataWriter("core-number", 1);
}

void tests::peano4::grid::VTKPlotterObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  assertion(_writer != nullptr);

  _vertexWriter->close();
  _cellWriter->close();
  _spacetreeIdWriter->close();
  _coreWriter->close();

  _writer->writeToFile();

  delete _writer;
  delete _vertexWriter;
  delete _cellWriter;
  delete _spacetreeIdWriter;
  delete _coreWriter;

  _writer            = nullptr;
  _vertexWriter      = nullptr;
  _cellWriter        = nullptr;
  _spacetreeIdWriter = nullptr;
  _coreWriter        = nullptr;
}

void tests::peano4::grid::VTKPlotterObserver::enterNode(const ::peano4::grid::GridTraversalEvent& event) {
  if (event.getWillBeRefined() == 0 and event.getIsCellLocal()) {
    plot(event);
  }
}

void tests::peano4::grid::VTKPlotterObserver::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {}

void tests::peano4::grid::VTKPlotterObserver::plot(const ::peano4::grid::GridTraversalEvent& event) {
  int vertexIndices[TWO_POWER_D];

  dfor2(k) assertion(_vertexWriter != nullptr);
  vertexIndices[kScalar] = _vertexWriter->plotVertex(
    event.getX() + tarch::la::multiplyComponents(tarch::la::convertScalar<double>(k), event.getH()) - event.getH() * 0.5
  );
  enddforx

      assertion(_cellWriter != nullptr);
  int cellIndex = -1;

#if DIMENSIONS == 2
  cellIndex = _cellWriter->plotQuadrangle(vertexIndices);
#else
  cellIndex = _cellWriter->plotHexahedron(vertexIndices);
#endif

  assertion(_spacetreeIdWriter != nullptr);
  assertion(_coreWriter != nullptr);
  _spacetreeIdWriter->plotCell(cellIndex, _spacetreeId);
  _coreWriter->plotCell(cellIndex, tarch::multicore::Core::getInstance().getCoreNumber());
}

::peano4::grid::GridTraversalObserver* tests::peano4::grid::VTKPlotterObserver::clone(int spacetreeId) {
  return new VTKPlotterObserver(_filename, spacetreeId);
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::grid::VTKPlotterObserver::getGridControlEvents() const {
  return std::vector<::peano4::grid::GridControlEvent>();
}
