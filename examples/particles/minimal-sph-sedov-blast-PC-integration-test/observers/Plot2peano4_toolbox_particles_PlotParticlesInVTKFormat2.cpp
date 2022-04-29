#include "Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2.h"


tarch::logging::Log examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::_log( "examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2");



tarch::mpi::BooleanSemaphore  examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::_semaphore("examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2");



examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2(int treeNumber) {

  _writer             = nullptr;
  _positionWriter     = nullptr;
  _cutOffWriter       = nullptr;
  _associationWriter  = nullptr;
  
  _vWriter = nullptr;
  _aWriter = nullptr;
  _a_prev_stepWriter = nullptr;
  _densityWriter = nullptr;
  _pressureWriter = nullptr;
  _uWriter = nullptr;
  _massWriter = nullptr;
  _smoothingLengthWriter = nullptr;
  _smoothingLengthPreviousWriter = nullptr;
  _OmegaWriter = nullptr;
  _rho_dhWriter = nullptr;
  _NcountWriter = nullptr;
  _wcountWriter = nullptr;
  _wcount_dhWriter = nullptr;
  _soundSpeedWriter = nullptr;
  _hasConvergedWriter = nullptr;
  _timeWriter = nullptr;
  _timeStepSizeWriter = nullptr;
  _residualWriter = nullptr;
  _iterCountWriter = nullptr;
  _rot_vWriter = nullptr;
  _div_vWriter = nullptr;
  _v_sig_AVWriter = nullptr;
  _balsaraWriter = nullptr;
  _treeNumber         = treeNumber;

  // An MPI lock (critical section) would be important!

  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );}


examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::~Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::beginTraversal(
      ) {

  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "particles-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>0 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }
  
  
  static int timeStep = -1;
  timeStep++;
  _writer = new tarch::plotter::pointdata::vtk::VTKWriter(
    false, snapshotFileName.str(), "particles",
    tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::AppendNewData,
    timeStep
  );
  
      
  _positionWriter    = _writer->createPointDataWriter( "x", 3 );
  _cutOffWriter      = _writer->createPointDataWriter( "cut-off-radius", 1 );
  _associationWriter = _writer->createPointDataWriter( "association", 3 );
  
  _vWriter = _writer->createPointDataWriter( "v", 2 );
  _aWriter = _writer->createPointDataWriter( "a", 2 );
  _a_prev_stepWriter = _writer->createPointDataWriter( "a_prev_step", 2 );
  _densityWriter = _writer->createPointDataWriter( "density", 1 );
  _pressureWriter = _writer->createPointDataWriter( "pressure", 1 );
  _uWriter = _writer->createPointDataWriter( "u", 1 );
  _massWriter = _writer->createPointDataWriter( "mass", 1 );
  _smoothingLengthWriter = _writer->createPointDataWriter( "smoothingLength", 1 );
  _smoothingLengthPreviousWriter = _writer->createPointDataWriter( "smoothingLengthPrevious", 1 );
  _OmegaWriter = _writer->createPointDataWriter( "Omega", 1 );
  _rho_dhWriter = _writer->createPointDataWriter( "rho_dh", 1 );
  _NcountWriter = _writer->createPointDataWriter( "Ncount", 1 );
  _wcountWriter = _writer->createPointDataWriter( "wcount", 1 );
  _wcount_dhWriter = _writer->createPointDataWriter( "wcount_dh", 1 );
  _soundSpeedWriter = _writer->createPointDataWriter( "soundSpeed", 1 );
  _hasConvergedWriter = _writer->createPointDataWriter( "hasConverged", 1 );
  _timeWriter = _writer->createPointDataWriter( "time", 1 );
  _timeStepSizeWriter = _writer->createPointDataWriter( "timeStepSize", 1 );
  _residualWriter = _writer->createPointDataWriter( "residual", 1 );
  _iterCountWriter = _writer->createPointDataWriter( "iterCount", 1 );
  _rot_vWriter = _writer->createPointDataWriter( "rot_v", 1 );
  _div_vWriter = _writer->createPointDataWriter( "div_v", 1 );
  _v_sig_AVWriter = _writer->createPointDataWriter( "v_sig_AV", 1 );
  _balsaraWriter = _writer->createPointDataWriter( "balsara", 1 );
}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::endTraversal(
      ) {

  assertion(_positionWriter!=nullptr);

  _positionWriter->close();
  _cutOffWriter->close();
  _associationWriter->close();
  
  _vWriter->close();
  _aWriter->close();
  _a_prev_stepWriter->close();
  _densityWriter->close();
  _pressureWriter->close();
  _uWriter->close();
  _massWriter->close();
  _smoothingLengthWriter->close();
  _smoothingLengthPreviousWriter->close();
  _OmegaWriter->close();
  _rho_dhWriter->close();
  _NcountWriter->close();
  _wcountWriter->close();
  _wcount_dhWriter->close();
  _soundSpeedWriter->close();
  _hasConvergedWriter->close();
  _timeWriter->close();
  _timeStepSizeWriter->close();
  _residualWriter->close();
  _iterCountWriter->close();
  _rot_vWriter->close();
  _div_vWriter->close();
  _v_sig_AVWriter->close();
  _balsaraWriter->close();
  _writer->writeToFile();

  delete _positionWriter;
  delete _cutOffWriter;
  delete _associationWriter;
  
  delete _vWriter;
  delete _aWriter;
  delete _a_prev_stepWriter;
  delete _densityWriter;
  delete _pressureWriter;
  delete _uWriter;
  delete _massWriter;
  delete _smoothingLengthWriter;
  delete _smoothingLengthPreviousWriter;
  delete _OmegaWriter;
  delete _rho_dhWriter;
  delete _NcountWriter;
  delete _wcountWriter;
  delete _wcount_dhWriter;
  delete _soundSpeedWriter;
  delete _hasConvergedWriter;
  delete _timeWriter;
  delete _timeStepSizeWriter;
  delete _residualWriter;
  delete _iterCountWriter;
  delete _rot_vWriter;
  delete _div_vWriter;
  delete _v_sig_AVWriter;
  delete _balsaraWriter;
  delete _writer;

  _positionWriter    = nullptr;
  _cutOffWriter      = nullptr;
  _associationWriter = nullptr;
  
  _vWriter = nullptr;
  _aWriter = nullptr;
  _a_prev_stepWriter = nullptr;
  _densityWriter = nullptr;
  _pressureWriter = nullptr;
  _uWriter = nullptr;
  _massWriter = nullptr;
  _smoothingLengthWriter = nullptr;
  _smoothingLengthPreviousWriter = nullptr;
  _OmegaWriter = nullptr;
  _rho_dhWriter = nullptr;
  _NcountWriter = nullptr;
  _wcountWriter = nullptr;
  _wcount_dhWriter = nullptr;
  _soundSpeedWriter = nullptr;
  _hasConvergedWriter = nullptr;
  _timeWriter = nullptr;
  _timeStepSizeWriter = nullptr;
  _residualWriter = nullptr;
  _iterCountWriter = nullptr;
  _rot_vWriter = nullptr;
  _div_vWriter = nullptr;
  _v_sig_AVWriter = nullptr;
  _balsaraWriter = nullptr;
  _writer            = nullptr;
}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  assertion( _positionWriter!=nullptr );
  for (auto& p: fineGridVertexParticleSet) {
    if (
      p->getParallelState()==globaldata::Particle::ParallelState::Local
    ) {
      int particleNumber = _writer->plotPoint(p->getX());
      _positionWriter->plot(particleNumber,p->getX());
      _cutOffWriter->plot(particleNumber,p->getCutOffRadius());
      _associationWriter->plot(particleNumber,marker.x()-p->getX());
      
    _vWriter->plot(particleNumber,p->getV());
    _aWriter->plot(particleNumber,p->getA());
    _a_prev_stepWriter->plot(particleNumber,p->getA_prev_step());
    _densityWriter->plot(particleNumber,p->getDensity());
    _pressureWriter->plot(particleNumber,p->getPressure());
    _uWriter->plot(particleNumber,p->getU());
    _massWriter->plot(particleNumber,p->getMass());
    _smoothingLengthWriter->plot(particleNumber,p->getSmoothingLength());
    _smoothingLengthPreviousWriter->plot(particleNumber,p->getSmoothingLengthPrevious());
    _OmegaWriter->plot(particleNumber,p->getOmega());
    _rho_dhWriter->plot(particleNumber,p->getRho_dh());
    _NcountWriter->plot(particleNumber,p->getNcount());
    _wcountWriter->plot(particleNumber,p->getWcount());
    _wcount_dhWriter->plot(particleNumber,p->getWcount_dh());
    _soundSpeedWriter->plot(particleNumber,p->getSoundSpeed());
    _hasConvergedWriter->plot(particleNumber,p->getHasConverged());
    _timeWriter->plot(particleNumber,p->getTime());
    _timeStepSizeWriter->plot(particleNumber,p->getTimeStepSize());
    _residualWriter->plot(particleNumber,p->getResidual());
    _iterCountWriter->plot(particleNumber,p->getIterCount());
    _rot_vWriter->plot(particleNumber,p->getRot_v());
    _div_vWriter->plot(particleNumber,p->getDiv_v());
    _v_sig_AVWriter->plot(particleNumber,p->getV_sig_AV());
    _balsaraWriter->plot(particleNumber,p->getBalsara());
    }
  }
}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


