#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "tarch/multicore/Lock.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::grid::TraversalObserver::SendReceiveContext  context,
  {{CLASSNAME}}&                                         neighbour,
  const peano4::datamanagement::VertexMarker&            marker,
  int                                                    spacetreeId
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
          const bool incomingParticleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( p->getX(), marker );
          if (not incomingParticleWillBeLocal and p->getParallelState()==DoFType::ParallelState::Virtual){
            logDebug(
              "merge(...)",
              "incoming particle " << p->toString() << " is virtual on neighbour. Such a copy is not relevant as it is a copy of the halo of another rank."
            );
            delete p;
            tarch::multicore::Lock lock( _statisticsSemaphore );
            _numberOfDroppedIncomingVirtualParticles++;
          }
          else if (incomingParticleWillBeLocal) {
            p->setParallelState( DoFType::ParallelState::Local );
            Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(p, _container.begin(), _container.end());
            if ( iteratorToExistingCopy!=end() and (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Virtual ) {
              logDebug( "merge(...)", "incoming particle " << p->toString() << " will be local and replaces previous virtual copy of " << (*iteratorToExistingCopy)->toString() << ", i.e. particle enters domain" );
              delete *iteratorToExistingCopy;
              _container.erase(iteratorToExistingCopy);
              _container.push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfIncomingParticlesWhichWerePreviouslyKnownAsVirtual++;
            }
            else if ( iteratorToExistingCopy!=end() ) {
              logDebug( "merge(...)", "incoming particle " << p->toString() << " would be local, but we already have a local particle copy here. Drop" );
              delete p;
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfRedundantlySharedLocalParticles++;
            }
            else {
              _container.push_back(p);
              logDebug( "merge(...)", "incoming particle " << p->toString() << " is a new local particle (will be counted by UpdateParticle action set)" );
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfNewIncomingLocalParticles++;
              toolbox::particles::assignmentchecks::assignParticleToVertex(
                toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
                p->getX(), true, marker.x(), marker.h(), spacetreeId );
            }
          }
          else {
            logDebug( "merge(...)", "incoming particle " << p->toString() << " will be virtual, i.e. will be halo copy" );
            p->setParallelState( DoFType::ParallelState::Virtual );
            Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(p, _container.begin(), _container.end());
            if ( iteratorToExistingCopy!=end() ) {
              delete *iteratorToExistingCopy;
              _container.erase(iteratorToExistingCopy);
              _container.push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfReplacedVirtualParticles++;
            }
            else {
              _container.push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfAddedVirtualParticles++;
              toolbox::particles::assignmentchecks::assignParticleToVertex(
                toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
                p->getX(), false, marker.x(), marker.h(), spacetreeId );
            }
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
  const Container::iterator&   particle,
  int                          targetTree
) {
  assertion1( (*particle)->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local, (*particle)->toString() );

  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle, targetTree );
  return _container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto p: _container) {
    _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p, toolbox::particles::SieveParticles< {{CLASSNAME}} >::GlobalSort );
  }
  _container.clear();
}


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


