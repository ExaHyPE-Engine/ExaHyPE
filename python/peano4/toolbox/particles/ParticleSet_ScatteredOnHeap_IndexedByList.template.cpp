#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "tarch/multicore/Lock.h"

#include "toolbox/particles/MultiscaleTransitions.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleIsDuplicate(const DoFType* particle) {
  iterator result = {{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin();
  while (result!={{NAMESPACE | join("::")}}::{{CLASSNAME}}::end()) {
    if (
      tarch::la::equals( (*result)->getX(), particle->getX(), toolbox::particles::SpatialDuplicateTolerance )
    ) {
      return result;
    }
    result++;
  }
  return result;
}


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
            p->setParallelState( DoFType::ParallelState::Virtual );

            iterator iteratorToExistingCopy = particleIsDuplicate(p);
            if ( iteratorToExistingCopy!=end() ) {
              logDebug( "merge(...)", "incoming particle " << p->toString() << " is local on other rank and will be virtual here. It replaces existing copy" );
              delete *iteratorToExistingCopy;
              erase(iteratorToExistingCopy);
              push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfVirtualEliminatedSpatialDuplicates++;
            }
            else {
              logDebug( "merge(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
              this->push_back(p);
              tarch::multicore::Lock lock( _statisticsSemaphore );
              _numberOfMergedVirtualParticles++;
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
          }
          else {
            assertion1(false,p->toString());
          }
        }

        // not done before we insert
        //eliminateSpatialDuplicates();

        logDebug( "merge(...)", "merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.x() << " which now hosts " << this->size() << " particle(s):" );
        for (auto *p: *this) {
          logDebug( "merge(...)", " - " << p->toString() );
        }
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
  auto [numberOfLocalEliminatedSpatialDuplicates,numberOfVirtualEliminatedSpatialDuplicates] =
    toolbox::particles::eliminateSpatialDuplicates(*this);

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
  msg << "(size=" << size();
  #if PeanoDebug>=1
  msg << ",x=" << _debugX
      << ",h=" << _debugH;
  #endif
  msg << ")";
  return msg.str();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCutOffRadius() const {
  double result = std::numeric_limits<double>::max();
  for (auto *p: *this) {
    result = std::min( result, p->getCutOffRadius() );
  }
  return result;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone( const {{CLASSNAME}}& otherSet ) {
  assertion1( empty(), toString() );
  for (auto p: otherSet) {
    DoFType* newParticle =  new DoFType(*p);
    this->push_back( newParticle );
  }
  #if PeanoDebug>=1
  _debugX = otherSet._debugX;
  _debugH = otherSet._debugH;
  #endif
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticles() {
  for (auto *p: *this) {
    delete p;
  }
  BaseContainer::clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clear() {
  BaseContainer::clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleCanNotBeLiftedLocally( globaldata::{{PARTICLE_TYPE}}* p ) {
  if ( p->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local ) {
    sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p);
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto *p: *this) {
    particleCanNotBeLiftedLocally(p);
  }
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getParticlesToBeDroppedIntoVertex(
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
