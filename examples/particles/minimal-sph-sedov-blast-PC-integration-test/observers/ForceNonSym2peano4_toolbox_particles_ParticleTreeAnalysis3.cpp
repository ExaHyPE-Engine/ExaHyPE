#include "ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3.h"


tarch::logging::Log examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::_log( "examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3");





examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3(int treeNumber) {
}


examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::~ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::beginTraversal(
      ) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::endTraversal(
      ) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  fineGridCellParticleSetCellStatistics.setNumberOfParticles(0);
  fineGridCellParticleSetCellStatistics.setParentOfRefinedCell(false);
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleTreeAnalysis3::touchCellLastTime(
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


