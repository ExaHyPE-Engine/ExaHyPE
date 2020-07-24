#include "TraversalVTKPlotter.h"
#include "Spacetree.h"
#include "GridControlEvent.h"
#include "GridTraversalEvent.h"

#include "peano4/utils/Loop.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/StringMessage.h"
#include "tarch/mpi/IntegerMessage.h"

#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"

#include <string>



tarch::logging::Log  peano4::grid::TraversalVTKPlotter::_log( "peano4::grid::TraversalVTKPlotter" );
tarch::multicore::BooleanSemaphore  peano4::grid::TraversalVTKPlotter::_semaphore;


int peano4::grid::TraversalVTKPlotter::_counter(0);

#ifdef Parallel
int peano4::grid::TraversalVTKPlotter::_plotterMessageTag = tarch::mpi::Rank::reserveFreeTag( "peano4::grid::TraversalVTKPlotter" );
#endif


peano4::grid::TraversalVTKPlotter::TraversalVTKPlotter( const std::string& filename, int treeId ):
  _filename(filename),
  _spacetreeId(treeId),
  _writer(nullptr),
  _vertexWriter(nullptr),
  _cellWriter(nullptr),
  _spacetreeIdWriter(nullptr),
  _coreWriter(nullptr),
  _timeSeriesWriter() {
  openFile();
}


peano4::grid::TraversalVTKPlotter::~TraversalVTKPlotter() {
  closeFile();
}


void peano4::grid::TraversalVTKPlotter::openFile() {
  if (_writer==nullptr) {
    _writer            = new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter();
    _vertexWriter      = _writer->createVertexWriter();
    _cellWriter        = _writer->createCellWriter();
    _spacetreeIdWriter = _writer->createCellDataWriter( "tree-id", 1 );
    _coreWriter        = _writer->createCellDataWriter( "core-number", 1 );
  }
}


void peano4::grid::TraversalVTKPlotter::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
}


void peano4::grid::TraversalVTKPlotter::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
}


std::string peano4::grid::TraversalVTKPlotter::getFilename( int spacetreeId ) const {
  std::string currentFile = _filename;

  // we run serially
  if (spacetreeId==-1) {
    currentFile += "-" + std::to_string( _counter );
  }
  else {
    currentFile += "-" + std::to_string(spacetreeId) + "-" + std::to_string( _counter );
  }

  return currentFile;
}


void peano4::grid::TraversalVTKPlotter::closeFile() {
  if (_writer!=nullptr) {
    _vertexWriter->close();
    _cellWriter->close();
    _spacetreeIdWriter->close();
    _coreWriter->close();

    delete _vertexWriter;
    delete _cellWriter;
    delete _spacetreeIdWriter;
    delete _coreWriter;

    _vertexWriter      = nullptr;
    _cellWriter        = nullptr;
    _spacetreeIdWriter = nullptr;
    _coreWriter        = nullptr;

    _writer->writeToFile( getFilename( _spacetreeId ) );
    delete _writer;
    _writer = nullptr;
  }
}


