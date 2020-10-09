#include "VTKWriter.h"
#include "tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter.h"
#include "tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter.h"


tarch::mpi::BooleanSemaphore  tarch::plotter::pointdata::vtk::VTKWriter::_sempahore( "tarch::plotter::pointdata::vtk::VTKTextFileWriter" );


tarch::plotter::pointdata::vtk::VTKWriter::VTKWriter(bool binary, const std::string& indexFile, tarch::plotter::VTUTimeSeriesWriter::IndexFileMode mode) {
  if (binary) {
    _vtkWriter = new tarch::plotter::griddata::unstructured::vtk::VTKBinaryFileWriter(indexFile,mode);
  }
  else {
    _vtkWriter = new tarch::plotter::griddata::unstructured::vtk::VTKTextFileWriter(indexFile,mode);
  }

  _vertexWriter = _vtkWriter->createVertexWriter();
  _cellWriter   = _vtkWriter->createCellWriter();
}



tarch::plotter::pointdata::vtk::VTKWriter::~VTKWriter() {
  assertionMsg( _vertexWriter==nullptr, "no close() on point data writer called" );
  assertionMsg( _cellWriter==nullptr, "no close() on point data writer called" );

  delete _vtkWriter;
}


tarch::plotter::pointdata::PointWriter::PointDataWriter*    tarch::plotter::pointdata::vtk::VTKWriter::createPointDataWriter( const std::string& identifier, int recordsPerPoint ) {
  return new PointDataWriter(*this,identifier,recordsPerPoint);
}


bool tarch::plotter::pointdata::vtk::VTKWriter::writeToFile( const std::string& filename ) {
  if (_vertexWriter!=nullptr) {
    _vertexWriter->close();
    _cellWriter->close();

    delete _vertexWriter;
    delete _cellWriter;

    _vertexWriter = nullptr;
    _cellWriter   = nullptr;
  }

  return _vtkWriter->writeToFile(filename);
}


bool tarch::plotter::pointdata::vtk::VTKWriter::isOpen() {
  return _vtkWriter->isOpen();
}


void tarch::plotter::pointdata::vtk::VTKWriter::clear() {
  _vtkWriter->clear();
}


int tarch::plotter::pointdata::vtk::VTKWriter::plotPoint(const tarch::la::Vector<2,double>& position) {
  const int particleNumber = _vertexWriter->plotVertex(position);
  _cellWriter->plotPoint(particleNumber);
  return particleNumber;
}


int tarch::plotter::pointdata::vtk::VTKWriter::plotPoint(const tarch::la::Vector<3,double>& position) {
  const int particleNumber = _vertexWriter->plotVertex(position);
  _cellWriter->plotPoint(particleNumber);
  return particleNumber;
}


tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::PointDataWriter(VTKWriter& myWriter, const std::string& identifier, int recordsPerPoint):
  _myWriter(myWriter),
  _particleDataWriter(_myWriter._vtkWriter->createVertexDataWriter(identifier,recordsPerPoint)) {
}


tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::~PointDataWriter() {
  assertionMsg( _particleDataWriter==nullptr, "forgot to call close() on PointDataWriter" );
}


void tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::plot( int index, double value ) {
  _particleDataWriter->plotVertex(index,value);
}


void tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::plot( int index, const tarch::la::Vector<2,double>& value ) {
  _particleDataWriter->plotVertex(index,value);
}


void tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::plot( int index, const tarch::la::Vector<3,double>& value ) {
  _particleDataWriter->plotVertex(index,value);
}


void tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::close() {
  if (_myWriter._vertexWriter!=nullptr) {
    _myWriter._vertexWriter->close();
    _myWriter._cellWriter->close();

    delete _myWriter._vertexWriter;
    delete _myWriter._cellWriter;

    _myWriter._vertexWriter = nullptr;
    _myWriter._cellWriter   = nullptr;
  }

  _particleDataWriter->close();
  delete _particleDataWriter;
  _particleDataWriter = nullptr;
}


void tarch::plotter::pointdata::vtk::VTKWriter::PointDataWriter::assignRemainingPointsDefaultValues() {
  _particleDataWriter->assignRemainingVerticesDefaultValues();
}
