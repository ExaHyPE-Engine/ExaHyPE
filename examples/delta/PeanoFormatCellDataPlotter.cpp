#include "PeanoFormatCellDataPlotter.h"
#include "CellData.h"

#include "peano4/utils/Loop.h"


#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"


tarch::multicore::BooleanSemaphore  examples::delta::PeanoFormatCellDataPlotter::_semaphore;
int                                 examples::delta::PeanoFormatCellDataPlotter::_instanceCounter(0);


examples::delta::PeanoFormatCellDataPlotter::PeanoFormatCellDataPlotter(const std::string&  fileNamePrefix, bool plotThroughoutDescent):
  _plotThroughoutDescent(plotThroughoutDescent),
  _fileNamePrefix(fileNamePrefix),
  _counter(0),
  _writer(nullptr),
  _dataWriter(nullptr),
  _spacetreeWriter(nullptr) {
  assertionEquals(_instanceCounter,0);
}


examples::delta::PeanoFormatCellDataPlotter::~PeanoFormatCellDataPlotter() {
  assertion( _writer == nullptr);
  assertion( _dataWriter == nullptr);
  assertion( _spacetreeWriter == nullptr);
}


void examples::delta::PeanoFormatCellDataPlotter::beginTraversal() {
  tarch::multicore::Lock lock(_semaphore);
  if (_instanceCounter==0) {
    assertion(_writer==nullptr);

    assertion(false);

/*
 	_writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions,
      _fileNamePrefix
      // sollte hier den spacetree id abfragen
    );
*/

    _dataWriter      = _writer->createCellDataWriter( "cell-data", CellData::DoFsPerAxis, Dimensions );
    #if PeanoDebug>0
    _spacetreeWriter = _writer->createCellDataWriter( "spacetreeId", 1, 1 );
    #endif
  }
  _instanceCounter++;
}


void examples::delta::PeanoFormatCellDataPlotter::endTraversal() {
  tarch::multicore::Lock lock(_semaphore);
  _instanceCounter--;

  if (_instanceCounter==0) {
    assertion( _dataWriter!=nullptr );

    #if PeanoDebug>0
    _spacetreeWriter->close();
    #endif

    _dataWriter->close();

    std::ostringstream filename;
    filename << _fileNamePrefix << "-" << _counter;
    _writer->writeToFile( filename.str() );

    delete _dataWriter;
    delete _writer;

    _dataWriter      = nullptr;
    _spacetreeWriter = nullptr;
    _writer          = nullptr;

    _counter++;
  }
}


void examples::delta::PeanoFormatCellDataPlotter::createCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {}


void examples::delta::PeanoFormatCellDataPlotter::destroyCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {}


void examples::delta::PeanoFormatCellDataPlotter::plotCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data
) {
  int vertexIndices[TwoPowerD];

  int patchIndex = _writer->plotPatch(center - h * 0.5, h);

  #if PeanoDebug>0
  assertion( _spacetreeWriter!=nullptr );
  int spacetreeCellIndex = _spacetreeWriter->getFirstCellWithinPatch(patchIndex);
  assertionEquals( spacetreeCellIndex, patchIndex );
  _spacetreeWriter->plotCell(spacetreeCellIndex,data.spacetreeId);
  #endif

  assertion( _dataWriter!=nullptr );
  int cellIndex = _dataWriter->getFirstCellWithinPatch(patchIndex);

  int subCellCounter = 0;

  dfor(k,CellData::DoFsPerAxis) {
    _dataWriter->plotCell(cellIndex,data.valueX[subCellCounter]);
    _dataWriter->plotCell(cellIndex,data.valueY[subCellCounter]);
    #if Dimensions==3
    _dataWriter->plotCell(cellIndex,data.valueZ[subCellCounter]);
    #endif
    cellIndex++;
    subCellCounter++;
  }
}


void examples::delta::PeanoFormatCellDataPlotter::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  if (_plotThroughoutDescent) {
    plotCell(center,h,data);
  }
}


void examples::delta::PeanoFormatCellDataPlotter::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  if (not _plotThroughoutDescent) {
    plotCell(center,h,data);
  }
}

