#include "MoveParticles.h"
#include "../globaldata/Particle.h"


tarch::logging::Log peanosph::actions::MoveParticles::_log( "peanosph::actions::MoveParticles");





peanosph::actions::MoveParticles::MoveParticles(int treeNumber) {
// @todo Please implement
}


peanosph::actions::MoveParticles::~MoveParticles() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::actions::MoveParticles::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::actions::MoveParticles::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::actions::MoveParticles::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::actions::MoveParticles::createPersistentVertex(
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


void peanosph::actions::MoveParticles::destroyPersistentVertex(
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


void peanosph::actions::MoveParticles::createHangingVertex(
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


void peanosph::actions::MoveParticles::destroyHangingVertex(
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


void peanosph::actions::MoveParticles::touchVertexFirstTime(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchVertexFirstTime()" );
// @todo Please implement
// logTraceOut( "touchVertexFirstTime()" );

}


void peanosph::actions::MoveParticles::touchVertexLastTime(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

  const double timeStepSize = 0.00001;
  for (auto& p: fineGridVertexParticleSet) {
    if (p->getPassesSincePositionUpdate() == 1) {
      p->setPassesSincePositionUpdate(0);

      // Semi-implicit Euler time integration
      p->setV( p->getV() + timeStepSize * p->getA() );
      p->setX( p->getX() + timeStepSize * p->getV() );

      assertion2( std::isfinite(p->getV()(0)), p->toString(), marker.toString() );
      assertion2( std::isfinite(p->getX()(0)), p->toString(), marker.toString() );

      // Update energy per mass
      const double newU = p->getU() + timeStepSize * p->getUDash();
      if (newU < 0.0) {
        p->setU(0.0);
      } else {
        p->setU(newU);
      }

      // Enforce periodic boundary conditions
      for (int d=0; d<Dimensions; d++) {
        if ( p->getX()(d)<0.0 ) {
          p->setX(d, p->getX()(d) + 1);
        }
        if ( p->getX()(d)>1.0 ) {
          p->setX(d, p->getX()(d) - 1);
        }
      }

    } else {
      p->setPassesSincePositionUpdate(p->getPassesSincePositionUpdate() + 1);
    }
  }

}


void peanosph::actions::MoveParticles::touchCellFirstTime(
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


void peanosph::actions::MoveParticles::touchCellLastTime(
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


void peanosph::actions::MoveParticles::createCell(
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


void peanosph::actions::MoveParticles::destroyCell(
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
