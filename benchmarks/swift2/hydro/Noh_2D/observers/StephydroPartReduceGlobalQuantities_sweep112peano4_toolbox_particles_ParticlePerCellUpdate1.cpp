#include "StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::_log( "benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1");





benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::~StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::prepareTraversal() {

                                         globaldata::hydroPart::getSpecies().clearCutOffRadius();
                                         globaldata::hydroPart::getSpecies().clearVelocity();
                                         }


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::unprepareTraversal() {

                                         globaldata::hydroPart::getSpecies().setTimeStamp( globaldata::hydroPart::getSpecies().getMinTimeStamp() + globaldata::hydroPart::getSpecies().getMinTimeStepSize(), false );
                                         ::swift2::statistics::reportCutOffRadiusVTDt<globaldata::hydroPart>( "hydroPart" );
                                         }


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::beginTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::endTraversal(
      ) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  
  std::list< globaldata::hydroPart* >   localParticles;
  std::list< globaldata::hydroPart* >  activeParticles;
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVerticeshydroPartSet(i) ) {
      if ( marker.isContained( p->getX() ) ) {
        activeParticles.push_front( p );
      }
    }
    for (auto* p: fineGridVerticeshydroPartSet(i) ) {
      if ( marker.isContained( p->getX() ) ) {
        localParticles.push_front( p );
      }
    }
  }

  ::swift2::statistics::reduceVelocityAndCutOffRadius( localParticles );
  
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


