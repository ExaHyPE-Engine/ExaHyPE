#include "MoveParticles.h"
#include "../globaldata/Particle.h"


tarch::logging::Log examples::particles::actions::MoveParticles::_log( "examples::particles::actions::MoveParticles");





examples::particles::actions::MoveParticles::MoveParticles(int treeNumber) {
// @todo Please implement
}


examples::particles::actions::MoveParticles::~MoveParticles() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > examples::particles::actions::MoveParticles::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::actions::MoveParticles::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void examples::particles::actions::MoveParticles::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void examples::particles::actions::MoveParticles::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "createPersistentVertex()" );
// @todo Please implement
// logTraceOut( "createPersistentVertex()" );

}


void examples::particles::actions::MoveParticles::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void examples::particles::actions::MoveParticles::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void examples::particles::actions::MoveParticles::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void examples::particles::actions::MoveParticles::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchVertexFirstTime()" );
// @todo Please implement
// logTraceOut( "touchVertexFirstTime()" );

}


void examples::particles::actions::MoveParticles::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

  const double timeStepSize = 0.0001;
  for (auto& p: fineGridVertexParticleSet) {
    p.setX( p.getX() + timeStepSize * p.getV() );
  }

}


void examples::particles::actions::MoveParticles::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void examples::particles::actions::MoveParticles::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}


