#include "CreateGrid.h"
#include "globaldata/Particle.h"
#include "Constants.h"
#include "peano4/utils/Loop.h"


tarch::logging::Log peanosph::actions::CreateGrid::_log( "peanosph::actions::CreateGrid");





peanosph::actions::CreateGrid::CreateGrid(int treeNumber) {
// @todo Please implement
}


peanosph::actions::CreateGrid::~CreateGrid() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::actions::CreateGrid::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::actions::CreateGrid::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::actions::CreateGrid::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::actions::CreateGrid::createPersistentVertex(
    const peano4::datamanagement::VertexMarker& marker,
    peanosph::vertexdata::ParticleSet& fineGridVertexParticleSet,
    peano4::datamanagement::VertexEnumerator<peanosph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
    peanosph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics
) {
  tarch::la::Vector<Dimensions,double>  debugX = marker.x();
  tarch::la::Vector<Dimensions,double>  debugH = marker.h();
  peanosph::globaldata::Particle::MoveState  moveState( peanosph::globaldata::Particle::MoveState::New );

  if (debugH(0) > 0.13) { return; }

  const tarch::la::Vector<Dimensions,double> zero(0.0);
  const tarch::la::Vector<Dimensions,double> domainOffset(peanosph::DOMAIN_OFFSET);
  const tarch::la::Vector<Dimensions,double> domainSize(peanosph::DOMAIN_SIZE);
  const tarch::la::Vector<Dimensions,double> explosionCentre(peanosph::EXPLOSION_CENTRE);

  const double sigma = peanosph::SPIKE_RADIUS / 3;
  const double backgroundU = 1e-5 / (peanosph::REST_DENSITY * (peanosph::GAMMA - 1));
  const int particlesPerAxis = static_cast<int>(marker.h()(0) / peanosph::TARGET_PARTICLE_SIZE);

  // Calculate actual particle size and mass
  const double empty = marker.h()(0) - (particlesPerAxis * peanosph::TARGET_PARTICLE_SIZE);
  const double particleSize = peanosph::TARGET_PARTICLE_SIZE + (empty / particlesPerAxis);
  const double particleMass = peanosph::REST_DENSITY * std::pow(particleSize, Dimensions);

  const tarch::la::Vector<Dimensions,double> offset = marker.x() - ((particlesPerAxis-1) * particleSize / 2.0);

  dfor(k, particlesPerAxis) {
    const tarch::la::Vector<Dimensions,double> x = offset + tarch::la::convertScalar<double>(k) * particleSize;
    // const tarch::la::Vector<Dimensions,double> x = offset + tarch::la::convertScalar<double>(k) * (particleSize + gap);

    if (tarch::la::allGreaterEquals(x, domainOffset) && tarch::la::allGreaterEquals((domainOffset+domainSize), x)) {
      double u = backgroundU;
      // Gaussian distribution of energy around explosion centre.
      const tarch::la::Vector<Dimensions,double> offset = x - explosionCentre;
      if (tarch::la::norm2(offset) < peanosph::SPIKE_RADIUS) {
        u = peanosph::ENERGY_SPIKE * (1.0 / std::pow(std::sqrt(2*tarch::la::PI)*sigma, Dimensions)) * std::pow(tarch::la::E, -tarch::la::norm2(offset)*tarch::la::norm2(offset) / (2*sigma*sigma));
      }

      fineGridVertexParticleSet.push_back(
          new peanosph::globaldata::Particle(
            debugX,
            debugH,
            x,
            moveState,
            2*peanosph::INITIAL_H,
            zero,
            zero,
            particleMass,
            0,
            0,
            u,
            0,
            peanosph::INITIAL_H,
            peanosph::globaldata::Particle::Step::calcDensity,
            x
          )
      );
    }
  }
}


void peanosph::actions::CreateGrid::destroyPersistentVertex(
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


void peanosph::actions::CreateGrid::createHangingVertex(
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


void peanosph::actions::CreateGrid::destroyHangingVertex(
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


void peanosph::actions::CreateGrid::touchVertexFirstTime(
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


void peanosph::actions::CreateGrid::touchVertexLastTime(
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


void peanosph::actions::CreateGrid::touchCellFirstTime(
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


void peanosph::actions::CreateGrid::touchCellLastTime(
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



void peanosph::actions::CreateGrid::createCell(
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



void peanosph::actions::CreateGrid::destroyCell(
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
