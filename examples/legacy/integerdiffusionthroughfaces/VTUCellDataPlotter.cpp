#include "VTUCellDataPlotter.h"
#include "CellData.h"

#include "peano4/utils/Loop.h"


examples::integerdiffusionthroughfaces::VTUCellDataPlotter::VTUCellDataPlotter(
  const std::string&  fileNamePrefix, bool plotThroughoutDescent
):
  _plotThroughoutDescent(plotThroughoutDescent),
  _fileNamePrefix(fileNamePrefix),
  _counter(0),
  _writer(nullptr),
  _vertexWriter(nullptr),
  _cellWriter(nullptr),
  _dataWriter(nullptr),
  _timeSeriesWriter() {
}


examples::integerdiffusionthroughfaces::VTUCellDataPlotter::~VTUCellDataPlotter() {
  assertion( _writer == nullptr);
  assertion( _vertexWriter == nullptr);
  assertion( _cellWriter == nullptr);
  assertion( _dataWriter == nullptr);
}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::beginTraversal() {
  _writer       = new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter();
  _vertexWriter = _writer->createVertexWriter();
  _cellWriter   = _writer->createCellWriter();
  _dataWriter   = _writer->createCellDataWriter( "cell-data", 1 );
}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::endTraversal() {
  assertion( _vertexWriter!=nullptr );
  assertion( _cellWriter!=nullptr );
  assertion( _dataWriter!=nullptr );

  _vertexWriter->close();
  _cellWriter->close();
  _dataWriter->close();

  std::ostringstream filename;
  filename << _fileNamePrefix << "-" << _counter;
  _writer->writeToFile( filename.str() );

  delete _vertexWriter;
  delete _cellWriter;
  delete _dataWriter;
  delete _writer;

  _vertexWriter = 0;
  _cellWriter = 0;
  _dataWriter = 0;
  _writer = 0;

  _timeSeriesWriter.addSnapshot( filename.str(), _counter, false );
  _timeSeriesWriter.writeFile( "data" );

  _counter++;
}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::createPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) {}

void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::createHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::destroyPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::destroyHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::touchFaceFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::touchFaceLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    )  {}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
)  {
  assertion(false);
/*
  if (not marker.isRefined and _plotThroughoutDescent) {
	plotCell( center, h, data );
  }
*/
}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::plotCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data
) {
  int vertexIndices[TwoPowerD];

  dfor2(k)
    assertion( _vertexWriter!=nullptr );
    vertexIndices[kScalar] = _vertexWriter->plotVertex(
      center + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(k), h ) - h * 0.5
    );
  enddforx

  assertion( _cellWriter!=nullptr );
  int cellIndex = -1;
  #if Dimensions==2
  cellIndex = _cellWriter->plotQuadrangle(vertexIndices);
  #elif Dimensions==3
  cellIndex = _cellWriter->plotHexahedron(vertexIndices);
  #else
  assertionMsg( false, "supports only 2d and 3d" );
  #endif

  assertion( _dataWriter!=nullptr );
  _dataWriter->plotCell(cellIndex,data.value);
}


void examples::integerdiffusionthroughfaces::VTUCellDataPlotter::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
)  {
  assertion(false);
/*
  if (not marker.isRefined and not _plotThroughoutDescent) {
	plotCell( center, h, data );
  }
*/
}

