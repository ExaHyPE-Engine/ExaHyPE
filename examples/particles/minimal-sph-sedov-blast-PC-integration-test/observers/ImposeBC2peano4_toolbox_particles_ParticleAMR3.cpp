#include "ImposeBC2peano4_toolbox_particles_ParticleAMR3.h"


tarch::logging::Log examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::_log( "examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3");



std::vector< peano4::grid::GridControlEvent >  examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::_gridControlEvents;



examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::ImposeBC2peano4_toolbox_particles_ParticleAMR3(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::~ImposeBC2peano4_toolbox_particles_ParticleAMR3() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::getGridControlEvents() const {
return _gridControlEvents;}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::beginTraversal(
      ) {

  _gridControlEvents.clear();

}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::endTraversal(
      ) {

  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock lock(semaphore);

  _gridControlEvents.insert( _gridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  bool refine  = false;
  bool erase   = false;
  if (
    not coarseGridCellParticleSetCellStatistics.getParentOfRefinedCell()
    and
    fineGridCellParticleSetCellStatistics.getNumberOfParticles() > 65536
  ) {
   refine = true;
  }

  if (
    not refine
    and
    not coarseGridCellParticleSetCellStatistics.getParentOfRefinedCell()
    and
    fineGridCellParticleSetCellStatistics.getNumberOfParticles() > 10
  ) {
    double h = std::numeric_limits<double>::max();

    for (int i=0; i<TwoPowerD; i++) {
      h = std::min( h, fineGridVerticesParticleSet(i).getMinCutOffRadius() );
    }

    if (h < tarch::la::min(marker.h())/3.0 ) {
      refine = true;
    }
  }

  if (
    not refine
    and
    not fineGridCellParticleSetCellStatistics.getParentOfRefinedCell()
    and
    fineGridCellParticleSetCellStatistics.getNumberOfParticles() <= 10
  ) {
    erase = true;
  }

  if (refine) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h()/3.0 );
    _localGridControlEvents.push_back(newEntry);
  }
  else if (erase) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h() );
    _localGridControlEvents.push_back(newEntry);
  }
}


