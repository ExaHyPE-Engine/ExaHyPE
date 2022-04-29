#include "NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1.h"


tarch::logging::Log examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::_log( "examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1");





examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1(int treeNumber) {

  _treeNumber  = treeNumber;
}


examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::~NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::beginTraversal(
      ) {

  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts = 0;
  _numberOfDrops = 0;
  _numberOfParticleAccesses = 0;
  _numberOfLocal = 0;
  _numberOfHalo = 0;
  _numberOfEliminations = 0;
  _numberOfLeavingParticles = 0;
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::endTraversal(
      ) {

  logInfo( 
    "endTraversal()", 
    "#reassignments/lifts/drops/accesses/local/halo/leaving/eliminated-duplicates=" << 
    _numberOfParticleReassignmentsOnCurrentLevel << "/" <<
    _numberOfLifts << "/" <<
    _numberOfDrops << "/" <<
    _numberOfParticleAccesses << "/" <<
    _numberOfLocal << "/" <<
    _numberOfHalo << "/" <<
    _numberOfLeavingParticles << "/" <<
    _numberOfEliminations << " on tree " <<  _treeNumber
  );
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  std::bitset<Dimensions> target;
  for (int d=0; d<Dimensions; d++) {
    target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
  }
  coarseGridVerticesParticleSet( target.to_ulong() ).insert( coarseGridVerticesParticleSet( target.to_ulong() ).begin(), fineGridVertexParticleSet.begin(), fineGridVertexParticleSet.end() );
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  const double CellToCutOffRadiusRatio = 0.5 * 0.9;
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::ParticleSet::iterator p = coarseGridVerticesParticleSet(i).begin(); 
    while ( p!=coarseGridVerticesParticleSet(i).end() ) {
      bool dropIntoThisVertex = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius();
      for (int d=0; d<Dimensions; d++) {
        dropIntoThisVertex &= (*p)->getX()(d) > marker.x()(d) - marker.h()(d)/2.0;
        dropIntoThisVertex &= (*p)->getX()(d) < marker.x()(d) + marker.h()(d)/2.0;
      }
      
      if (dropIntoThisVertex) {
        fineGridVertexParticleSet.push_back( *p );
        p = coarseGridVerticesParticleSet(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  std::bitset<Dimensions> target;
  for (int d=0; d<Dimensions; d++) {
    target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
  }
  coarseGridVerticesParticleSet( target.to_ulong() ).insert( coarseGridVerticesParticleSet( target.to_ulong() ).begin(), fineGridVertexParticleSet.begin(), fineGridVertexParticleSet.end() );
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (auto& p: fineGridVertexParticleSet ) {
    p->setNewParallelState( globaldata::Particle::NewParallelState::Halo );
  }
  const double CellToCutOffRadiusRatio = 0.5 * 0.9;
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::ParticleSet::iterator p = coarseGridVerticesParticleSet(i).begin(); 
    while ( p!=coarseGridVerticesParticleSet(i).end() ) {
      bool dropIntoThisVertex = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius();
      for (int d=0; d<Dimensions; d++) {
        dropIntoThisVertex &= (*p)->getX()(d) > marker.x()(d) - marker.h()(d)/2.0;
        dropIntoThisVertex &= (*p)->getX()(d) < marker.x()(d) + marker.h()(d)/2.0;
      }
      
      if (dropIntoThisVertex) {
        fineGridVertexParticleSet.push_back( *p );
        p = coarseGridVerticesParticleSet(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  if ( true and true ) {
    auto p = fineGridVertexParticleSet.begin();
    while (p!=fineGridVertexParticleSet.end()) {
      auto pp = p;
      pp++;
      bool foundDuplicate = false;
      while (
        pp!=fineGridVertexParticleSet.end()
        and 
        not foundDuplicate
      ) {
        foundDuplicate |= tarch::la::equals( (*p)->getX(), (*pp)->getX() );
        pp++;
      }
      
      if (foundDuplicate) {
        p = fineGridVertexParticleSet.erase(p);
        _numberOfEliminations++;
      }
      else {
        p++;
      }
    }
  }
  
  for (auto& p: fineGridVertexParticleSet ) {
    if (
      p->getNewParallelState()==globaldata::Particle::NewParallelState::Halo
      and 
      p->getParallelState()==globaldata::Particle::ParallelState::Halo
    ) {
      p->setParallelState( globaldata::Particle::ParallelState::Halo );
      _numberOfHalo++;
    }
    else if (
      p->getNewParallelState()==globaldata::Particle::NewParallelState::Halo
      and 
      p->getParallelState()==globaldata::Particle::ParallelState::Local
    ) {
      p->setParallelState( globaldata::Particle::ParallelState::Halo );
      _numberOfHalo++;
      _numberOfLeavingParticles++;
    }
    else if (
      p->getNewParallelState()==globaldata::Particle::NewParallelState::Local
      and 
      p->getParallelState()==globaldata::Particle::ParallelState::Halo
    ) {
      p->setParallelState( globaldata::Particle::ParallelState::Local );
      _numberOfLocal++;
    }
    else if (
      p->getNewParallelState()==globaldata::Particle::NewParallelState::Local
      and 
      p->getParallelState()==globaldata::Particle::ParallelState::Local
    ) {
      p->setParallelState( globaldata::Particle::ParallelState::Local );
      _numberOfLocal++;
    }
  }
  const double CellToCutOffRadiusRatio = 0.5 * 0.9;
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::ParticleSet::iterator p = coarseGridVerticesParticleSet(i).begin(); 
    while ( p!=coarseGridVerticesParticleSet(i).end() ) {
      bool dropIntoThisVertex = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius();
      for (int d=0; d<Dimensions; d++) {
        dropIntoThisVertex &= (*p)->getX()(d) > marker.x()(d) - marker.h()(d)/2.0;
        dropIntoThisVertex &= (*p)->getX()(d) < marker.x()(d) + marker.h()(d)/2.0;
      }
      
      if (dropIntoThisVertex) {
        fineGridVertexParticleSet.push_back( *p );
        p = coarseGridVerticesParticleSet(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }
}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::NewtonRaphsonSmoothingLength2peano4_toolbox_particles_UpdateParticleGridAssociation1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

if ( true ) {  
  vertexdata::ParticleSet particlesThatMoveOnSameLevel[TwoPowerD];
  vertexdata::ParticleSet liftParticles[TwoPowerD];

  for (int i=0; i<TwoPowerD; i++) {
    std::bitset<Dimensions> currentAssociation;
    std::bitset<Dimensions> destAssociation;
    bool                    remainsOnSameLevel;
    
    vertexdata::ParticleSet::iterator p = fineGridVerticesParticleSet(i).begin(); 
    while ( p!=fineGridVerticesParticleSet(i).end() ) {
      _numberOfParticleAccesses++;
      
      if ( marker.isContained((*p)->getX()) ) {
        (*p)->setNewParallelState( globaldata::Particle::NewParallelState::Local );
      }
      
      std::bitset<Dimensions>  currentAssociation = i;
      bool                     remainsOnSameLevel = true;
      for (int d=0; d<Dimensions; d++) {
        destAssociation[d]  = tarch::la::greaterEquals( (*p)->getX(d), marker.x()(d) );
        remainsOnSameLevel &= tarch::la::smallerEquals( std::abs((*p)->getX(d) - marker.x()(d)), marker.h()(d));
      }
      
      if (remainsOnSameLevel and currentAssociation==destAssociation) {
        p++;
      }
      else if (remainsOnSameLevel) {
        logDebug( "touchCellLastTime(...)", "reassign particle " << (*p)->toString() << " in tree " << _treeNumber << " within cell " << marker.toString() << " which had been associated with vertex " << i << " but belongs to " << destAssociation );
        particlesThatMoveOnSameLevel[ destAssociation.to_ulong() ].push_back(*p);
        p = fineGridVerticesParticleSet(i).erase(p);
        _numberOfParticleReassignmentsOnCurrentLevel++;
      }
      else {
        logDebug( "touchCellLastTime(...)", "lift particle " << (*p)->toString() << " in tree " << _treeNumber << " from cell " << marker.toString() << " as difference is " << ((*p)->getX() - marker.x()) );
        liftParticles[ destAssociation.to_ulong() ].push_back(*p);
        p = fineGridVerticesParticleSet(i).erase(p);
        _numberOfLifts++;
      }
    }
  }
  
  for (int i=0; i<TwoPowerD; i++) {
    fineGridVerticesParticleSet(i).insert(   fineGridVerticesParticleSet(i).end(),   particlesThatMoveOnSameLevel[ i ].begin(), particlesThatMoveOnSameLevel[ i ].end() ); 
    coarseGridVerticesParticleSet(i).insert( coarseGridVerticesParticleSet(i).end(), liftParticles[ i ].begin(),                liftParticles[ i ].end() ); 
  }
}
}


