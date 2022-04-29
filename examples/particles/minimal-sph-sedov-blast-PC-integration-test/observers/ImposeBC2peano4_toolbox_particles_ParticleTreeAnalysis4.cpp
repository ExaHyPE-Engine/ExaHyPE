#include "ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4.h"


tarch::logging::Log examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::_log( "examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4");





examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4(int treeNumber) {
}


examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::~ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::beginTraversal(
      ) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::endTraversal(
      ) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  fineGridCellParticleSetCellStatistics.setNumberOfParticles(0);
  fineGridCellParticleSetCellStatistics.setParentOfRefinedCell(false);
}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  if (marker.hasBeenRefined()) {
    coarseGridCellParticleSetCellStatistics.setParentOfRefinedCell(true);
  }
  else {
    int count = 0;
    for (int i=0; i<TwoPowerD; i++) {
      count += fineGridVerticesParticleSet(i).size();
    }
    fineGridCellParticleSetCellStatistics.setNumberOfParticles(count);
  }
  
  coarseGridCellParticleSetCellStatistics.setNumberOfParticles(
    coarseGridCellParticleSetCellStatistics.getNumberOfParticles() 
    +
    fineGridCellParticleSetCellStatistics.getNumberOfParticles()
  );
}


