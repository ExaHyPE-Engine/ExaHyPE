#include "Init2peano4_toolbox_particles_ParticleTreeAnalysis2.h"


tarch::logging::Log examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::_log( "examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2");





examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::Init2peano4_toolbox_particles_ParticleTreeAnalysis2(int treeNumber) {
}


examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::~Init2peano4_toolbox_particles_ParticleTreeAnalysis2() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::beginTraversal(
      ) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::endTraversal(
      ) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  fineGridCellParticleSetCellStatistics.setNumberOfParticles(0);
  fineGridCellParticleSetCellStatistics.setParentOfRefinedCell(false);
}


void examples::particles::observers::Init2peano4_toolbox_particles_ParticleTreeAnalysis2::touchCellLastTime(
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


