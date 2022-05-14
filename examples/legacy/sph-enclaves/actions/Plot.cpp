#include "Plot.h"
#include "globaldata/Particle.h"


tarch::logging::Log enclavesph::actions::Plot::_log( "enclavesph::actions::Plot");





enclavesph::actions::Plot::Plot(int treeNumber) {
// @todo Please implement
}


enclavesph::actions::Plot::~Plot() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > enclavesph::actions::Plot::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void enclavesph::actions::Plot::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void enclavesph::actions::Plot::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void enclavesph::actions::Plot::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void enclavesph::actions::Plot::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void enclavesph::actions::Plot::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void enclavesph::actions::Plot::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void enclavesph::actions::Plot::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

  for (auto& p: fineGridVertexParticleSet) {
    p->setX( p->getNewX() );
  }

}


void enclavesph::actions::Plot::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexLabel& fineGridVertexVertexLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void enclavesph::actions::Plot::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> fineGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& fineGridCellCellTaskLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createCell()" );
// @todo Please implement
// logTraceOut( "createCell()" );

}


void enclavesph::actions::Plot::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> fineGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& fineGridCellCellTaskLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyCell()" );
// @todo Please implement
// logTraceOut( "destroyCell()" );

}


void enclavesph::actions::Plot::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> fineGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& fineGridCellCellTaskLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void enclavesph::actions::Plot::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> fineGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& fineGridCellCellTaskLabel,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexLabel> coarseGridVerticesVertexLabel,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics,
      enclavesph::celldata::CellTaskLabel& coarseGridCellCellTaskLabel) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}
