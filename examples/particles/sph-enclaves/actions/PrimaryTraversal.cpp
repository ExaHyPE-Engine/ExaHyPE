#include "PrimaryTraversal.h"
#include "globaldata/Particle.h"


tarch::logging::Log enclavesph::actions::PrimaryTraversal::_log( "enclavesph::actions::PrimaryTraversal");





enclavesph::actions::PrimaryTraversal::PrimaryTraversal(int treeNumber) {
// @todo Please implement
}


enclavesph::actions::PrimaryTraversal::~PrimaryTraversal() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > enclavesph::actions::PrimaryTraversal::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void enclavesph::actions::PrimaryTraversal::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void enclavesph::actions::PrimaryTraversal::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void enclavesph::actions::PrimaryTraversal::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void enclavesph::actions::PrimaryTraversal::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void enclavesph::actions::PrimaryTraversal::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void enclavesph::actions::PrimaryTraversal::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void enclavesph::actions::PrimaryTraversal::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (auto& p: fineGridVertexParticleSet) {
    p->setX( p->getNewX() );
  }

}


void enclavesph::actions::PrimaryTraversal::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void enclavesph::actions::PrimaryTraversal::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createCell()" );
// @todo Please implement
// logTraceOut( "createCell()" );

}


void enclavesph::actions::PrimaryTraversal::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyCell()" );
// @todo Please implement
// logTraceOut( "destroyCell()" );

}


void enclavesph::actions::PrimaryTraversal::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void enclavesph::actions::PrimaryTraversal::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> fineGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}
