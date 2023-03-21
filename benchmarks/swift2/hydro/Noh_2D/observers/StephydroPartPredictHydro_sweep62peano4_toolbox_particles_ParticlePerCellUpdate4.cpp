#include "StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::_log( "benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4");





benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::~StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::prepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::unprepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  ::swift2::timestepping::resetMovedParticleMarker(localParticles);
  
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  ::swift2::timestepping::computePredictHydro(
                        localParticles, 2 );
  
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


