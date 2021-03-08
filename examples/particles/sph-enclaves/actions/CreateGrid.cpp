#include "CreateGrid.h"
#include "globaldata/Particle.h"
#include "Constants.h"
#include "peano4/utils/Loop.h"


tarch::logging::Log peanosph::enclavesph::actions::CreateGrid::_log( "peanosph::enclavesph::actions::CreateGrid");





peanosph::enclavesph::actions::CreateGrid::CreateGrid(int treeNumber) {
// @todo Please implement
}


peanosph::enclavesph::actions::CreateGrid::~CreateGrid() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > peanosph::enclavesph::actions::CreateGrid::getGridControlEvents() const {
// @todo Please implement
return std::vector< peano4::grid::GridControlEvent >();
}


void peanosph::enclavesph::actions::CreateGrid::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );

}


void peanosph::enclavesph::actions::CreateGrid::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );

}


void peanosph::enclavesph::actions::CreateGrid::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  tarch::la::Vector<Dimensions,double>  debugX = marker.x();
  tarch::la::Vector<Dimensions,double>  debugH = marker.h();
  peanosph::enclavesph::globaldata::Particle::MoveState  moveState( peanosph::enclavesph::globaldata::Particle::MoveState::New );

  if (debugH(0) > 0.13) { return; }

  const tarch::la::Vector<Dimensions,double> zero(0.0);
  const tarch::la::Vector<Dimensions,double> domainOffset(peanosph::enclavesph::DOMAIN_OFFSET);
  const tarch::la::Vector<Dimensions,double> domainSize(peanosph::enclavesph::DOMAIN_SIZE);
  const tarch::la::Vector<Dimensions,double> explosionCentre(peanosph::enclavesph::EXPLOSION_CENTRE);

  const double sigma = peanosph::enclavesph::SPIKE_RADIUS / 3;
  const double backgroundU = 1e-5 / (peanosph::enclavesph::REST_DENSITY * (peanosph::enclavesph::GAMMA - 1));
  const int particlesPerAxis = static_cast<int>(marker.h()(0) / peanosph::enclavesph::TARGET_PARTICLE_SIZE);

  // Calculate actual particle size and mass
  const double empty = marker.h()(0) - (particlesPerAxis * peanosph::enclavesph::TARGET_PARTICLE_SIZE);
  const double particleSize = peanosph::enclavesph::TARGET_PARTICLE_SIZE + (empty / particlesPerAxis);
  const double particleMass = peanosph::enclavesph::REST_DENSITY * std::pow(particleSize, Dimensions);

  const tarch::la::Vector<Dimensions,double> offset = marker.x() - ((particlesPerAxis-1) * particleSize / 2.0);

  dfor(k, particlesPerAxis) {
    const tarch::la::Vector<Dimensions,double> x = offset + tarch::la::convertScalar<double>(k) * particleSize;
    // const tarch::la::Vector<Dimensions,double> x = offset + tarch::la::convertScalar<double>(k) * (particleSize + gap);

    if (tarch::la::allGreaterEquals(x, domainOffset) && tarch::la::allGreaterEquals((domainOffset+domainSize), x)) {
      double u = backgroundU;
      // Gaussian distribution of energy around explosion centre.
      const tarch::la::Vector<Dimensions,double> offset = x - explosionCentre;
      if (tarch::la::norm2(offset) < peanosph::enclavesph::SPIKE_RADIUS) {
        u = peanosph::enclavesph::ENERGY_SPIKE * (1.0 / std::pow(std::sqrt(2*tarch::la::PI)*sigma, Dimensions)) * std::pow(tarch::la::E, -tarch::la::norm2(offset)*tarch::la::norm2(offset) / (2*sigma*sigma));
      }

      fineGridVertexParticleSet.push_back(
          new peanosph::enclavesph::globaldata::Particle(
            debugX,
            debugH,
            x,
            moveState,
            2*peanosph::enclavesph::INITIAL_H,
            zero,
            zero,
            particleMass,
            0,
            0,
            u,
            0,
            peanosph::enclavesph::INITIAL_H,
            peanosph::enclavesph::globaldata::Particle::Step::calcDensity,
            x
          )
      );
    }
  }

}


void peanosph::enclavesph::actions::CreateGrid::destroyPersistentVertex(
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


void peanosph::enclavesph::actions::CreateGrid::createHangingVertex(
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


void peanosph::enclavesph::actions::CreateGrid::destroyHangingVertex(
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


void peanosph::enclavesph::actions::CreateGrid::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      peanosph::enclavesph::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peanosph::enclavesph::vertexdata::VertexTaskCounter& fineGridVertexVertexTaskCounter,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter> coarseGridVerticesVertexTaskCounter,
      peanosph::enclavesph::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  // fineGridVertexVertexTaskCounter.setNumTasksRemaining(0);

}


void peanosph::enclavesph::actions::CreateGrid::touchVertexLastTime(
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


void peanosph::enclavesph::actions::CreateGrid::createCell(
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


void peanosph::enclavesph::actions::CreateGrid::destroyCell(
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


void peanosph::enclavesph::actions::CreateGrid::touchCellFirstTime(
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


void peanosph::enclavesph::actions::CreateGrid::touchCellLastTime(
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
