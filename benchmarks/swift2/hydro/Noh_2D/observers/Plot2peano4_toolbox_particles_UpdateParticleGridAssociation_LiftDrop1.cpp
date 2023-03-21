#include "Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.h"


tarch::logging::Log benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::_log( "benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1");





benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1(int treeNumber) {

  _treeNumber  = treeNumber;
}


benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::~Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::unprepareTraversal() {

}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::beginTraversal(
      ) {

  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts  = 0;
  _numberOfDrops  = 0;
  _numberOfSieves = 0;
  
  _sieveParticles = vertexdata::hydroPartSet::getSievedParticlesForNextGridTraversal();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::endTraversal(
      ) {

  vertexdata::hydroPartSet::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfSieves
  );

  _sieveParticles.deleteParticles();
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  if ( marker.isParentCellLocal() ) {
    std::bitset<Dimensions> target;
    for (int d=0; d<Dimensions; d++) {
      target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
    }
    _numberOfLifts+=fineGridVertexhydroPartSet.size();
    coarseGridVerticeshydroPartSet( target.to_ulong() ).insert( coarseGridVerticeshydroPartSet( target.to_ulong() ).end(), fineGridVertexhydroPartSet.begin(), fineGridVertexhydroPartSet.end() );
  }
  else {
    fineGridVertexhydroPartSet.particlesCanNotBeLiftedLocally();
  }
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  for (int i=0; i<TwoPowerD; i++) {
    // Define inside for loop, as sieve will define it as well
    vertexdata::hydroPartSet::iterator p = coarseGridVerticeshydroPartSet(i).begin(); 
    while ( p!=coarseGridVerticeshydroPartSet(i).end() ) {
      if ( ::toolbox::particles::dropParticle(**p,marker) ) {
        fineGridVertexhydroPartSet.push_back( *p );
        p = coarseGridVerticeshydroPartSet(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }  
  if ( not _sieveParticles.empty() ) {
    vertexdata::hydroPartSet sievedParticles = _sieveParticles.getParticlesToBeSievedIntoVertex( marker );
      
    _numberOfSieves += sievedParticles.size();

    fineGridVertexhydroPartSet.insert(
      fineGridVertexhydroPartSet.end(),
      sievedParticles.begin(),
      sievedParticles.end()
    );

    fineGridVertexhydroPartSet.eliminateSpatialDuplicates();
  }
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  auto p = fineGridVertexhydroPartSet.begin();
  while (p!=fineGridVertexhydroPartSet.end()) {
    toolbox::particles::ParticleReassociationInstruction instruction = toolbox::particles::liftParticleAssociatedWithVertex(**p, marker);
    switch ( instruction ) {
      case toolbox::particles::ParticleReassociationInstruction_Keep:
        p++;
        break;
      case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
        logDebug( "touchVertexLastTime(...)", "have to lift particle " << (*p)->toString() << " globally. Previously assigned to " << marker.toString() );
        fineGridVertexhydroPartSet.particleCanNotBeLiftedLocally(*p);    
        p = fineGridVertexhydroPartSet.erase(p);
        break;
      case 0:
      case 1:
      case 2:
      case 3:
      #if Dimensions==3
      case 4:
      case 5:
      case 6:
      case 7:
      #endif
        coarseGridVerticeshydroPartSet( instruction ).push_back( *p );
        _numberOfLifts ++;
        p = fineGridVertexhydroPartSet.erase(p);
        break;
      default:
        assertionMsg( false, "value not implemented" );
        break;
    }
  }
}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


