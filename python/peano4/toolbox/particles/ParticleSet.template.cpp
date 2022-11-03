#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"
#include "tarch/multicore/Lock.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


tarch::multicore::BooleanSemaphore   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_semaphore;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_particlesThatCanNotBeLiftedWithinTheirTree;
{{NAMESPACE | join("::")}}::{{CLASSNAME}}     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_particlesThatHaveToBeSieved;
bool              {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_thereAreParticlesToBeSieved;

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfLifts                    = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfDrops                    = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfSieves                   = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfReassignments            = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfAssignmentsToGlobalSieve = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_totalRealParticleCount = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_totalVirtualParticleCount = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_totalEliminatedSpatialDuplicates = 0;


#if PeanoDebug>=1
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugH() const {
  return _debugH;
}
#endif


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(::peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        for (auto p: neighbour) {
          if (
            p->getParallelState()==DoFType::ParallelState::Local
          ) {
            logDebug( "merge(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
            p->setParallelState( DoFType::ParallelState::Virtual );
            this->push_back(p);
          }
          else if (
            p->getParallelState()==DoFType::ParallelState::Virtual
          ) {
            logDebug(
              "merge(...)",
              "incoming particle " << p->toString() << " is virtual on neighbour, yet was not deleted there. " <<
              "Assume it had just switched from local to virtual there, as it had moved. " <<
              "We should have got the virtual copy in the step before which we have switched to local here already if it fits into our domain. " <<
              "Hence, ignore this incoming copy."
            );
          }
          else {
            assertion1(false,p->toString());
          }
        }

        eliminateSpatialDuplicates( true, 1.0 + 1e-6 );

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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::eliminateSpatialDuplicates(bool examineOnlyVirtualParticles, double sortingTolerance) {
  assertion(sortingTolerance>1.0);

  int numberOfEliminations = 0;
  auto p = this->begin();
  while (p!=this->end()) {
    bool foundDuplicate = false;

    if (
      examineOnlyVirtualParticles
      and
      (*p)->getParallelState()==DoFType::ParallelState::Virtual
    ) {
      auto pp = this->begin();
      while (
        pp!=this->end()
        and
        not foundDuplicate
      ) {
        if (
          pp!=p
          and
          tarch::la::equals( (*p)->getX(), (*pp)->getX(), sortingTolerance-1.0 )
        ) {
          logDebug( "eliminateSpatialDuplicates()", "found duplicates p=" << (*p)->toString() << " and pp=" << (*pp)->toString() );
          foundDuplicate = true;
          pp = this->end();
        }
        else {
          pp++;
        }
      }
    }
    else if ( not examineOnlyVirtualParticles ) {
      auto pp = p;
      pp++;
      while (
        pp!=this->end()
        and
        not foundDuplicate
      ) {
        if (
          not examineOnlyVirtualParticles
          and
          tarch::la::equals( (*p)->getX(), (*pp)->getX(), sortingTolerance-1.0 )
        ) {
          logDebug( "eliminateSpatialDuplicates()", "found duplicates p=" << (*p)->toString() << " and pp=" << (*pp)->toString() );
          foundDuplicate = true;
          pp = this->end();
        }
        else {
          pp++;
        }
      }
    }

    if (foundDuplicate) {
      delete *p;
      p = erase(p);
      tarch::multicore::Lock lock( _semaphore );
      _totalEliminatedSpatialDuplicates++;
    }
    else {
      p++;
    }
  }
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


#ifdef Parallel
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  DoFType::initDatatype();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {
  DoFType::shutdownDatatype();
}
#endif


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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishedTraversal(bool updateStatistics) {
  if (updateStatistics) {
    int globalNumberOfLifts                     = 0;
    int globalNumberOfDrops                     = 0;
    int globalNumberOfSieves                    = 0;
    int globalNumberOfReassignments             = 0;
    int globalNumberOfAssignmentsToGlobalSieve  = 0;
    int globalRealNumberOfParticles             = 0;
    int globalVirtualNumberOfParticles          = 0;
    int globalEliminatedSpatialDuplicates       = 0;
    #if Parallel
    tarch::mpi::Rank::getInstance().reduce(
      &globalNumberOfLifts,
      &_numberOfLifts,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalNumberOfDrops,
      &_numberOfDrops,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalNumberOfSieves,
      &_numberOfSieves,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalNumberOfReassignments,
      &_numberOfReassignments,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalNumberOfAssignmentsToGlobalSieve,
      &_numberOfAssignmentsToGlobalSieve,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalRealNumberOfParticles,
      &_totalRealParticleCount,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalVirtualNumberOfParticles,
      &_totalVirtualParticleCount,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().reduce(
      &globalEliminatedSpatialDuplicates,
      &_totalEliminatedSpatialDuplicates,
      1, MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getGlobalMasterRank(),
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    #else
    globalNumberOfLifts                       = _numberOfLifts;
    globalNumberOfDrops                       = _numberOfDrops;
    globalNumberOfSieves                      = _numberOfSieves;
    globalNumberOfReassignments               = _numberOfReassignments;
    globalNumberOfAssignmentsToGlobalSieve    = _numberOfAssignmentsToGlobalSieve;
    globalRealNumberOfParticles               = _totalRealParticleCount;
    globalVirtualNumberOfParticles            = _totalVirtualParticleCount;
    globalEliminatedSpatialDuplicates         = _totalEliminatedSpatialDuplicates;
    #endif

    if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
      logInfo(
        "finishedTraversal()",
        "#lifts=" << globalNumberOfLifts <<
        ", #drops=" << globalNumberOfDrops <<
        ", #sieves=" << globalNumberOfSieves <<
        ", #reassignments=" << globalNumberOfReassignments <<
        ", #assignments-to-sieve-set=" << globalNumberOfAssignmentsToGlobalSieve <<
        ", #particles=" << globalRealNumberOfParticles <<
        ", #virtual particles=" << globalVirtualNumberOfParticles <<
        ", #eliminated particle duplicates=" << globalEliminatedSpatialDuplicates
      );
    }

    _numberOfLifts                    = 0;
    _numberOfDrops                    = 0;
    _numberOfSieves                   = 0;
    _numberOfReassignments            = 0;
    _numberOfAssignmentsToGlobalSieve = 0;
    _totalRealParticleCount           = 0;
    _totalVirtualParticleCount        = 0;
    _totalEliminatedSpatialDuplicates = 0;
  }

  _particlesThatHaveToBeSieved.deleteParticles();

  _particlesThatHaveToBeSieved.insert(
    _particlesThatHaveToBeSieved.end(),
    _particlesThatCanNotBeLiftedWithinTheirTree.begin(),
    _particlesThatCanNotBeLiftedWithinTheirTree.end()
  );
  // this is important, but I don't know why
  _particlesThatHaveToBeSieved.eliminateSpatialDuplicates( false, 1.0 + 1e-6 );
  _thereAreParticlesToBeSieved = not _particlesThatHaveToBeSieved.empty();
  _particlesThatCanNotBeLiftedWithinTheirTree.clear();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}} {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSievedParticlesForNextGridTraversal() {
  {{NAMESPACE | join("::")}}::{{CLASSNAME}} result;
  for (auto *p: _particlesThatHaveToBeSieved) {
    globaldata::{{PARTICLE_TYPE}}* copyOfParticle = new globaldata::{{PARTICLE_TYPE}}(*p);
    copyOfParticle->setParallelState( globaldata::{{PARTICLE_TYPE}}::ParallelState::Virtual );
    result.push_back( copyOfParticle );
  }
  return result;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticles() {
  for (auto *p: *this) {
    delete p;
  }
  clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleCanNotBeLiftedLocally( globaldata::{{PARTICLE_TYPE}}* p ) {
  assertion1( p->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local, p->toString() );
  _particlesThatCanNotBeLiftedWithinTheirTree.push_back(p);
  _numberOfAssignmentsToGlobalSieve++;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto *p: *this) {
    particleCanNotBeLiftedLocally(p);
  }
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getParticlesToBeSievedIntoVertex(
  const peano4::datamanagement::VertexMarker& marker,
  double sortingTolerance
) {
  assertion( not empty() );

  auto sieveParticle = [&](globaldata::{{PARTICLE_TYPE}}* p) {
    return
        marker.isContainedInAdjacentCells( p->getX(), 0.5 * sortingTolerance)
        and
        tarch::la::min( marker.h() ) >= 2.0 * p->getCutOffRadius()
        and
        (
          tarch::la::min( marker.h() ) / 3.0 < 2.0 * p->getCutOffRadius()
          or
          not marker.hasBeenRefined()
        );
  };

  {{NAMESPACE | join("::")}}::{{CLASSNAME}} result;
  {{CLASSNAME}}::iterator p = begin();
  while ( p!=end() ) {
    if ( sieveParticle(*p) ) {
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateLiftDropStatistics(
  int numberOfLiftsInThisTree,
  int numberOfDropsInThisTree,
  int numberOfReassignmentsInThisTree,
  int numberOfSievesInThisTree
) {
  tarch::multicore::Lock lock( _semaphore );
  _numberOfLifts         += numberOfLiftsInThisTree;
  _numberOfDrops         += numberOfDropsInThisTree;
  _numberOfReassignments += numberOfReassignmentsInThisTree;
  _numberOfSieves        += numberOfSievesInThisTree;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateParticleCountStatistics(
  int totalLocalParticleCountInThisTree,
  int totalVirtualParticleCountInThisTree
) {
  tarch::multicore::Lock lock( _semaphore );
  _totalRealParticleCount += totalLocalParticleCountInThisTree;
  _totalVirtualParticleCount += totalVirtualParticleCountInThisTree;
}
