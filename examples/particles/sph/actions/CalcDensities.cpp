#include "CalcDensities.h"
#include "globaldata/Particle.h"

tarch::logging::Log peanosph::actions::CalcDensities::_log( "peanosph::actions::CalcDensities");





peanosph::actions::CalcDensities::CalcDensities(int treeNumber) {
// @todo Please implement
}


peanosph::actions::CalcDensities::~CalcDensities() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::actions::CalcDensities::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::actions::CalcDensities::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::actions::CalcDensities::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::actions::CalcDensities::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void peanosph::actions::CalcDensities::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void peanosph::actions::CalcDensities::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void peanosph::actions::CalcDensities::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void peanosph::actions::CalcDensities::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (auto& p: fineGridVertexParticleSet) {
    p->setX( p->getNewX() );
  }

}


void peanosph::actions::CalcDensities::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void peanosph::actions::CalcDensities::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createCell()" );
// @todo Please implement
// logTraceOut( "createCell()" );

}


void peanosph::actions::CalcDensities::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyCell()" );
// @todo Please implement
// logTraceOut( "destroyCell()" );

}


void peanosph::actions::CalcDensities::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void peanosph::actions::CalcDensities::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}
