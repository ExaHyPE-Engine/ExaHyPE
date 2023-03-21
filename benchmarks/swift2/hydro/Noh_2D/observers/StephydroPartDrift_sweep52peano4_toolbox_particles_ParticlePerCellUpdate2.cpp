#include "StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::_log( "benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2");





benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::~StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::prepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::unprepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  ::swift2::timestepping::resetMovedParticleMarker(localParticles);
  
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  ::swift2::timestepping::computeLeapfrogDriftWithGlobalTimeStepSize( localParticles );
  
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
}


void benchmarks::swift2::noh::observers::StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


