// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Request.h"
#include "tarch/multicore/Lock.h"

#include "peano4/stacks/StackWithCustomSerialisation.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

/** Consumes the incoming particle by storing it or deleting an irrelevant copy. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(
  DoFType* const neighbourParticle,
  const peano4::datamanagement::VertexMarker& marker,
  const int spacetreeId
) {
  [[maybe_unused]] const auto ownershipTolerance =
    toolbox::particles::ReleaseOwnershipSpatialSortingTolerance
    * tarch::la::max(marker.getH()) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  assertion(
    marker.isContainedInAdjacentCells(
      neighbourParticle->getX(),
      0.5,
      ownershipTolerance
    ),
    neighbourParticle->toString(),
    marker.toString(),
    spacetreeId
  );
  const bool incomingParticleWillBeLocal =
    toolbox::particles::particleAssignedToVertexWillBeLocal(neighbourParticle->getX(), marker);
  const auto iteratorToExistingCopy = toolbox::particles::particleIsDuplicate(
    neighbourParticle,
    _container.begin(),
    _container.end()
  );

  if (
    neighbourParticle->getParallelState() == DoFType::ParallelState::Virtual
    and not incomingParticleWillBeLocal
  ) {
    logDebug(
      "Discard virtual incoming particle " << neighbourParticle->toString()
                                            << " at " << marker.toString() << "."
    );
    delete neighbourParticle;
    tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
    ++_numberOfDroppedIncomingVirtualParticles;
  } else if (
    iteratorToExistingCopy != end()
    and neighbourParticle->getParallelState() == DoFType::ParallelState::Virtual
    and incomingParticleWillBeLocal
  ) {
    logDebug(
      "Discard returned virtual particle " << neighbourParticle->toString()
                                             << " on tree " << spacetreeId << "."
    );
    delete neighbourParticle;
    tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
    ++_numberOfDroppedIncomingVirtualParticles;
  } else if (iteratorToExistingCopy != end()) {
    neighbourParticle->setParallelState(
      incomingParticleWillBeLocal
        ? DoFType::ParallelState::Local
        : DoFType::ParallelState::Virtual
    );

#if PEANO_DEBUG
    neighbourParticle->setDebugX((*iteratorToExistingCopy)->getDebugX());
#endif
#if PEANO_ASSERTIONS
    const int particleId = neighbourParticle->getPartid();
#else
    const int particleId = -1;
#endif

    if (
      (*iteratorToExistingCopy)->getParallelState() == DoFType::ParallelState::Local
      and incomingParticleWillBeLocal
    ) {
      tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
      ++_numberOfRedundantlySharedLocalParticles;
      logDebug(
        "Replace a shared local particle with " << neighbourParticle->toString()
                                                 << " on tree " << spacetreeId << "."
      );
    } else if (not incomingParticleWillBeLocal) {
      tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
      ++_numberOfReplacedVirtualParticlesAlongBoundary;
      logDebug(
        "Refresh virtual particle " << neighbourParticle->toString()
                                     << " on tree " << spacetreeId << "."
      );
    } else {
      tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
      ++_numberOfAddedLocalParticlesAlongBoundary;
      logDebug(
        "Promote incoming particle " << neighbourParticle->toString()
                                      << " to local ownership."
      );
    }

    toolbox::particles::updateContainedInAdjacentCellProperty(marker, *neighbourParticle);
    toolbox::particles::assignmentchecks::detachParticleFromVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      particleId,
      (*iteratorToExistingCopy)->getParallelState() == DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker="
        + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::eraseParticle(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      particleId,
      (*iteratorToExistingCopy)->getParallelState() == DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker="
        + marker.toString() + ")"
    );
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      particleId,
      neighbourParticle->getParallelState() == DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker="
        + marker.toString() + ")",
      marker.getX(),
      marker.getH()
    );

    delete *iteratorToExistingCopy;
    _container.erase(iteratorToExistingCopy);
    _container.push_back(neighbourParticle);
  } else {
    neighbourParticle->setParallelState(
      incomingParticleWillBeLocal
        ? DoFType::ParallelState::Local
        : DoFType::ParallelState::Virtual
    );

#if PEANO_ASSERTIONS
    const int particleId = neighbourParticle->getPartid();
#else
    const int particleId = -1;
#endif

    _container.push_back(neighbourParticle);
    if (incomingParticleWillBeLocal) {
      tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
      ++_numberOfAddedLocalParticlesAlongBoundary;
      logDebug(
        "Add local incoming particle " << neighbourParticle->toString()
                                        << " at " << marker.toString() << "."
      );
    } else {
      tarch::multicore::Lock statisticsLock(_statisticsSemaphore);
      ++_numberOfAddedVirtualParticlesAlongBoundary;
      logDebug(
        "Add virtual incoming particle " << neighbourParticle->toString()
                                          << " on tree " << spacetreeId << "."
      );
    }

    toolbox::particles::updateContainedInAdjacentCellProperty(marker, *neighbourParticle);
    toolbox::particles::assignmentchecks::assignParticleToVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      neighbourParticle->getX(),
      particleId,
      neighbourParticle->getParallelState() == DoFType::ParallelState::Local,
      marker.getX(),
      marker.getH(),
      spacetreeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker="
        + marker.toString() + ")",
      marker.getX(),
      marker.getH()
    );
  }
}

/** Copies particles from the neighbour, which retains ownership of its storage. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  const peano4::stacks::SendReceiveContext context,
  const {{CLASSNAME}}& neighbour,
  const peano4::datamanagement::VertexMarker& marker,
  const int spacetreeId
) {
  switch (context) {
    case peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap: {
#if PEANO_DEBUG
      assertion(
        tarch::la::equals(neighbour._debugH, _debugH),
        neighbour._debugH,
        _debugH,
        neighbour.toString(),
        toString()
      );
#endif
      logDebug(
        "Merge " << neighbour.size() << " periodic particle copies from "
                 << neighbour.toString() << " into " << marker.toString() << "."
      );
      for (const auto* const particle : neighbour) {
        auto* const mirroredParticle = new DoFType(*particle);
        mirroredParticle->setX(toolbox::particles::mirrorParticleAlongPeriodicDomains(
          mirroredParticle->getX(),
          marker,
          DomainOffset,
          DomainSize,
          PeriodicBC
        ));
        mergeWithParticle(mirroredParticle, marker, spacetreeId);
      }
      assertion(isValid(), toString(), marker.toString());
    } break;
    case peano4::stacks::SendReceiveContext::BoundaryExchange: {
#if PEANO_DEBUG
      assertion(
        tarch::la::equals(neighbour._debugX, _debugX),
        neighbour._debugX,
        _debugX,
        neighbour.toString(),
        toString()
      );
      assertion(
        tarch::la::equals(neighbour._debugH, _debugH),
        neighbour._debugH,
        _debugH,
        neighbour.toString(),
        toString()
      );
#endif
      logDebug(
        "Merge " << neighbour.size() << " particle copies from " << neighbour.toString()
                 << " into " << marker.toString() << "."
      );
      for (const auto* const particle : neighbour) {
        mergeWithParticle(new DoFType(*particle), marker, spacetreeId);
      }
      assertion(isValid(), toString(), marker.toString());
    } break;
    case peano4::stacks::SendReceiveContext::MultiscaleExchange:
      assertion(false, "Particle sets do not support multiscale exchange.");
      break;
    case peano4::stacks::SendReceiveContext::ForkDomain:
    case peano4::stacks::SendReceiveContext::JoinDomain:
      *this = neighbour;
      break;
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] const peano4::stacks::SendReceiveContext context
) {
  return true;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] const peano4::stacks::SendReceiveContext context
) {
  return true;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::load(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker
) {
  return true;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::store(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker
) {
  return true;
}

/** Returns a deep copy with independent particle storage. */
{{NAMESPACE | join("::")}}::{{CLASSNAME}}
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::createExchangeView() {
  return {{CLASSNAME}}(*this);
}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream message;
  message << "(size=" << size() << "," << Base::toString(false) << ")";
  return message.str();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() noexcept {
  deleteParticles();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other) :
  Base(other) {
  clone(other);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}&
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
  if (this != &other) {
    deleteParticles();
    Base::operator=(other);
    clone(other);
  }
  return *this;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}({{CLASSNAME}}&& other) noexcept :
  Base(std::move(static_cast<Base&>(other))),
  _container(std::move(other._container)) {
  other._container.clear();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}&
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=({{CLASSNAME}}&& other) noexcept {
  if (this != &other) {
    deleteParticles();
    static_cast<Base&>(*this) = std::move(static_cast<Base&>(other));
    _container = std::move(other._container);
    other._container.clear();
  }
  return *this;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone(const {{CLASSNAME}}& sourceParticleSet) {
  assertion(_container.empty(), toString());
  for (const auto* const particle : sourceParticleSet._container) {
    assertion(
      particle->getParallelState() == DoFType::ParallelState::Local
        or particle->getParallelState() == DoFType::ParallelState::Virtual,
      particle->toString(),
      toString()
    );
    auto* const newParticle = new DoFType(*particle);
    _container.push_back(newParticle);
  }
  logDebug(
    "Clone " << sourceParticleSet.size() << " particles into a set of size " << size() << "."
  );
#if PEANO_DEBUG
  _debugX = sourceParticleSet._debugX;
  _debugH = sourceParticleSet._debugH;
#endif
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticles() noexcept {
  for (auto* const particle : _container) {
    delete particle;
  }
  _container.clear();
}

/** Removes particle pointers after their ownership has moved to another set. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clear() {
  _container.clear();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::particleCanNotBeLiftedLocally(
  iterator particle
) {
  assertion(
    (*particle)->getParallelState()
      == ::{{PARTICLE_FULL_QUALIFIED_TYPE}}::ParallelState::Local,
    (*particle)->toString()
  );
  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(*particle);
  return _container.erase(particle);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeLiftedLocally() {
  for (auto* const particle : _container) {
    _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(particle);
  }
  _container.clear();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(
  DoFType* const particle
) {
  _sieveParticles.addParticleThatCanNotBeLiftedWithinItsTree(particle);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::const_iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() const {
  return _container.begin();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::const_iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() const {
  return _container.end();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::begin() {
  return _container.begin();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::end() {
  return _container.end();
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::size() const {
  return static_cast<int>(_container.size());
}

/** Takes ownership of the particle. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::addParticle(DoFType* const particle) {
  assertion(
    particle->getParallelState() == DoFType::ParallelState::Local
      or particle->getParallelState() == DoFType::ParallelState::Virtual,
    particle->toString(),
    toString()
  );
  _container.push_back(particle);
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isValid() const {
  for (const auto* const particle : _container) {
    if (
      particle->getParallelState() != DoFType::ParallelState::Local
      and particle->getParallelState() != DoFType::ParallelState::Virtual
    ) {
      return false;
    }
  }
  return true;
}

/** Transfers every particle from the source set. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticles({{CLASSNAME}}& sourceParticleSet) {
  _container.insert(
    _container.end(),
    sourceParticleSet.begin(),
    sourceParticleSet.end()
  );
  sourceParticleSet.clear();
}

/** Transfers the selected particle from the source set. */
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::grabParticle(
  iterator particle,
  {{CLASSNAME}}& sourceParticleSet
) {
  assertion(
    (*particle)->getParallelState() == DoFType::ParallelState::Local
      or (*particle)->getParallelState() == DoFType::ParallelState::Virtual,
    (*particle)->toString(),
    toString(),
    sourceParticleSet.toString()
  );
  _container.push_back(*particle);
  return sourceParticleSet._container.erase(particle);
}

/** Deletes the selected particle and returns the next iterator. */
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::iterator
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticle(
  iterator particle
) {
  assertion(
    (*particle)->getParallelState() == DoFType::ParallelState::Local
      or (*particle)->getParallelState() == DoFType::ParallelState::Virtual,
    (*particle)->toString(),
    toString()
  );
  delete *particle;
  return _container.erase(particle);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSend(
  peano4::stacks::Stack<{{CLASSNAME}}>& stack,
  [[maybe_unused]] const peano4::stacks::SendReceiveContext context,
  const MPI_Comm communicator
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);
  const int numberOfElements = static_cast<int>(customStack.size());
  assertion(numberOfElements >= 0, numberOfElements);

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();
  for (int elementIndex = 0; elementIndex < numberOfElements; ++elementIndex) {
    const auto& element = stack[elementIndex];
    const int numberOfParticles = static_cast<int>(element._container.size());
    customStack.countBuffer[elementIndex] = numberOfParticles;
    for (const auto* const particle : element._container) {
      assertion(particle != nullptr, "A particle set cannot serialise a null pointer.");
      customStack.dataBuffer.push_back(*particle);
    }
  }

  const int totalNumberOfParticles = static_cast<int>(customStack.dataBuffer.size());
  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = tarch::mpi::isend(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }
  if (totalNumberOfParticles > 0) {
    [[maybe_unused]] const int result = tarch::mpi::isend(
      customStack.dataBuffer.data(),
      static_cast<int>(totalNumberOfParticles * sizeof(DoFType)),
      MPI_BYTE,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiDataRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startReceive(
  peano4::stacks::Stack<{{CLASSNAME}}>& stack,
  const peano4::stacks::SendReceiveContext context,
  const MPI_Comm communicator,
  const int numberOfElements
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);
  assertion(numberOfElements >= 0, numberOfElements);
  assertion(
    numberOfElements == static_cast<int>(customStack.size()),
    numberOfElements,
    customStack.size()
  );

  customStack.mpiComm = communicator;
  customStack.mpiContext = context;
  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();
  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = tarch::mpi::irecv(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::tryToFinishSendOrReceive(
  peano4::stacks::Stack<{{CLASSNAME}}>& stack
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<{{CLASSNAME}}>&>(stack);
  bool allDone = true;

  if (customStack.getIOMode() == peano4::stacks::IOMode::MPIReceive) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int communicationFinished = 0;
      [[maybe_unused]] const int result = tarch::mpi::test(
        &customStack.mpiCountRequest,
        &communicationFinished,
        MPI_STATUS_IGNORE
      );
      assertion(result == MPI_SUCCESS);
      if (communicationFinished != 0) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;
        const int totalNumberOfParticles = std::accumulate(
          customStack.countBuffer.begin(),
          customStack.countBuffer.end(),
          0
        );
        assertion(totalNumberOfParticles >= 0, totalNumberOfParticles);
        customStack.dataBuffer.resize(static_cast<std::size_t>(totalNumberOfParticles));

        if (totalNumberOfParticles > 0) {
          [[maybe_unused]] const int result = tarch::mpi::irecv(
            customStack.dataBuffer.data(),
            static_cast<int>(totalNumberOfParticles * sizeof(DoFType)),
            MPI_BYTE,
            customStack.getActiveIORank(),
            customStack.getActiveIOTag(),
            customStack.mpiComm,
            &customStack.mpiDataRequest
          );
          assertion(result == MPI_SUCCESS);
          allDone = false;
        } else {
          for (std::size_t elementIndex = 0; elementIndex < customStack.size(); ++elementIndex) {
            assertion(customStack.countBuffer[elementIndex] == 0, elementIndex);
            customStack[elementIndex].deleteParticles();
          }
        }
      } else {
        allDone = false;
      }
    }

    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int communicationFinished = 0;
      [[maybe_unused]] const int result = tarch::mpi::test(
        &customStack.mpiDataRequest,
        &communicationFinished,
        MPI_STATUS_IGNORE
      );
      assertion(result == MPI_SUCCESS);
      if (communicationFinished != 0) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;
        const int numberOfElements = static_cast<int>(customStack.size());
        assertion(
          numberOfElements == static_cast<int>(customStack.countBuffer.size()),
          numberOfElements,
          customStack.countBuffer.size()
        );

        std::size_t particleOffset = 0;
        for (int elementIndex = 0; elementIndex < numberOfElements; ++elementIndex) {
          const int numberOfParticles = customStack.countBuffer[elementIndex];
          assertion(numberOfParticles >= 0, numberOfParticles);
          auto& element = customStack[elementIndex];
          element.deleteParticles();
          element._container.reserve(static_cast<std::size_t>(numberOfParticles));
          for (int particleIndex = 0; particleIndex < numberOfParticles; ++particleIndex) {
            assertion(
              particleOffset < customStack.dataBuffer.size(),
              particleOffset,
              customStack.dataBuffer.size()
            );
            element.addParticle(new DoFType(customStack.dataBuffer[particleOffset]));
            ++particleOffset;
          }
        }
        assertion(particleOffset == customStack.dataBuffer.size(), particleOffset);
      } else {
        allDone = false;
      }
    }
  } else if (customStack.getIOMode() == peano4::stacks::IOMode::MPISend) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int communicationFinished = 0;
      [[maybe_unused]] const int result = tarch::mpi::test(
        &customStack.mpiCountRequest,
        &communicationFinished,
        MPI_STATUS_IGNORE
      );
      assertion(result == MPI_SUCCESS);
      if (communicationFinished != 0) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }
    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int communicationFinished = 0;
      [[maybe_unused]] const int result = tarch::mpi::test(
        &customStack.mpiDataRequest,
        &communicationFinished,
        MPI_STATUS_IGNORE
      );
      assertion(result == MPI_SUCCESS);
      if (communicationFinished != 0) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }
  }
  return allDone;
}
