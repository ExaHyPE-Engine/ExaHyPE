#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "tarch/multicore/Lock.h"

#include "toolbox/particles/MultiscaleTransitions.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::grid::TraversalObserver::SendReceiveContext  context,
  {{CLASSNAME}}&                                         neighbour,
  const peano4::datamanagement::VertexMarker&            marker
) {
  assertion( not neighbour._memoryPool.isGathered() );
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        for (auto p: neighbour._memoryPool.container) {
          assertionMsg( false, "Cristian fix. Here, the coordinates have to be flipped/mirrored along the boundaries. If a boundary is periodic is encoded in Constants.h (I think), so we might want to include it from there" );
        }
      }
      // There is no break here, as we continue with the normal boundary exchange handling!
      // this is not a bug but intentional.
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      {
        logDebug( "merge(...)", "merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString() );
        for (auto p: neighbour._memoryPool.container) {
          if (
            p->getParallelState()==DoFType::ParallelState::Local
          ) {
            p->setParallelState( DoFType::ParallelState::Virtual );

            Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(p, _memoryPool.container.begin(), _memoryPool.container.end());
            if ( iteratorToExistingCopy!=end() ) {
              logDebug( "merge(...)", "incoming particle " << p->toString() << " is local on other rank and will be virtual here. It replaces existing copy" );
              _memoryPool.replace( iteratorToExistingCopy, p );
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfReplacedVirtualParticles++;
            }
            else {
              logDebug( "merge(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
              _memoryPool.scatter();
              _memoryPool.container.push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfAddedVirtualParticles++;
            }
          }
          else if (
            p->getParallelState()==DoFType::ParallelState::Virtual
          ) {
            logDebug(
              "merge(...)",
              "incoming particle " << p->toString() << " is virtual on neighbour. Such a copy is not relevant here."
            );
            delete p;
            tarch::multicore::Lock lock( _statisticsSemaphore );
            _numberOfDroppedVirtualParticles++;
          }
          else {
            assertion3(false,p->toString(),size(),marker.x());
          }
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::eliminateSpatialDuplicates() {
  int numberOfLocalEliminatedSpatialDuplicates   = 0;
  int numberOfVirtualEliminatedSpatialDuplicates = 0;

  auto p = _memoryPool.container.begin();
  while (p != _memoryPool.container.end()) {
    bool foundDuplicate = false;

    // data consistency check
    assertion2(
      (*p)->getParallelState()==DoFType::ParallelState::Local
      or
      (*p)->getParallelState()==DoFType::ParallelState::Virtual,
      (*p)->toString(),
      toString()
    );

    auto pp = p;
    pp++;
    while (pp != _memoryPool.container.end() and not foundDuplicate) {
      if (tarch::la::equals((*p)->getX(), (*pp)->getX(), ::toolbox::particles::SpatialDuplicateTolerance)) {
        logDebug(
          "eliminateSpatialDuplicates()", "found duplicates p=" << (*p)->toString() << " and pp=" << (*pp)->toString()
        );
        foundDuplicate = true;
      } else {
        pp++;
      }
    }

    if (foundDuplicate and _memoryPool.isGathered()) {
      _memoryPool.scatter();
      p = _memoryPool.container.begin();
    }
    else if (foundDuplicate) {
      assertion3((*p) != (*pp), (*p)->toString(), (*pp)->toString(), toString() );

      // bias to eliminate virtual particles
      if (
        (*p)->getParallelState()==DoFType::ParallelState::Local
        and
        (*pp)->getParallelState()==DoFType::ParallelState::Local
      ) {
        logDebug("eliminateSpatialDuplicates()", "particle " << (*pp)->toString() << " found twice as local in domain");

        delete *p;
        _memoryPool.container.erase(p);

        numberOfLocalEliminatedSpatialDuplicates++;
      } else if ((*p)->getParallelState() == DoFType::ParallelState::Virtual) {
        delete *p;
        _memoryPool.container.erase(p);

        numberOfVirtualEliminatedSpatialDuplicates++;
      } else {
        delete *pp;
        _memoryPool.container.erase(pp);

        numberOfVirtualEliminatedSpatialDuplicates++;
      }

      p = _memoryPool.container.begin(); // if we delete something, we might corrupt the whole container, so
                                         // better start from scratch.
    }
    else {
      p++;
    }
  }

  tarch::multicore::Lock lock( _statisticsSemaphore );
  _numberOfLocalEliminatedSpatialDuplicates   += numberOfLocalEliminatedSpatialDuplicates;
  _numberOfVirtualEliminatedSpatialDuplicates += numberOfVirtualEliminatedSpatialDuplicates;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
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
  logInfo( "particleCanNotBeLiftedLocally()", "call scatter and update iterator" );
  Container::iterator particle = _memoryPool.scatterAndUpdateIterator( originalParticleContainer );

  assertion1( (*particle)->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local, (*particle)->toString() );

  sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle);
  return _memoryPool.container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  _memoryPool.scatter();
  for (auto p: _memoryPool.container) {
    sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p);
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::addParticles(const ParticleList&  particles) {
  _memoryPool.scatter();

  _memoryPool.container.insert(
    _memoryPool.container.end(),
    particles.begin(),
    particles.end()
  );
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
  const Container::iterator&     originalParticleContainerIterator,
  {{CLASSNAME}}&                 sourceParticleSet
) {
  // data consistency check
  assertion3(
    (*originalParticleContainerIterator)->getParallelState()==DoFType::ParallelState::Local
    or
    (*originalParticleContainerIterator)->getParallelState()==DoFType::ParallelState::Virtual,
    (*originalParticleContainerIterator)->toString(),
    toString(),
    sourceParticleSet.toString()
  );

  _memoryPool.scatter();

  if ( sourceParticleSet.isGathered() ) {
    logInfo( "grabParticle()", "call scatter and update iterator" );
    Container::iterator particle = sourceParticleSet._memoryPool.scatterAndUpdateIterator( originalParticleContainerIterator );

    _memoryPool.container.push_back( *particle );

    logInfo( "grabParticle()", "now host " << _memoryPool.container.size() << " particles in " << toString() );
    // This time, we cannot say anything about the ordering
    return sourceParticleSet._memoryPool.container.begin();
  }
  else {
    _memoryPool.container.push_back( *originalParticleContainerIterator );
    logInfo( "grabParticle()", "add further particle to scattered data set: " << toString() );
    return sourceParticleSet._memoryPool.container.erase(originalParticleContainerIterator);
  }
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticle( const Container::iterator&   originalParticleContainerIterator ) {
  // data consistency check
  assertion2(
    (*originalParticleContainerIterator)->getParallelState()==DoFType::ParallelState::Local
    or
    (*originalParticleContainerIterator)->getParallelState()==DoFType::ParallelState::Virtual,
    (*originalParticleContainerIterator)->toString(),
    toString()
  );

  if ( _memoryPool.isGathered() ) {
    logInfo( "deleteParticle()", "call scatter and update iterator" );
    Container::iterator particle = _memoryPool.scatterAndUpdateIterator( originalParticleContainerIterator );
    delete *particle;
    _memoryPool.container.erase(particle);
    // Just to be on teh safe side. We don't make assumptions on ordering
    return _memoryPool.container.begin();
  }
  else {
    delete *originalParticleContainerIterator;
    return _memoryPool.container.erase(originalParticleContainerIterator);
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::gather() {
  _memoryPool.gather();
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isGathered() const {
  return _memoryPool.isGathered();
}