void peano4::grid::TraversalVTKPlotter::enterCell(
  const GridTraversalEvent&  event
) {
  bool plot = event.getIsRefined()==0 and event.getIsCellLocal();
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


void peano4::grid::TraversalVTKPlotter::updateMetaFile(int spacetreeId) {
  tarch::multicore::Lock lock(_semaphore);

  std::string newFile = getFilename( spacetreeId );

  for (auto& p: _clonedSpacetreeIds) {
    assertion3( p!=newFile, p, newFile, spacetreeId );
  }

  _clonedSpacetreeIds.push_back( newFile );
  // avoid typical invocation for same id twice in a row
  assertion2(
    _clonedSpacetreeIds.size()<=1 or
    _clonedSpacetreeIds[0]!=_clonedSpacetreeIds[1],
    spacetreeId,
    _clonedSpacetreeIds[0]
  );
}


peano4::grid::TraversalObserver*  peano4::grid::TraversalVTKPlotter::clone(int spacetreeId) {
  peano4::grid::TraversalVTKPlotter* result = new peano4::grid::TraversalVTKPlotter(
    _filename,
    spacetreeId
  );

  if (_spacetreeId!=-1) {
    assertionMsg( false, "clone() should not be called for particular spacetree plotter" );
  }
  else {
    updateMetaFile(spacetreeId);
  }

  return result;
}


void peano4::grid::TraversalVTKPlotter::beginTraversalOnRank(bool isParallelRun) {
}


void peano4::grid::TraversalVTKPlotter::endTraversalOnRank(bool isParallelRun) {
  // Should not be necessary, but you never know
  tarch::multicore::Lock lock(_semaphore);

  _counter++;

  assertion( _spacetreeId==-1 );

  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    #ifdef Parallel
    assertion(isParallelRun);

    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
      if (rank!=tarch::mpi::Rank::getGlobalMasterRank()) {
        int entries;
        tarch::mpi::IntegerMessage snapshotCounter;
        tarch::mpi::IntegerMessage::receiveAndPollDanglingMessages( snapshotCounter, rank, _plotterMessageTag );
        // @todo Debug
        logInfo( "endTraversalOnRank(...)", "will receive " << snapshotCounter.getValue() << " snapshots from rank " << rank);

        for (int i=0; i<snapshotCounter.getValue(); i++) {
          tarch::mpi::StringMessage message;
          tarch::mpi::StringMessage::receiveAndPollDanglingMessages( message, rank, _plotterMessageTag );
          _clonedSpacetreeIds.push_back( message.getData() );
          // @todo Debug
          logInfo( "endTraversalOnRank(...)", "- rank " << rank << " has written snapshot " << message.getData() );
        }
      }
    }
    // @todo Debug
    logInfo( "endTraversalOnRank(...)", "received snapshots from all other ranks, dump a meta file with " << _clonedSpacetreeIds.size() << " entries" );
    #endif

    if ( not _clonedSpacetreeIds.empty() ) {
      assertion1( _writer!=nullptr, _spacetreeId );
      _writer->writeMetaDataFileForParallelSnapshot(
         _filename + "-" + std::to_string( _counter ),
        _clonedSpacetreeIds
      );

      _timeSeriesWriter.addSnapshot( _filename + "-" + std::to_string( _counter ), _counter, isParallelRun );
      _timeSeriesWriter.writeFile( _filename );
    }
  }
  else {
    #ifdef Parallel
    assertion(isParallelRun);

    // @todo Debug
    logInfo( "endTraversalOnRank(...)", "inform master that rank has written " << _clonedSpacetreeIds.size() << " snapshots" );

    tarch::mpi::IntegerMessage entriesMessage;
    entriesMessage.setValue( _clonedSpacetreeIds.size() );
    tarch::mpi::IntegerMessage::sendAndPollDanglingMessages(entriesMessage, tarch::mpi::Rank::getGlobalMasterRank(), _plotterMessageTag);

    for (auto p: _clonedSpacetreeIds) {
      // @todo Debug
      logInfo( "endTraversalOnRank(...)", "- hand over snapshot filename " << p );
      tarch::mpi::StringMessage message;
      message.setData( p );
      tarch::mpi::StringMessage::sendAndPollDanglingMessages(message, tarch::mpi::Rank::getGlobalMasterRank(), _plotterMessageTag);
    }
    // @todo Debug
    logInfo( "endTraversalOnRank(...)", "sent names of all snapshots to master" );
    #else
    assertionMsg( false, "should never enter" );
    #endif
  }

  _clonedSpacetreeIds.clear();
}


std::vector< peano4::grid::GridControlEvent > peano4::grid::TraversalVTKPlotter::getGridControlEvents() {
  return std::vector< peano4::grid::GridControlEvent >();
}
