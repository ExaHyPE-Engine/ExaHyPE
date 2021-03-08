#include "PrimaryTraversal.h"
#include "globaldata/Particle.h"


tarch::logging::Log peanosph::enclavesph::actions::PrimaryTraversal::_log( "peanosph::enclavesph::actions::PrimaryTraversal");





peanosph::enclavesph::actions::PrimaryTraversal::PrimaryTraversal(int treeNumber) {
// @todo Please implement
}


peanosph::enclavesph::actions::PrimaryTraversal::~PrimaryTraversal() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::enclavesph::actions::PrimaryTraversal::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::enclavesph::actions::PrimaryTraversal::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (auto& p: fineGridVertexParticleSet) {
    p->setX( p->getNewX() );
  }

  // std::cout << "[actions::PrimaryTraversal::touchVertexFirstTime] x = (" << marker.x()(0) << ", " << marker.x()(1) << ")\n";
  // std::cout << "[actions::PrimaryTraversal::touchVertexFirstTime] taskcounter = " << fineGridVertexVertexTaskCounter.getNumTasksRemaining() << '\n';

}


void peanosph::enclavesph::actions::PrimaryTraversal::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createCell()" );
// @todo Please implement
// logTraceOut( "createCell()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyCell()" );
// @todo Please implement
// logTraceOut( "destroyCell()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void peanosph::enclavesph::actions::PrimaryTraversal::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}
