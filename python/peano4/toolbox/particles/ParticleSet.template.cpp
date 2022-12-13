#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "tarch/multicore/Lock.h"
#include "tarch/mpi/IntegerMessage.h"

#include "toolbox/particles/MultiscaleTransitions.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_SieveReductionTag = tarch::mpi::Rank::reserveFreeTag("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");


tarch::multicore::BooleanSemaphore   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_semaphore;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_particlesThatCanNotBeLiftedWithinTheirTree;
{{NAMESPACE | join("::")}}::{{CLASSNAME}}     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_particlesThatHaveToBeSieved;

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfLifts                        = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfDrops                        = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfSieves                       = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfReassignments                = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfAssignmentsToGlobalSieve     = 0;

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfRemainingLocalParticles      = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfIncomingLocalParticles       = 0;
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfLeavingLocalParticles        = 0;

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfMergedVirtualParticles       = 0;

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_numberOfEliminatedSpatialDuplicates  = 0;


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
            tarch::multicore::Lock lock( _semaphore );
            _numberOfMergedVirtualParticles++;
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
      _numberOfEliminatedSpatialDuplicates++;
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishedTraversal() {
  _particlesThatHaveToBeSieved.deleteParticles();

  #ifdef Parallel
  const int numberOfParticlesToSend = _particlesThatCanNotBeLiftedWithinTheirTree.size();
  logInfo( "finishedTraversal()", "inform all other ranks that I host " << numberOfParticlesToSend << " particles that cannot be sorted locally. Use tag " << _SieveReductionTag );
  for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
    if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
      tarch::mpi::IntegerMessage message( numberOfParticlesToSend );
      tarch::mpi::IntegerMessage::sendAndPollDanglingMessages(
        message,
        rank,
        _SieveReductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
    }
  }

  logInfo( "finishedTraversal()", "pack send information (vector of pointers) into array" );
  std::vector<DoFType>  sendBuffer;    // no pointers here
  sendBuffer.resize( numberOfParticlesToSend );
  for (int i=0; i<numberOfParticlesToSend; i++) {
    sendBuffer[i] = *(_particlesThatCanNotBeLiftedWithinTheirTree[i]);
  }

  for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
    if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
      logInfo( "finishedTraversal()", "ask rank " << rank << " how many messages have to be received" );
      tarch::mpi::IntegerMessage numberOfReceivedMessages;
      tarch::mpi::IntegerMessage::receiveAndPollDanglingMessages(
        numberOfReceivedMessages,
        rank,
        _SieveReductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      logInfo( "finishedTraversal()", "will receive " << numberOfReceivedMessages.getValue() << " particles from rank " << rank );

      MPI_Request* sendRequest    = nullptr;
      MPI_Request* receiveRequest = nullptr;

      std::vector<DoFType>  receiveBuffer; // no pointers here
      if ( numberOfReceivedMessages.getValue()>0 ) {
        receiveRequest = new MPI_Request();
        receiveBuffer.resize( numberOfReceivedMessages.getValue() );
        MPI_Irecv(
          receiveBuffer.data(),
          numberOfReceivedMessages.getValue(),
          DoFType::getGlobalCommunciationDatatype(),
          rank,
          _SieveReductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          receiveRequest
        );
      }

      if ( numberOfParticlesToSend>0 ) {
        sendRequest = new MPI_Request();
        MPI_Isend(
          sendBuffer.data(),
          numberOfParticlesToSend,  // this is important
          DoFType::getGlobalCommunciationDatatype(),
          rank,
          _SieveReductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          sendRequest
        );
      }

      logInfo( "finishedTraversal()", "issued my sends and receives" );
      int         flag = 0;
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
      while (not flag) {
        int sendFlag    = 1;
        int receiveFlag = 1;
        if (sendRequest!=nullptr)    MPI_Test( sendRequest,    &sendFlag,    MPI_STATUS_IGNORE );
        if (receiveRequest!=nullptr) MPI_Test( receiveRequest, &receiveFlag, MPI_STATUS_IGNORE );
        flag = sendFlag and receiveFlag;

        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "toolbox::particles::ParticleSet", "finishedTraversal()",rank, _SieveReductionTag );
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "toolbox::particles::ParticleSet", "finishedTraversal()", rank, _SieveReductionTag );
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      }

      logInfo( "finishedTraversal()", "append " << receiveBuffer.size() << " messages to local sieve particle list" );
      for (int i=0; i<numberOfReceivedMessages.getValue(); i++) {
        _particlesThatHaveToBeSieved.push_back( new DoFType(receiveBuffer[i]) );
      }

      if (sendRequest!=nullptr)    delete sendRequest;
      if (receiveRequest!=nullptr) delete receiveRequest;
    }
  }
  #endif

  _particlesThatHaveToBeSieved.insert(
    _particlesThatHaveToBeSieved.end(),
    _particlesThatCanNotBeLiftedWithinTheirTree.begin(),
    _particlesThatCanNotBeLiftedWithinTheirTree.end()
  );
  logInfo( "finishedTraversal()", "rolled over local list and now have " << _particlesThatHaveToBeSieved.size() << " sieve particles here" );

  _particlesThatHaveToBeSieved.eliminateSpatialDuplicates( false, 1.0 + 1e-6 );
  logInfo( "finishedTraversal()", "after eliminating all redundant particles, " << _particlesThatHaveToBeSieved.size() << " particles are left" );
  _particlesThatCanNotBeLiftedWithinTheirTree.clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::reduceAndPrintStatistics() {
  int globalNumberOfLifts                       = 0;
  int globalNumberOfDrops                       = 0;
  int globalNumberOfSieves                      = 0;
  int globalNumberOfReassignments               = 0;
  int globalNumberOfAssignmentsToGlobalSieve    = 0;

  int globalNumberOfRemainingLocalParticles     = 0;
  int globalNumberOfIncomingLocalParticles      = 0;
  int globalNumberOfLeavingLocalParticles       = 0;

  int globalNumberOfMergedVirtualParticles      = 0;

  int globalNumberOfEliminatedSpatialDuplicates = 0;
  #if Parallel
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfLifts,
    &globalNumberOfLifts,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfDrops,
    &globalNumberOfDrops,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfSieves,
    &globalNumberOfSieves,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfReassignments,
    &globalNumberOfReassignments,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfAssignmentsToGlobalSieve,
    &globalNumberOfAssignmentsToGlobalSieve,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfRemainingLocalParticles,
    &globalNumberOfRemainingLocalParticles,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfIncomingLocalParticles,
    &globalNumberOfIncomingLocalParticles,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfLeavingLocalParticles,
    &globalNumberOfLeavingLocalParticles,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfMergedVirtualParticles,
    &globalNumberOfMergedVirtualParticles,
    1, MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getGlobalMasterRank(),
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

  tarch::mpi::Rank::getInstance().reduce(
    &_numberOfEliminatedSpatialDuplicates,
    &globalNumberOfEliminatedSpatialDuplicates,
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

  globalNumberOfRemainingLocalParticles     = _numberOfRemainingLocalParticles;
  globalNumberOfIncomingLocalParticles      = _numberOfIncomingLocalParticles;
  globalNumberOfLeavingLocalParticles       = _numberOfLeavingLocalParticles;

  globalNumberOfMergedVirtualParticles      = _numberOfMergedVirtualParticles;

  globalNumberOfEliminatedSpatialDuplicates = _numberOfEliminatedSpatialDuplicates;
  #endif

  bool log = tarch::mpi::Rank::getInstance().isGlobalMaster();
  #if PeanoDebug==0
  log &= (
      globalNumberOfLifts>0
   or globalNumberOfDrops>0
   or globalNumberOfSieves>0
   or globalNumberOfAssignmentsToGlobalSieve>0
   or globalNumberOfReassignments>0

   or globalNumberOfRemainingLocalParticles>0
   or globalNumberOfIncomingLocalParticles>0
   or globalNumberOfLeavingLocalParticles>0

   or globalNumberOfMergedVirtualParticles>0
   or globalNumberOfEliminatedSpatialDuplicates>0
      );
  #endif
  if ( log ) {
    logInfo(
      "reduceAndPrintStatistics()",
      "#lifts=" << globalNumberOfLifts <<
      ", #drops=" << globalNumberOfDrops <<
      ", #sieves=" << globalNumberOfSieves <<
      ", #assignments-to-sieve-set=" << globalNumberOfAssignmentsToGlobalSieve <<
      ", #reassignments=" << globalNumberOfReassignments <<

      ", #remaining local particles=" << globalNumberOfRemainingLocalParticles <<
      ", #incoming local particles=" << globalNumberOfIncomingLocalParticles <<
      ", #leaving local particles=" << globalNumberOfLeavingLocalParticles  <<

      ", #merged virtual particles=" << globalNumberOfMergedVirtualParticles <<
      ", #eliminated spatial duplicates=" << globalNumberOfEliminatedSpatialDuplicates
    );
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clearReassignmentStatistics() {
  _numberOfLifts                        = 0;
  _numberOfDrops                        = 0;
  _numberOfSieves                       = 0;
  _numberOfReassignments                = 0;
  _numberOfAssignmentsToGlobalSieve     = 0;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clearParticleStateStatistics() {
  _numberOfRemainingLocalParticles     = 0;
  _numberOfIncomingLocalParticles      = 0;
  _numberOfLeavingLocalParticles       = 0;

  _numberOfMergedVirtualParticles      = 0;

  _numberOfEliminatedSpatialDuplicates = 0;
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
  if ( p->getParallelState()==globaldata::{{PARTICLE_TYPE}}::ParallelState::Local ) {
    _particlesThatCanNotBeLiftedWithinTheirTree.push_back(p);
    _numberOfAssignmentsToGlobalSieve++;
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto *p: *this) {
    particleCanNotBeLiftedLocally(p);
  }
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getParticlesToBeSievedIntoVertex(
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateNumberOfLocalParticleStatistics(
  int numberOfRemainingLocalParticles,
  int numberOfIncomingLocalParticles,
  int numberOfLeavingLocalParticles
) {
  tarch::multicore::Lock lock( _semaphore );
  _numberOfRemainingLocalParticles += numberOfRemainingLocalParticles;
  _numberOfIncomingLocalParticles  += numberOfIncomingLocalParticles;
  _numberOfLeavingLocalParticles   += numberOfLeavingLocalParticles;
}
