#include "CreateGrid.h"
#include "globaldata/Particle.h"


tarch::logging::Log examples::particles::actions::CreateGrid::_log( "examples::particles::actions::CreateGrid");





examples::particles::actions::CreateGrid::CreateGrid(int treeNumber) {
// @todo Please implement
}


examples::particles::actions::CreateGrid::~CreateGrid() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > examples::particles::actions::CreateGrid::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::actions::CreateGrid::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void examples::particles::actions::CreateGrid::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void examples::particles::actions::CreateGrid::createPersistentVertex(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
  tarch::la::Vector<Dimensions,double>  debugX = marker.x();
  tarch::la::Vector<Dimensions,double>  debugH = marker.h();
  examples::particles::globaldata::Particle::MoveState  moveState( examples::particles::globaldata::Particle::MoveState::New );

  #if Dimensions==2
  const double ParticleDensity = 1000;
  #else
  const double ParticleDensity = 100000;
  #endif
  const int NumberOfParticles = tarch::la::round( tarch::la::volume( marker.h() ) * ParticleDensity );

  for (int i=0; i<NumberOfParticles; i++) {
    tarch::la::Vector<Dimensions,double>  x = marker.x();
    tarch::la::Vector<Dimensions,double>  v;
    for (int d=0; d<Dimensions; d++) {
      x(d) += (static_cast<double>(rand())/static_cast<double>(RAND_MAX)-0.5) * marker.h()(d);
      v(d)  = static_cast<double>(rand())/static_cast<double>(RAND_MAX)-0.5;
    }
    fineGridVertexParticleSet.push_back( examples::particles::globaldata::Particle(debugX,debugH,x,moveState,v) );
  }
}


void examples::particles::actions::CreateGrid::destroyPersistentVertex(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyPersistentVertex()" );
// @todo Please implement
// logTraceOut( "destroyPersistentVertex()" );

}


void examples::particles::actions::CreateGrid::createHangingVertex(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "createHangingVertex()" );
// @todo Please implement
// logTraceOut( "createHangingVertex()" );

}


void examples::particles::actions::CreateGrid::destroyHangingVertex(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyHangingVertex()" );
// @todo Please implement
// logTraceOut( "destroyHangingVertex()" );

}


void examples::particles::actions::CreateGrid::touchVertexFirstTime(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchVertexFirstTime()" );
// @todo Please implement
// logTraceOut( "touchVertexFirstTime()" );

}


void examples::particles::actions::CreateGrid::touchVertexLastTime(
    const peano4::datamanagement::VertexMarker& marker,
    examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchVertexLastTime()" );
// @todo Please implement
// logTraceOut( "touchVertexLastTime()" );

}


void examples::particles::actions::CreateGrid::touchCellFirstTime(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );

}


void examples::particles::actions::CreateGrid::touchCellLastTime(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}



void examples::particles::actions::CreateGrid::createCell(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}



void examples::particles::actions::CreateGrid::destroyCell(
    const peano4::datamanagement::CellMarker& marker,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
    peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );

}


