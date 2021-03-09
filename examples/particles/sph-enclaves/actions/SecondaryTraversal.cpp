#include "SecondaryTraversal.h"
#include "../globaldata/Particle.h"
#include "Constants.h"

tarch::logging::Log enclavesph::actions::SecondaryTraversal::_log( "enclavesph::actions::SecondaryTraversal");





enclavesph::actions::SecondaryTraversal::SecondaryTraversal(int treeNumber) {
// @todo Please implement
}


enclavesph::actions::SecondaryTraversal::~SecondaryTraversal() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > enclavesph::actions::SecondaryTraversal::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void enclavesph::actions::SecondaryTraversal::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void enclavesph::actions::SecondaryTraversal::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void enclavesph::actions::SecondaryTraversal::createPersistentVertex(
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


void enclavesph::actions::SecondaryTraversal::destroyPersistentVertex(
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


void enclavesph::actions::SecondaryTraversal::createHangingVertex(
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


void enclavesph::actions::SecondaryTraversal::destroyHangingVertex(
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


void enclavesph::actions::SecondaryTraversal::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

}


void enclavesph::actions::SecondaryTraversal::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  const tarch::la::Vector<Dimensions,double> domainOffset(enclavesph::DOMAIN_OFFSET);
  const tarch::la::Vector<Dimensions,double> domainSize(enclavesph::DOMAIN_SIZE);
  const double timeStepSize = 0.00001;

  for (auto& p: fineGridVertexParticleSet) {
    if (p->getStep() ==  enclavesph::globaldata::Particle::Step::move) {
      p->setStep(enclavesph::globaldata::Particle::Step::calcDensity);

      // Semi-implicit Euler time integration
      p->setV( p->getV() + timeStepSize * p->getA() );
      p->setNewX( p->getX() + timeStepSize * p->getV() ); // Store new position for start of next traversal

      assertion2( std::isfinite(p->getV()(0)), p->toString(), marker.toString() );
      assertion2( std::isfinite(p->getX()(0)), p->toString(), marker.toString() );

      // Update energy per mass
      const double newU = p->getU() + timeStepSize * p->getUDash();
      if (newU < 0.0) {
        p->setU(0.0);
      } else {
        p->setU(newU);
      }
    }
  }

}


void enclavesph::actions::SecondaryTraversal::createCell(
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


void enclavesph::actions::SecondaryTraversal::destroyCell(
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


void enclavesph::actions::SecondaryTraversal::touchCellFirstTime(
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


void enclavesph::actions::SecondaryTraversal::touchCellLastTime(
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
