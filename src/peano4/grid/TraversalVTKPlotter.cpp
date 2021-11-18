#include "TraversalVTKPlotter.h"
#include "Spacetree.h"
#include "GridControlEvent.h"
#include "GridTraversalEvent.h"

#include "peano4/utils/Loop.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/StringMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"

#include <string>



tarch::logging::Log           peano4::grid::TraversalVTKPlotter::_log( "peano4::grid::TraversalVTKPlotter" );
tarch::mpi::BooleanSemaphore  peano4::grid::TraversalVTKPlotter::_sempahore( "peano4::grid::TraversalVTKPlotter" );


peano4::grid::TraversalVTKPlotter::TraversalVTKPlotter( const std::string& filename, int treeId ):
  _filename(filename),
  _spacetreeId(treeId),
  _writer(nullptr),
  _vertexWriter(nullptr),
  _cellWriter(nullptr),
  _spacetreeIdWriter(nullptr),
  _coreWriter(nullptr) {
}


peano4::grid::TraversalVTKPlotter::~TraversalVTKPlotter() {
}


void peano4::grid::TraversalVTKPlotter::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << _filename << "-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>0 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::mpi::Lock lock( _sempahore );

  _writer = new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter(
    snapshotFileName.str(),
    _filename,
    tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::NoIndexFile,
    0.0
  );

  _vertexWriter      = _writer->createVertexWriter();
  _cellWriter        = _writer->createCellWriter();
  _spacetreeIdWriter = _writer->createCellDataWriter( "tree-id", 1 );
  _coreWriter        = _writer->createCellDataWriter( "core-number", 1 );
}


void peano4::grid::TraversalVTKPlotter::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  assertion(_writer!=nullptr);

  _vertexWriter->close();
  _cellWriter->close();
  _spacetreeIdWriter->close();
  _coreWriter->close();

  if (_spacetreeId>=0) {
    _writer->writeToFile();
  }

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


void peano4::grid::TraversalVTKPlotter::loadCell(
  const GridTraversalEvent&  event
) {
}


void peano4::grid::TraversalVTKPlotter::storeCell(
  const GridTraversalEvent&  event
) {
}


void peano4::grid::TraversalVTKPlotter::enterCell(
  const GridTraversalEvent&  event
) {
  bool plot = event.getWillBeRefined()==0 and event.getIsCellLocal();
  if (plot) {
    plotCell(event);
  }
}


void peano4::grid::TraversalVTKPlotter::plotCell(
  const GridTraversalEvent&  event
) {
  int vertexIndices[TwoPowerD];

  dfor2(k)
    assertion( _vertexWriter!=nullptr );
    vertexIndices[kScalar] = _vertexWriter->plotVertex(
      event.getX() + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(k), event.getH() ) - event.getH() * 0.5
    );
  enddforx

	assertion( _cellWriter!=nullptr );
	int cellIndex = -1;

	#if Dimensions==2
  cellIndex = _cellWriter->plotQuadrangle(vertexIndices);
  #elif Dimensions==3
  cellIndex = _cellWriter->plotHexahedron(vertexIndices);
  #else
  logError( "enterCell(...)", "supports only 2d and 3d" );
  #endif

  assertion( _spacetreeIdWriter!=nullptr );
  assertion( _coreWriter!=nullptr );
  _spacetreeIdWriter->plotCell(cellIndex,_spacetreeId);
   _coreWriter->plotCell(cellIndex,tarch::multicore::Core::getInstance().getCoreNumber());
}


void peano4::grid::TraversalVTKPlotter::leaveCell(
  const GridTraversalEvent&  event
) {
}


peano4::grid::TraversalObserver*  peano4::grid::TraversalVTKPlotter::clone(int spacetreeId) {
  return new peano4::grid::TraversalVTKPlotter(
    _filename,
    spacetreeId
  );
}


std::vector< peano4::grid::GridControlEvent > peano4::grid::TraversalVTKPlotter::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}
