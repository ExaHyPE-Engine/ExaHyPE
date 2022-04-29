#include "Plot2peano4_toolbox_particles_ParticleTreeAnalysis0.h"


tarch::logging::Log examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::_log( "examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0");





examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0(int treeNumber) {
}


examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::~Plot2peano4_toolbox_particles_ParticleTreeAnalysis0() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::beginTraversal(
      ) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::endTraversal(
      ) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  fineGridCellParticleSetCellStatistics.setNumberOfParticles(0);
  fineGridCellParticleSetCellStatistics.setParentOfRefinedCell(false);
}


void examples::particles::observers::Plot2peano4_toolbox_particles_ParticleTreeAnalysis0::touchCellLastTime(
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


