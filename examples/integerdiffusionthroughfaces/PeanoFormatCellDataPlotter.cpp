#include "PeanoFormatCellDataPlotter.h"
#include "CellData.h"

#include "peano4/utils/Loop.h"


#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"


tarch::multicore::BooleanSemaphore  examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::_semaphore;
int                                 examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::_instanceCounter(0);


examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::PeanoFormatCellDataPlotter(const std::string&  fileNamePrefix, bool plotThroughoutDescent):
  _plotThroughoutDescent(plotThroughoutDescent),
  _fileNamePrefix(fileNamePrefix),
  _counter(0),
  _writer(nullptr),
  _dataWriter(nullptr),
  _oldDataWriter(nullptr) {
  assertionEquals(_instanceCounter,0);
}


examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::~PeanoFormatCellDataPlotter() {
  assertion( _writer == nullptr);
  assertion( _dataWriter == nullptr);
  assertion( _oldDataWriter == nullptr);
}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::beginTraversal() {
  tarch::multicore::Lock lock(_semaphore);
  if (_instanceCounter==0) {
    assertion(_writer==nullptr);

    assertionMsg( false, "not yet implemented" );
/*
 	_writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions,
      _fileNamePrefix,
      // sollte hier den spacetree id abfragen
    );
*/
    _dataWriter      = _writer->createCellDataWriter( "cell-data", 1, 1 );
    _oldDataWriter   = _writer->createCellDataWriter( "old-cell-data", 1, 1 );
  }
  _instanceCounter++;
}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::endTraversal() {
  tarch::multicore::Lock lock(_semaphore);
  _instanceCounter--;

  if (_instanceCounter==0) {
    assertion( _dataWriter!=nullptr );
    assertion( _oldDataWriter!=nullptr );

    _dataWriter->close();
    _oldDataWriter->close();

    std::ostringstream filename;
    filename << _fileNamePrefix << "-" << _counter;
    _writer->writeToFile( filename.str() );

    delete _dataWriter;
    delete _oldDataWriter;
    delete _writer;

    _dataWriter    = nullptr;
    _oldDataWriter = nullptr;
    _writer        = nullptr;

    _counter++;
  }
}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::createPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) {}

void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::createHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::destroyPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::destroyHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::touchFaceFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::touchFaceLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
)  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
)  {}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::plotCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data
) {
  int vertexIndices[TwoPowerD];

  // As we have only one index per patch, we don't have to convert anything
  int indices = _writer->plotPatch(
    center - h * 0.5,
	h
  );

  assertion( _dataWriter!=nullptr );
  assertion( _oldDataWriter!=nullptr );
  _dataWriter->plotCell(indices,data.value);
  _oldDataWriter->plotCell(indices,data.oldValue);
}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
)  {
  if (_plotThroughoutDescent) {
	plotCell(center,h,data);
  }
}


void examples::integerdiffusionthroughfaces::PeanoFormatCellDataPlotter::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
)  {
  if (not _plotThroughoutDescent) {
	plotCell(center,h,data);
  }
}

