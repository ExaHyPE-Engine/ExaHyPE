#include "Move.h"
#include "globaldata/Particle.h"
#include "peano4/utils/Loop.h"


tarch::logging::Log examples::particles::actions::Move::_log( "examples::particles::actions::Move");


examples::particles::actions::Move::Move(int treeNumber) {
}


examples::particles::actions::Move::~Move() {
}


std::vector< peano4::grid::GridControlEvent > examples::particles::actions::Move::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::actions::Move::beginTraversal() {
}


void examples::particles::actions::Move::endTraversal() {
}


void examples::particles::actions::Move::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
  for (auto& p: fineGridVertexParticleSet) {
    p->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
  }
}


void examples::particles::actions::Move::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
  const double timeStepSize = 0.0001;
  for (auto& p: fineGridVertexParticleSet) {
    if (
      p->getMoveState()!=globaldata::Particle::MoveState::Moved
      and
      p->getParallelState()==globaldata::Particle::ParallelState::Local
    ) {
      p->setX( p->getX() + timeStepSize * p->getV() );
      for (int d=0; d<Dimensions; d++) {
        if ( p->getX()(d)<0.0 ) {
          p->setV(d, std::abs(p->getV()(d)));
        }
        if ( p->getX()(d)>1.0 ) {
          p->setV(d, -std::abs(p->getV()(d)));
        }
      }
    }
    p->setMoveState( globaldata::Particle::MoveState::Moved );
  }
}


void examples::particles::actions::Move::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


void examples::particles::actions::Move::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
  // We collect all the particles associated to the 2^d adjacent
  // vertices to a local set. Likely not the most efficient way
  // to do things, but it works.
  examples::particles::vertexdata::ParticleSet particlesInAndAroundCell;
  for (int k=0; k<TwoPowerD; k++) {
    particlesInAndAroundCell.insert(particlesInAndAroundCell.begin(),fineGridVerticesParticleSet(k).begin(),fineGridVerticesParticleSet(k).end());
  }

  // Loop over all particles. If a particle is contained within the
  // cell, loop over all others again and compute "forces".
  for (auto& p: particlesInAndAroundCell) {
    if ( marker.isContained( p->getX() ) ) {
      for (auto& pp: particlesInAndAroundCell) {
        tarch::la::Vector<Dimensions,double> dist = pp->getX() - p->getX();
        if ( tarch::la::greater( tarch::la::norm2(dist),0.0 ) ) {
          const double mass1 = p->getCutOffRadius();   // made-up hack, no real physics. But illustrates principle
          const double mass2 = pp->getCutOffRadius();
          double forceQuantity = mass1 * mass2 / tarch::la::norm2(dist) / tarch::la::norm2(dist);
          p->setV(
            p->getV() + 0.001 * forceQuantity / tarch::la::norm2(dist) * dist
          );
        }
      }
    }
  }
}


void examples::particles::actions::Move::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {
}


