#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "Constants.h"

#include "tarch/multicore/Lock.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(
  DoFType*                                     neighbourParticle,
  const peano4::datamanagement::VertexMarker&  marker,
  int                                          spacetreeId
) {
  assertion3(
    marker.isContainedInAdjacentCells(neighbourParticle->getX(), 0.5, toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    neighbourParticle->toString(),
    marker.toString(),
    spacetreeId
  );
  const bool incomingParticleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( neighbourParticle->getX(), marker );
  Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(neighbourParticle, _memoryPool.container.begin(), _memoryPool.container.end());

  if (neighbourParticle->getParallelState()==DoFType::ParallelState::Virtual and not incomingParticleWillBeLocal) {
    logDebug(
      "mergeWithParticle(...)",
      "incoming particle " << neighbourParticle->toString() << " tied to " << marker.toString() << " is virtual on neighbour. Such a copy is not relevant as it is a copy of the halo of another rank."
    );
    delete neighbourParticle;
    tarch::multicore::Lock lock( _statisticsSemaphore );
    _numberOfDroppedIncomingVirtualParticles++;
  }
  else if (
    iteratorToExistingCopy!=end()
    and
    neighbourParticle->getParallelState()==DoFType::ParallelState::Virtual
    and
    incomingParticleWillBeLocal
  ) {
    logDebug(
      "mergeWithParticle(...)",
      "incoming particle " << neighbourParticle->toString() << " tied to " << marker.toString() << " is virtual on neighbour and local here, i.e. neighbour sent me back my own copy. Drop on tree " << spacetreeId << " (marker=" << marker.toString() << ")"
    );
    delete neighbourParticle;
    tarch::multicore::Lock lock( _statisticsSemaphore );
    _numberOfDroppedIncomingVirtualParticles++;
  }
  else if ( iteratorToExistingCopy!=end() ) {
    neighbourParticle->setParallelState( incomingParticleWillBeLocal ? DoFType::ParallelState::Local : DoFType::ParallelState::Virtual );
   
    #if PeanoDebug>0
    neighbourParticle->setDebugX( (*iteratorToExistingCopy)->getDebugX() );
    #endif
    
    if ( (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local and incomingParticleWillBeLocal ) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfRedundantlySharedLocalParticles++;
      logDebug( "mergeWithParticle(...)", "inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing copy and will be local on tree  " << spacetreeId << " (marker=" << marker.toString() << ")" );
    }
    else if (not incomingParticleWillBeLocal) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfReplacedVirtualParticlesAlongBoundary++;
      logDebug( "mergeWithParticle(...)", "inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing halo copy on tree  " << spacetreeId << " (marker=" << marker.toString() << ")");
    }
    else {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedLocalParticlesAlongBoundary++;
      logDebug( "mergeWithParticle(...)", "inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing halo copy but is local, i.e. particle entered subdomain");
    }

    toolbox::particles::assignmentchecks::detachParticleFromVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(), (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local,
      marker.x(), marker.h(), spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::eraseParticle(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(), (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local,
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(), neighbourParticle->getParallelState()==DoFType::ParallelState::Local,
      marker.x(), marker.h(), spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
    
    // See docu of replace(): it will delete p
    _memoryPool.replace( iteratorToExistingCopy, neighbourParticle );
  }
  else {
    neighbourParticle->setParallelState( incomingParticleWillBeLocal ? DoFType::ParallelState::Local : DoFType::ParallelState::Virtual );
    
    _memoryPool.scatter();
    _memoryPool.container.push_back(neighbourParticle);

    if (incomingParticleWillBeLocal) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedLocalParticlesAlongBoundary++;
      logDebug( "mergeWithParticle(...)", "inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " is new local vertex");
    }
    else {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedVirtualParticlesAlongBoundary++;
      logDebug( "mergeWithParticle(...)", "inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " is new and will be remote on tree  " << spacetreeId << " (marker=" << marker.toString() << ")" );
    }
    
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(), neighbourParticle->getParallelState()==DoFType::ParallelState::Local,
      marker.x(), marker.h(), spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::grid::TraversalObserver::SendReceiveContext  context,
  {{CLASSNAME}}&                                         neighbour,
  const peano4::datamanagement::VertexMarker&            marker,
  int                                                    spacetreeId
) {
  assertion( not neighbour._memoryPool.isGathered() );
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        assertionVectorNumericalEquals2( neighbour._debugH, _debugH, neighbour.toString(), toString() );

        logDebug( "merge(...)", "merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString() );
        for (auto p: neighbour._memoryPool.container) {
          p->setX( toolbox::particles::mirrorParticleAlongPeriodicDomains(
            p->getX(),
            marker,
            DomainOffset,
            DomainSize,
            PeriodicBC
          ));
          mergeWithParticle(p, marker, spacetreeId);
        }

        logDebug( "merge(...)", "merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.toString() << " which now hosts " << this->size() << " particle(s)" );
        for (auto *p: *this) {
          logDebug( "merge(...)", " - " << p->toString() );
        }

        assertion2( isValid(), toString(), marker.toString() );
      }
      break;
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      {
        assertionVectorNumericalEquals2( neighbour._debugX, _debugX, neighbour.toString(), toString() );
        assertionVectorNumericalEquals2( neighbour._debugH, _debugH, neighbour.toString(), toString() );

        logDebug( "merge(...)", "merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString() );
        for (auto p: neighbour._memoryPool.container) {
          mergeWithParticle(p, marker, spacetreeId);
        }

        logDebug( "merge(...)", "merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.toString() << " which now hosts " << this->size() << " particle(s)" );
        for (auto *p: *this) {
          logDebug( "merge(...)", " - " << p->toString() );
        }

        assertion2( isValid(), toString(), marker.toString() );
      }
      break;

    case ::peano4::grid::TraversalObserver::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;

    case ::peano4::grid::TraversalObserver::SendReceiveContext::ForkDomain:
      *this = neighbour;
      break;

    case ::peano4::grid::TraversalObserver::SendReceiveContext::JoinDomain:
      assertionMsg( false, "not implemented yet, but should be plain copy" );
      break;

  }
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(const peano4::datamanagement::VertexMarker& marker) {
  return ::peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream msg;
  msg << "(size=" << size()
      << ",gathered=" << _memoryPool.isGathered()
      << ",";
  msg << Base::toString(false);
  msg << ")";
  return msg.str();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone( const {{CLASSNAME}}& otherSet ) {
  assertion1( _memoryPool.container.empty(), toString() );
  assertion1( not _memoryPool.isGathered(), toString() );
  for (auto particle: otherSet._memoryPool.container) {
    // data consistency check
    assertion2(
      particle->getParallelState()==DoFType::ParallelState::Local
      or
      particle->getParallelState()==DoFType::ParallelState::Virtual,
      particle->toString(),
      toString()
    );
    DoFType* newParticle =  new DoFType(*particle);
    _memoryPool.container.push_back( newParticle );
  }
  logDebug( "clone()", "copied " << otherSet.size() << " objects over into local one of size " << size() );
  #if PeanoDebug>=1
  _debugX = otherSet._debugX;
  _debugH = otherSet._debugH;
  #endif
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticles() {
  _memoryPool.scatter();
  for (auto *p: _memoryPool.container) {
    delete p;
  }
  _memoryPool.container.clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clear() {
  _memoryPool.clearAndReset();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleCanNotBeLiftedLocally(
  const Container::iterator&   originalParticleContainer
) {
  logDebug( "particleCanNotBeLiftedLocally()", "call scatter and update iterator" );
  Container::iterator particle = _memoryPool.scatterAndUpdateIterator( originalParticleContainer );

  assertion1( (*particle)->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local, (*particle)->toString() );

  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle );
  return _memoryPool.container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(DoFType*  particle) {
  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  _memoryPool.scatter();
  for (auto p: _memoryPool.container) {
    _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p);
  }
  _memoryPool.container.clear();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::const_iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() const {
  return _memoryPool.container.begin();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::const_iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() const {
  return _memoryPool.container.end();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() {
  return _memoryPool.container.begin();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() {
  return _memoryPool.container.end();
}


int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::size() const {
  return _memoryPool.container.size();
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::empty() const {
  return _memoryPool.container.empty();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::addParticle(DoFType*  particle) {
  _memoryPool.scatter();

  // data consistency check
  assertion2(
    particle->getParallelState()==DoFType::ParallelState::Local
    or
    particle->getParallelState()==DoFType::ParallelState::Virtual,
    particle->toString(),
    toString()
  );

  _memoryPool.container.push_back(particle);
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isValid() const {
  for (auto particle: _memoryPool.container) {
    if (
      particle->getParallelState()!=DoFType::ParallelState::Local
      and
      particle->getParallelState()!=DoFType::ParallelState::Virtual
    ) return false;
  }

  return true;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticles({{CLASSNAME}}&  particles) {
  _memoryPool.scatter();

  _memoryPool.container.insert(
    _memoryPool.container.end(),
    particles.begin(),
    particles.end()
  );
  particles.clear();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticle(
  Container::iterator  particle,
  {{CLASSNAME}}&       sourceParticleSet
) {
  // data consistency check
  assertion3(
    (*particle)->getParallelState()==DoFType::ParallelState::Local
    or
    (*particle)->getParallelState()==DoFType::ParallelState::Virtual,
    (*particle)->toString(),
    toString(),
    sourceParticleSet.toString()
  );

  _memoryPool.scatter();

  if ( sourceParticleSet.isGathered() ) {
    logDebug( "grabParticle()", "call scatter and update iterator" );
    particle = sourceParticleSet._memoryPool.scatterAndUpdateIterator( particle );
  }

  _memoryPool.container.push_back( *particle );
  logDebug( "grabParticle()", "add further particle to scattered data set: " << toString() );
  return sourceParticleSet._memoryPool.container.erase(particle);
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticle( 
  Container::iterator  particle
) {
  // data consistency check
  assertion2(
    (*particle)->getParallelState()==DoFType::ParallelState::Local
    or
    (*particle)->getParallelState()==DoFType::ParallelState::Virtual,
    (*particle)->toString(),
    toString()
  );

  if ( _memoryPool.isGathered() ) {
    logDebug( "deleteParticle()", "call scatter and update iterator" );
    particle = _memoryPool.scatterAndUpdateIterator( particle );
  }

  delete *particle;
  return _memoryPool.container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::gather() {
  _memoryPool.gather();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::scatter() {
  _memoryPool.scatter();
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isGathered() const {
  return _memoryPool.isGathered();
}
