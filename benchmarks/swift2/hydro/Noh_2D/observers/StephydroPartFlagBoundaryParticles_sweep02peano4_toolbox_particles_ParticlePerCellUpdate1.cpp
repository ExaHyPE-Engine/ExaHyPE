#include "StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::_log( "benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1");





benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::~StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::prepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::unprepareTraversal() {
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  
                                         
  
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  auto& localParticles  = fineGridVertexhydroPartSet;
  auto& activeParticles = fineGridVertexhydroPartSet;

  ::swift2::kernels::flagBoundaryParticles(localParticles );
  
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
}


void benchmarks::swift2::noh::observers::StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


