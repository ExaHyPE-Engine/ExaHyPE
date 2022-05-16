#include "Plot.h"
#include "../globaldata/Particle.h"

tarch::logging::Log peanosph::actions::Plot::_log( "peanosph::actions::Plot");





peanosph::actions::Plot::Plot(int treeNumber) {
// @todo Please implement
}


peanosph::actions::Plot::~Plot() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::actions::Plot::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::actions::Plot::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::actions::Plot::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::actions::Plot::createPersistentVertex(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void peanosph::actions::Plot::destroyPersistentVertex(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void peanosph::actions::Plot::createHangingVertex(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void peanosph::actions::Plot::destroyHangingVertex(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void peanosph::actions::Plot::touchVertexFirstTime(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

  for (auto& p: fineGridVertexParticleSet) {
    p->setX( p->getNewX() );
  }

}


void peanosph::actions::Plot::touchVertexLastTime(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void peanosph::actions::Plot::touchCellFirstTime(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void peanosph::actions::Plot::touchCellLastTime(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}

void peanosph::actions::Plot::createCell(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}


void peanosph::actions::Plot::destroyCell(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}
