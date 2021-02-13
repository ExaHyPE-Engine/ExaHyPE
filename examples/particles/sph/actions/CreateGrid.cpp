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

  if (debugH(0) > 0.2) { return; }

  const double h = 0.02;
  const double restDensity = 1;
  const double particleSize = 0.01;
  const double energySpike = 0.1;
  const double spikeRadius = 0.08;

  const double sigma = spikeRadius / 3;
  const double backgroundU = 1e-5 / (restDensity * (peanosph::GAMMA - 1));
  const double particleMass = restDensity * std::pow(particleSize, Dimensions);
  tarch::la::Vector<Dimensions,double> zero(0.0);
  tarch::la::Vector<Dimensions,double> ones(1.0);
  tarch::la::Vector<Dimensions,double> explosionCentre(0.5);
  const int particlesPerAxis = static_cast<int>(marker.h()(0) / particleSize);

  const tarch::la::Vector<Dimensions,double> offset = marker.x() - ((particlesPerAxis-1) * particleSize / 2.0);

  dfor(k, particlesPerAxis) {
    const tarch::la::Vector<Dimensions,double> x = offset + tarch::la::convertScalar<double>(k) * particleSize;

    if (tarch::la::allGreaterEquals(x, zero) && tarch::la::allGreaterEquals(ones, x)) {
        double u = backgroundU;
        // Gaussian distribution of energy around explosion centre.
        const tarch::la::Vector<Dimensions,double> offset = x - explosionCentre;
        if (tarch::la::norm2(offset) < spikeRadius) {
        u = energySpike * (1.0 / std::pow(std::sqrt(2*tarch::la::PI)*sigma, Dimensions)) * std::pow(tarch::la::E, -tarch::la::norm2(offset)*tarch::la::norm2(offset) / (2*sigma*sigma));
        }

        fineGridVertexParticleSet.push_back(
            new peanosph::globaldata::Particle(debugX,debugH,x,moveState,2*h,zero,zero,particleMass,0,0,u,0,h,0,false,0)
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
