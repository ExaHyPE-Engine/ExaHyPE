// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Request.h"
#include "tarch/Termination.h"

#include "peano4/stacks/StackWithCustomSerialisation.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"
#include "toolbox/particles/particles.h"

namespace {
  int getMpiElementCount(const std::size_t numberOfElements) {
    constexpr auto MaximumMpiCount = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (numberOfElements > MaximumMpiCount) {
      tarch::terminateWithError("The particle serialisation count exceeds MPI's maximum element count.");
    }
    return static_cast<int>(numberOfElements);
  }

  template <typename Value>
  int getMpiByteCount(const std::size_t numberOfValues) {
    constexpr auto MaximumMpiCount = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (numberOfValues > MaximumMpiCount / sizeof(Value)) {
      tarch::terminateWithError("The particle payload exceeds MPI's maximum byte count.");
    }
    return static_cast<int>(numberOfValues * sizeof(Value));
  }
} // namespace

/** Consumes the incoming particle by storing it or deleting an irrelevant copy. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(
  DoFType* const neighbourParticle,
  const peano4::datamanagement::VertexMarker& marker,
  const int treeId
) {
  [[maybe_unused]] const auto ownershipTolerance =
    toolbox::particles::ReleaseOwnershipSpatialSortingTolerance
    * tarch::la::max(marker.getH()) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  assertion(
    marker.isContainedInAdjacentCells(
      toolbox::particles::periodicImageTowardsVertex(neighbourParticle->getX(), marker),
      0.5,
      ownershipTolerance
    ),
    neighbourParticle->toString(),
    marker.toString(),
    treeId
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
    _numberOfDroppedIncomingVirtualParticles.fetch_add(1, std::memory_order_relaxed);
  } else if (
    iteratorToExistingCopy != end()
    and neighbourParticle->getParallelState() == DoFType::ParallelState::Virtual
    and incomingParticleWillBeLocal
  ) {
    logDebug(
      "Discard returned virtual particle " << neighbourParticle->toString()
                                             << " on tree " << treeId << "."
    );
    delete neighbourParticle;
    _numberOfDroppedIncomingVirtualParticles.fetch_add(1, std::memory_order_relaxed);
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
      _numberOfRedundantlyOwnedLocalParticles.fetch_add(1, std::memory_order_relaxed);
      logDebug(
        "Replace a particle both trees store as local with " << neighbourParticle->toString()
                                                              << " on tree " << treeId << "."
      );
    } else if (not incomingParticleWillBeLocal) {
      _numberOfReplacedVirtualParticlesAlongBoundary.fetch_add(1, std::memory_order_relaxed);
      logDebug(
        "Refresh virtual particle " << neighbourParticle->toString()
                                     << " on tree " << treeId << "."
      );
    } else {
      _numberOfAddedLocalParticlesAlongBoundary.fetch_add(1, std::memory_order_relaxed);
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
      treeId,
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
      treeId,
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
      treeId,
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
      _numberOfAddedLocalParticlesAlongBoundary.fetch_add(1, std::memory_order_relaxed);
      logDebug(
        "Add local incoming particle " << neighbourParticle->toString()
                                        << " at " << marker.toString() << "."
      );
    } else {
      _numberOfAddedVirtualParticlesAlongBoundary.fetch_add(1, std::memory_order_relaxed);
      logDebug(
        "Add virtual incoming particle " << neighbourParticle->toString()
                                          << " on tree " << treeId << "."
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
      treeId,
      "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeWithParticle(marker="
        + marker.toString() + ")",
      marker.getX(),
      marker.getH()
    );
  }
}

/**
 * Deletes every particle in the set and records each deletion.
 *
 * The rank overwrites its copy of a vertex that several subtree tasks touch
 * with those tasks' copies. detachParticleFromVertex() and eraseParticle()
 * record the removal of the deleted particles from the assignment history.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticlesForSlotRebuild(
  const peano4::datamanagement::VertexMarker& marker,
  const int treeId
) {
  if (_container.empty()) {
    return;
  }
  const std::string trace =
    "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::deleteParticlesForSlotRebuild(marker="
    + marker.toString() + ")";
  for (DoFType* const particle : _container) {
#if PEANO_ASSERTIONS
    const int particleId = particle->getPartid();
#else
    const int particleId = -1;
#endif
    const bool particleIsLocal = particle->getParallelState() == DoFType::ParallelState::Local;

    toolbox::particles::assignmentchecks::detachParticleFromVertex(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      particle->getX(),
      particleId,
      particleIsLocal,
      marker.getX(),
      marker.getH(),
      treeId,
      trace
    );
    toolbox::particles::assignmentchecks::eraseParticle(
      toolbox::particles::assignmentchecks::pruneTypeName<DoFType>(),
      particle->getX(),
      particleId,
      particleIsLocal,
      marker.getX(),
      marker.getH(),
      treeId,
      trace
    );
    delete particle;
  }
  _container.clear();
}

/** Merges a neighbour rank's or subtree task's particle-set copy. */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  const peano4::stacks::SendReceiveContext context,
  const {{CLASSNAME}}& neighbour,
  const peano4::datamanagement::VertexMarker& marker,
  const int treeId
) {
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

  if (context == peano4::stacks::SendReceiveContext::Consolidation) {
    deleteParticlesForSlotRebuild(marker, treeId);
  }

  logDebug(
    "Merge " << neighbour.size() << " particle copies from " << neighbour.toString()
             << " into " << marker.toString() << "."
  );
  for (const auto* const particle : neighbour) {
    // The neighbour owns and frees its particles.
    mergeWithParticle(new DoFType(*particle), marker, treeId);
  }

  assertion(isValid(), toString(), marker.toString());
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

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other) :
  Base(other) {
  clone(other);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}({{CLASSNAME}}&& other) noexcept :
  Base(static_cast<Base&>(other)),
  _container(std::move(other._container)) {
  other._container.clear();
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

{{NAMESPACE | join("::")}}::{{CLASSNAME}}&
{{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=({{CLASSNAME}}&& other) noexcept {
  if (this != &other) {
    deleteParticles();
    static_cast<Base&>(*this) = static_cast<Base&>(other);
    _container = std::move(other._container);
    other._container.clear();
  }
  return *this;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() noexcept {
  deleteParticles();
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
  return std::ranges::all_of(_container, [](const auto* const particle) {
    return particle->getParallelState() == DoFType::ParallelState::Local
           or particle->getParallelState() == DoFType::ParallelState::Virtual;
  });
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
  const auto numberOfElements = customStack.size();
  const int mpiNumberOfElements = getMpiElementCount(numberOfElements);
  if (numberOfElements > customStack.countBuffer.max_size()) {
    tarch::terminateWithError("The particle count buffer cannot represent the stack.");
  }

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  std::size_t totalNumberOfParticles{0};
  for (std::size_t elementIndex = 0; elementIndex < numberOfElements; ++elementIndex) {
    const auto& element = stack[elementIndex];
    const auto numberOfParticles = element._container.size();
    customStack.countBuffer[elementIndex] = getMpiElementCount(numberOfParticles);

    if (numberOfParticles > customStack.dataBuffer.max_size() - totalNumberOfParticles) {
      tarch::terminateWithError("The particle payload exceeds the serialisation buffer's capacity.");
    }
    totalNumberOfParticles += numberOfParticles;

    for (const DoFType* const particle : element._container) {
      if (particle == nullptr) {
        tarch::terminateWithError("The particle payload contains a null pointer.");
      }
    }
  }

  const int numberOfPayloadBytes = getMpiByteCount<DoFType>(totalNumberOfParticles);
  customStack.dataBuffer.reserve(totalNumberOfParticles);
  for (std::size_t elementIndex = 0; elementIndex < numberOfElements; ++elementIndex) {
    for (const DoFType* const particle : stack[elementIndex]._container) {
      customStack.dataBuffer.push_back(*particle);
    }
  }

  if (numberOfElements > 0) {
    tarch::mpi::isend(
      customStack.countBuffer.data(),
      mpiNumberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiCountRequest
    );
  }
  if (totalNumberOfParticles > 0) {
    tarch::mpi::isend(
      customStack.dataBuffer.data(),
      numberOfPayloadBytes,
      MPI_BYTE,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiDataRequest
    );
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
  if (numberOfElements < 0) {
    tarch::terminateWithError("The received particle-stack element count is negative.");
  }
  const auto numberOfElementsAsSize = static_cast<std::size_t>(numberOfElements);
  if (numberOfElementsAsSize != customStack.size()) {
    tarch::terminateWithError("The received particle-stack element count does not match the destination stack.");
  }
  if (numberOfElementsAsSize > customStack.countBuffer.max_size()) {
    tarch::terminateWithError("The particle count buffer cannot represent the received stack.");
  }

  customStack.mpiComm = communicator;
  customStack.mpiContext = context;
  customStack.countBuffer.assign(numberOfElementsAsSize, 0);
  customStack.dataBuffer.clear();
  if (numberOfElements > 0) {
    tarch::mpi::irecv(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      communicator,
      &customStack.mpiCountRequest
    );
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
        std::size_t totalNumberOfParticles{0};
        for (const int numberOfParticles : customStack.countBuffer) {
          if (numberOfParticles < 0) {
            tarch::terminateWithError("The received particle payload contains a negative element count.");
          }
          const auto numberOfParticlesAsSize = static_cast<std::size_t>(numberOfParticles);
          if (numberOfParticlesAsSize > customStack.dataBuffer.max_size() - totalNumberOfParticles) {
            tarch::terminateWithError("The received particle payload exceeds the serialisation buffer's capacity.");
          }
          totalNumberOfParticles += numberOfParticlesAsSize;
        }
        const int numberOfPayloadBytes = getMpiByteCount<DoFType>(totalNumberOfParticles);
        customStack.dataBuffer.resize(totalNumberOfParticles);

        if (totalNumberOfParticles > 0) {
          tarch::mpi::irecv(
            customStack.dataBuffer.data(),
            numberOfPayloadBytes,
            MPI_BYTE,
            customStack.getActiveIORank(),
            customStack.getActiveIOTag(),
            customStack.mpiComm,
            &customStack.mpiDataRequest
          );
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
        const auto numberOfElements = customStack.size();
        if (numberOfElements != customStack.countBuffer.size()) {
          tarch::terminateWithError("The particle count buffer does not match the destination stack.");
        }

        std::size_t particleOffset = 0;
        for (std::size_t elementIndex = 0; elementIndex < numberOfElements; ++elementIndex) {
          const int numberOfParticles = customStack.countBuffer[elementIndex];
          if (numberOfParticles < 0) {
            tarch::terminateWithError("The received particle payload contains a negative element count.");
          }
          auto& element = customStack[elementIndex];
          element.deleteParticles();
          const auto numberOfParticlesAsSize = static_cast<std::size_t>(numberOfParticles);
          if (numberOfParticlesAsSize > customStack.dataBuffer.size() - particleOffset) {
            tarch::terminateWithError("The received particle counts exceed the payload buffer.");
          }
          element._container.reserve(numberOfParticlesAsSize);
          for (std::size_t particleIndex = 0; particleIndex < numberOfParticlesAsSize; ++particleIndex) {
            element.addParticle(new DoFType(customStack.dataBuffer[particleOffset]));
            ++particleOffset;
          }
        }
        if (particleOffset != customStack.dataBuffer.size()) {
          tarch::terminateWithError("The received particle counts do not consume the payload buffer.");
        }
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
