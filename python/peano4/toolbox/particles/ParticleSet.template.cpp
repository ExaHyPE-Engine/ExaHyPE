// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "tarch/multicore/Lock.h"

#include "peano4/stacks/StackWithCustomSerialisation.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

#include <numeric> // std::accumulate

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(
  DoFType*                                     neighbourParticle,
  const ::peano4::datamanagement::VertexMarker&  marker,
  int                                          spacetreeId
) {
  assertion3(
    marker.isContainedInAdjacentCells(neighbourParticle->getX(), 0.5, toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE),
    neighbourParticle->toString(),
    marker.toString(),
    spacetreeId
  );
  const bool incomingParticleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( neighbourParticle->getX(), marker );
  Container::iterator iteratorToExistingCopy = ::toolbox::particles::particleIsDuplicate(neighbourParticle, _container.begin(), _container.end());

  if (neighbourParticle->getParallelState()==DoFType::ParallelState::Virtual and not incomingParticleWillBeLocal) {
    logDebug("incoming particle " << neighbourParticle->toString() << " tied to " << marker.toString() << " is virtual on neighbour. Such a copy is not relevant as it is a copy of the halo of another rank.");
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
    logDebug("incoming particle " << neighbourParticle->toString() << " tied to " << marker.toString() << " is virtual on neighbour and local here, i.e. neighbour sent me back my own copy. Drop on tree " << spacetreeId << " (marker=" << marker.toString() << ")");
    delete neighbourParticle;
    tarch::multicore::Lock lock( _statisticsSemaphore );
    _numberOfDroppedIncomingVirtualParticles++;
  }
  else if ( iteratorToExistingCopy!=end() ) {
    neighbourParticle->setParallelState( incomingParticleWillBeLocal ? DoFType::ParallelState::Local : DoFType::ParallelState::Virtual );

    #if PEANO_DEBUG
    neighbourParticle->setDebugX( (*iteratorToExistingCopy)->getDebugX() );
    int neighbourPartID = neighbourParticle->getPartid();
    #else
    int neighbourPartID = -1;
    #endif

    if ( (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local and incomingParticleWillBeLocal ) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfRedundantlySharedLocalParticles++;
      logDebug("inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing copy and will be local on tree  " << spacetreeId << " (marker=" << marker.toString() << ")");
    }
    else if (not incomingParticleWillBeLocal) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfReplacedVirtualParticlesAlongBoundary++;
      logDebug("inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing halo copy on tree  " << spacetreeId << " (marker=" << marker.toString() << ")");
    }
    else {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedLocalParticlesAlongBoundary++;
      logDebug("inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " replaces existing halo copy but is local, i.e. particle entered subdomain");
    }

    toolbox::particles::updateContainedInAdjacentCellProperty(marker, *neighbourParticle );
    toolbox::particles::assignmentchecks::detachParticleFromVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      neighbourPartID,
      (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::eraseParticle(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      neighbourPartID,
      (*iteratorToExistingCopy)->getParallelState()==DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      neighbourPartID,
      neighbourParticle->getParallelState()==DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")",
      marker.getX(),
      marker.getH()
    );

    delete *iteratorToExistingCopy;
    _container.erase(iteratorToExistingCopy);
    _container.push_back(neighbourParticle);
  }
  else {
    neighbourParticle->setParallelState( incomingParticleWillBeLocal ? DoFType::ParallelState::Local : DoFType::ParallelState::Virtual );

    #if PEANO_DEBUG
    int neighbourPartID = neighbourParticle->getPartid();
    #else
    int neighbourPartID = -1;
    #endif

    _container.push_back(neighbourParticle);

    if (incomingParticleWillBeLocal) {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedLocalParticlesAlongBoundary++;
      logDebug("inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " is new local vertex");
    }
    else {
      tarch::multicore::Lock lock( _statisticsSemaphore );
      _numberOfAddedVirtualParticlesAlongBoundary++;
      logDebug("inflying particle=" << neighbourParticle->toString() << " at vertex " << marker.toString() << " is new and will be remote on tree  " << spacetreeId << " (marker=" << marker.toString() << ")");
    }

    toolbox::particles::updateContainedInAdjacentCellProperty(marker, *neighbourParticle );
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      neighbourPartID,
      neighbourParticle->getParallelState()==DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker=" + marker.toString() + ")",
      marker.getX(),
      marker.getH()
    );
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  ::peano4::stacks::SendReceiveContext  context,
  const {{CLASSNAME}}&                                         neighbour,
  const ::peano4::datamanagement::VertexMarker&            marker,
  int                                                    spacetreeId
) {
  switch (context) {
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        assertionVectorNumericalEquals2( neighbour._debugH, _debugH, neighbour.toString(), toString() );

        logDebug("merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString());
        for (auto p: neighbour) {
          p->setX( toolbox::particles::mirrorParticleAlongPeriodicDomains(
            p->getX(),
            marker,
            DomainOffset,
            DomainSize,
            PeriodicBC
          ));
          mergeWithParticle(p, marker, spacetreeId);
        }

        logDebug("merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.toString() << " which now hosts " << this->size() << " particle(s)");
        for (auto *p: *this) {
          logDebug(" - " << p->toString());
        }

        assertion2( isValid(), toString(), marker.toString() );
      }
      break;
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      {
        assertionVectorNumericalEquals2( neighbour._debugX, _debugX, neighbour.toString(), toString() );
        assertionVectorNumericalEquals2( neighbour._debugH, _debugH, neighbour.toString(), toString() );

        logDebug("merge " << neighbour.size() << " particles from neighbour " << neighbour.toString() << " into local vertex set of " << marker.toString());
        for (auto p: neighbour) {
          mergeWithParticle(p, marker, spacetreeId);
        }

        logDebug("merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.toString() << " which now hosts " << this->size() << " particle(s)");
        for (auto *p: *this) {
          logDebug(" - " << p->toString());
        }

        assertion2( isValid(), toString(), marker.toString() );
      }
      break;

    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;

    case ::peano4::stacks::SendReceiveContext::ForkDomain:
      *this = neighbour;
      break;

    case ::peano4::stacks::SendReceiveContext::JoinDomain:
      assertionMsg( false, "not implemented yet, but should be plain copy" );
      break;

  }
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext context
) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const ::peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] ::peano4::stacks::SendReceiveContext context
) {
  return true;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}} {{NAMESPACE | join("::")}}::{{CLASSNAME}}::createExchangeView() {
    return {{CLASSNAME}}(*this);
}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream msg;
  msg << "(size=" << size()
      << ",";
  msg << Base::toString(false);
  msg << ")";
  return msg.str();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other):
  Base(other) {
  clone(other);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
  if (this != &other) {
    deleteParticles();
    Base::operator=(other);
    clone(other);
  }
  return *this;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}({{CLASSNAME}}&& other) noexcept:
  Base(std::move(static_cast<Base&>(other))),
  _container(std::move(other._container)) {
  other._container.clear();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=({{CLASSNAME}}&& other) noexcept {
  if (this != &other) {
    deleteParticles();
    static_cast<Base&>(*this) = std::move(static_cast<Base&>(other));
    _container = std::move(other._container);
    other._container.clear(); // TODO: Should not be required
  }
  return *this;
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
  logDebug("copied " << otherSet.size() << " objects over into local one of size " << size());
  #if PEANO_DEBUG
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

  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle );
  return _container.erase(particle);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto p: _container) {
    _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(p);
  }
  _container.clear();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(DoFType*  particle) {
  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(particle);
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

#ifdef WITH_MPI
MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryExchangeDatatype() {
  return {% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}::getBoundaryExchangeDatatype();
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMultiscaleDataExchangeDatatype() {
  return {% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}::getMultiscaleDataExchangeDatatype();
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getForkDatatype() {
  return {% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}::getForkDatatype();
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getJoinDatatype() {
  return {% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}::getJoinDatatype();
}

MPI_Datatype {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunicationDatatype() {
  return {% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}::getGlobalCommunicationDatatype();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSend(::peano4::stacks::Stack<{{CLASSNAME}}>& stack, const ::peano4::stacks::SendReceiveContext context, MPI_Comm comm) {
  auto& customStack = static_cast<::peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);

  const int numberOfElements = static_cast<int>(customStack.size());
  assertion1(numberOfElements >= 0, numberOfElements);

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  for (int i = 0; i < numberOfElements; ++i) {
    const {{CLASSNAME}}& element = stack[i];
    const int localCount = static_cast<int>(element._container.size());
    customStack.countBuffer[i] = localCount;
    for (DoFType* p : element._container) {
      if (p != nullptr) {
        customStack.dataBuffer.push_back(*p);
      } else {
        assertionMsg(false, "Found nullptr in {{CLASSNAME}} during startSend");
      }
    }
  }

  const int totalValues = static_cast<int>(customStack.dataBuffer.size());

  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = MPI_Isend(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }

  if (totalValues > 0) {
    [[maybe_unused]] const int result = MPI_Isend(
      customStack.dataBuffer.data(),
      totalValues,
      ::peano4::stacks::translateContextIntoDatatype<{{CLASSNAME}}>(context),
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiDataRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startReceive(::peano4::stacks::Stack<{{CLASSNAME}}>& stack, const ::peano4::stacks::SendReceiveContext context, MPI_Comm comm, int numberOfElements) {
  auto& customStack = static_cast<::peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);

  assertion1(numberOfElements >= 0, numberOfElements);
  assertion2(numberOfElements == static_cast<int>(customStack.size()), numberOfElements, customStack.size());

  customStack.mpiComm = comm;
  customStack.mpiContext = context;

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = MPI_Irecv(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::tryToFinishSendOrReceive(::peano4::stacks::Stack<{{CLASSNAME}}>& stack) {
  auto& customStack = static_cast<::peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);
  bool allDone = true;

  if (customStack.getIOMode() == ::peano4::stacks::IOMode::MPIReceive) {
    // Wait for counts to arrive
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      MPI_Test(&customStack.mpiCountRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;

        const int totalValues = std::accumulate(customStack.countBuffer.begin(), customStack.countBuffer.end(), 0);
        assertion(totalValues >= 0);
        customStack.dataBuffer.resize(totalValues);

        if (totalValues > 0) {
          [[maybe_unused]] const int result = MPI_Irecv(
            customStack.dataBuffer.data(),
            totalValues,
            ::peano4::stacks::translateContextIntoDatatype<{{CLASSNAME}}>(customStack.mpiContext),
            customStack.getActiveIORank(),
            customStack.getActiveIOTag(),
            customStack.mpiComm,
            &customStack.mpiDataRequest
          );
          assertion(result == MPI_SUCCESS);
          allDone = false;
        }
      } else {
        allDone = false;
      }
    }

    // Wait for data to arrive and unpack
    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      MPI_Test(&customStack.mpiDataRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;

        const int numberOfElements = static_cast<int>(customStack.size());
        assertion2(numberOfElements == static_cast<int>(customStack.countBuffer.size()), numberOfElements, customStack.countBuffer.size());

        std::size_t offset = 0;
        for (int i = 0; i < numberOfElements; ++i) {
          const int localCount = customStack.countBuffer[i];
          assertion1(localCount >= 0, localCount);

          {{CLASSNAME}}& element = customStack[i];

          // Clean existing data in the stack element (prepare for overwrite)
          element.deleteParticles();

          element._container.reserve(static_cast<std::size_t>(localCount));
          for (int j = 0; j < localCount; ++j) {
            assertion2(offset < customStack.dataBuffer.size(), offset, customStack.dataBuffer.size());
            // Reconstruct particles from flat buffer
            element.addParticle(new DoFType(customStack.dataBuffer[offset]));
            ++offset;
          }
        }
        assertion1(offset == customStack.dataBuffer.size(), offset);
      } else {
        allDone = false;
      }
    }
  // TODO: Is this branch necessary? -> Probably to make sure that allDone remains false
  } else if (customStack.getIOMode() == ::peano4::stacks::IOMode::MPISend) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      MPI_Test(&customStack.mpiCountRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }

    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      MPI_Test(&customStack.mpiDataRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }
  }

  return allDone;
}
#endif
