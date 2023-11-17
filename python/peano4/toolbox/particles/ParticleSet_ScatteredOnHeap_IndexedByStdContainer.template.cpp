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
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        for (auto p: neighbour) {
          assertionMsg( false, "Cristian fix. Here, the coordinates have to be flipped/mirrored along the boundaries. If a boundary is periodic is encoded in Constants.h (I think), so we might want to include it from there" );
        }
      }
      // There is no break here, as we continue with the normal boundary exchange handling!
      // this is not a bug but intentional.
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      {
        logDebug( "merge(...)", "merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString() );
        for (auto p: neighbour) {
          if (
            p->getParallelState()==DoFType::ParallelState::Local
          ) {
            Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(p, _container.begin(), _container.end());
            if ( iteratorToExistingCopy!=end() ) {
              logDebug( "merge(...)", "incoming particle " << p->toString() << " is local on other rank and will be virtual here. It replaces existing copy" );
              delete *iteratorToExistingCopy;
              _container.erase(iteratorToExistingCopy);
              p->setParallelState( DoFType::ParallelState::Virtual );
              _container.push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfReplacedVirtualParticles++;
            }
            else {
              logDebug( "merge(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
              p->setParallelState( DoFType::ParallelState::Virtual );
              _container.push_back(p);
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

  auto p = _container.begin();
  while (p != _container.end()) {
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
    while (pp != _container.end() and not foundDuplicate) {
      if (tarch::la::equals((*p)->getX(), (*pp)->getX(), ::toolbox::particles::SpatialDuplicateTolerance)) {
        logDebug(
          "eliminateSpatialDuplicates()", "found duplicates p=" << (*p)->toString() << " and pp=" << (*pp)->toString()
        );
        foundDuplicate = true;
      } else {
        pp++;
      }
    }

    if (foundDuplicate) {
      assertion3((*p) != (*pp), (*p)->toString(), (*pp)->toString(), toString() );

      // bias to eliminate virtual particles
      if (
        (*p)->getParallelState()==DoFType::ParallelState::Local
        and
        (*pp)->getParallelState()==DoFType::ParallelState::Local
      ) {
        logDebug("eliminateSpatialDuplicates()", "particle " << (*pp)->toString() << " found twice as local in domain");

        delete *p;
        _container.erase(p);

        numberOfLocalEliminatedSpatialDuplicates++;
      } else if ((*p)->getParallelState() == DoFType::ParallelState::Virtual) {
        delete *p;
        _container.erase(p);

        numberOfVirtualEliminatedSpatialDuplicates++;
      } else {
        delete *pp;
        _container.erase(pp);

        numberOfVirtualEliminatedSpatialDuplicates++;
      }

      p = _container.begin(); // if we delete something, we might corrupt the whole container, so
                              // better start from scratch.
    } else {
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


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(const peano4::datamanagement::VertexMarker& marker) {
  return ::peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream;
}



std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream msg;
  msg << "(size=" << size()
      << ",";
  msg << Base::toString(false);
  msg << ")";
  return msg.str();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone( const {{CLASSNAME}}& otherSet ) {
  assertion1( _container.empty(), toString() );
  for (auto particle: otherSet._container) {
    // data consistency check
    assertion2(
      particle->getParallelState()==DoFType::ParallelState::Local
      or
      particle->getParallelState()==DoFType::ParallelState::Virtual,
      particle->toString(),
      toString()
    );
    DoFType* newParticle =  new DoFType(*particle);
    _container.push_back( newParticle );
  }
  logDebug( "clone()", "copied " << otherSet.size() << " objects over into local one of size " << size() );
  #if PeanoDebug>=1
  _debugX = otherSet._debugX;
  _debugH = otherSet._debugH;
  #endif
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticles() {
  for (auto *p: _container) {
    delete p;
  }
  _container.clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clear() {
  _container.clear();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleCanNotBeLiftedLocally(
  const Container::iterator&   particle
) {
  assertion1( (*particle)->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local, (*particle)->toString() );

  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle, GlobalSort);
  return _container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto p: _container) {
    _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p, GlobalSort);
  }
  _container.clear();
}


/*
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::ParticleList  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getParticlesToBeDroppedIntoVertex(
  const peano4::datamanagement::VertexMarker& marker
) {
  assertion( not empty() );

  {{NAMESPACE | join("::")}}::{{CLASSNAME}} result;
  {{CLASSNAME}}::iterator p = begin();
  while ( p!=end() ) {
    if ( ::toolbox::particles::sieveParticle( **p, marker ) ) {
      logDebug( "getParticlesToBeSievedIntoVertex(...)", "drop particle " << (*p)->toString() << " from global sieve list into vertex " << marker.toString() );
      (*p)->setParallelState( globaldata::{{PARTICLE_TYPE}}::ParallelState::Virtual );
      result.push_back( *p );
      p = this->erase(p);
    }
    else {
      p++;
    }
  }
  return result;
}
*/


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::const_iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() const {
  return _container.begin();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::const_iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() const {
  return _container.end();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() {
  return _container.begin();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() {
  return _container.end();
}


int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::size() const {
  return _container.size();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::addParticle(DoFType*  particle) {
  // data consistency check
  assertion2(
    particle->getParallelState()==DoFType::ParallelState::Local
    or
    particle->getParallelState()==DoFType::ParallelState::Virtual,
    particle->toString(),
    toString()
  );

  _container.push_back(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::addParticles(const ParticleList&  particles) {
  _container.insert(
    _container.end(),
    particles.begin(),
    particles.end()
  );
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isValid() const {
  for (auto particle: _container) {
    if (
      particle->getParallelState()!=DoFType::ParallelState::Local
      and
      particle->getParallelState()!=DoFType::ParallelState::Virtual
    ) return false;
  }

  return true;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticles({{CLASSNAME}}&  particles) {
  _container.insert(
    _container.end(),
    particles.begin(),
    particles.end()
  );
  particles.clear();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticle(
  const Container::iterator&     particle,
  {{CLASSNAME}}&                 sourceParticleSet
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
  _container.push_back( *particle );
  return sourceParticleSet._container.erase(particle);
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Container::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticle( const Container::iterator&   particle ) {
  // data consistency check
  assertion2(
    (*particle)->getParallelState()==DoFType::ParallelState::Local
    or
    (*particle)->getParallelState()==DoFType::ParallelState::Virtual,
    (*particle)->toString(),
    toString()
  );
  delete *particle;
  return _container.erase(particle);
}


