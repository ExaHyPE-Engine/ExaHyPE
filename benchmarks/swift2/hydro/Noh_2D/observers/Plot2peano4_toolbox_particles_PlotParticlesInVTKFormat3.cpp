#include "Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3.h"


tarch::logging::Log benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::_log( "benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3");



tarch::mpi::BooleanSemaphore  benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::_semaphore("benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3");



benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3(int treeNumber) {

  _writer             = nullptr;
  _positionWriter     = nullptr;
  _cutOffWriter       = nullptr;
  _associationWriter  = nullptr;
  
  _partidWriter = nullptr;
  _vWriter = nullptr;
  _aWriter = nullptr;
  _massWriter = nullptr;
  _densityWriter = nullptr;
  _pressureWriter = nullptr;
  _uWriter = nullptr;
  _smoothingLengthWriter = nullptr;
  _fWriter = nullptr;
  _NcountWriter = nullptr;
  _isBoundaryParticleWriter = nullptr;
  _treeNumber         = treeNumber;

  // An MPI lock (critical section) would be important!

  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );}


benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::~Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3() {
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "particles-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }
  
  
  _writer = new tarch::plotter::pointdata::vtk::VTKWriter(
    false, snapshotFileName.str(), "particles",
    tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::AppendNewData,
    globaldata::hydroPart::getSpecies().getMinTimeStamp()
  );    
  
      
  _positionWriter    = _writer->createPointDataWriter( "x", 3 );
  _cutOffWriter      = _writer->createPointDataWriter( "cut-off-radius", 1 );
  _associationWriter = _writer->createPointDataWriter( "association", 3 );
  
  _partidWriter = _writer->createPointDataWriter( "partid", 1 );
  _vWriter = _writer->createPointDataWriter( "v", 2 );
  _aWriter = _writer->createPointDataWriter( "a", 2 );
  _massWriter = _writer->createPointDataWriter( "mass", 1 );
  _densityWriter = _writer->createPointDataWriter( "density", 1 );
  _pressureWriter = _writer->createPointDataWriter( "pressure", 1 );
  _uWriter = _writer->createPointDataWriter( "u", 1 );
  _smoothingLengthWriter = _writer->createPointDataWriter( "smoothingLength", 1 );
  _fWriter = _writer->createPointDataWriter( "f", 1 );
  _NcountWriter = _writer->createPointDataWriter( "Ncount", 1 );
  _isBoundaryParticleWriter = _writer->createPointDataWriter( "isBoundaryParticle", 1 );
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::endTraversal(
      ) {

  assertion(_positionWriter!=nullptr);

  _positionWriter->close();
  _cutOffWriter->close();
  _associationWriter->close();
  
  _partidWriter->close();
  _vWriter->close();
  _aWriter->close();
  _massWriter->close();
  _densityWriter->close();
  _pressureWriter->close();
  _uWriter->close();
  _smoothingLengthWriter->close();
  _fWriter->close();
  _NcountWriter->close();
  _isBoundaryParticleWriter->close();
  _writer->writeToFile();

  delete _positionWriter;
  delete _cutOffWriter;
  delete _associationWriter;
  
  delete _partidWriter;
  delete _vWriter;
  delete _aWriter;
  delete _massWriter;
  delete _densityWriter;
  delete _pressureWriter;
  delete _uWriter;
  delete _smoothingLengthWriter;
  delete _fWriter;
  delete _NcountWriter;
  delete _isBoundaryParticleWriter;
  delete _writer;

  _positionWriter    = nullptr;
  _cutOffWriter      = nullptr;
  _associationWriter = nullptr;
  
  _partidWriter = nullptr;
  _vWriter = nullptr;
  _aWriter = nullptr;
  _massWriter = nullptr;
  _densityWriter = nullptr;
  _pressureWriter = nullptr;
  _uWriter = nullptr;
  _smoothingLengthWriter = nullptr;
  _fWriter = nullptr;
  _NcountWriter = nullptr;
  _isBoundaryParticleWriter = nullptr;
  _writer            = nullptr;
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  assertion( _positionWriter!=nullptr );
  for (auto* p: fineGridVertexhydroPartSet) {
    if (
      p->getParallelState()==globaldata::hydroPart::ParallelState::Local
    ) {
      int particleNumber = _writer->plotPoint(p->getX());
      _positionWriter->plot(particleNumber,p->getX());
      _cutOffWriter->plot(particleNumber,p->getCutOffRadius());
      _associationWriter->plot(particleNumber,marker.x()-p->getX());
      
    _partidWriter->plot(particleNumber,p->getPartid());
    _vWriter->plot(particleNumber,p->getV());
    _aWriter->plot(particleNumber,p->getA());
    _massWriter->plot(particleNumber,p->getMass());
    _densityWriter->plot(particleNumber,p->getDensity());
    _pressureWriter->plot(particleNumber,p->getPressure());
    _uWriter->plot(particleNumber,p->getU());
    _smoothingLengthWriter->plot(particleNumber,p->getSmoothingLength());
    _fWriter->plot(particleNumber,p->getF());
    _NcountWriter->plot(particleNumber,p->getNcount());
    _isBoundaryParticleWriter->plot(particleNumber,p->getIsBoundaryParticle());
    }
  }
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


