// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlService.h"

#include "exahype2/RefinementControl.h"
#include "peano4/Globals.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

#include <mpi.h>

namespace {
  /**
   * Names the Peano parent cell a leaf-sized erase request belongs to. Level and lattice
   * position are integers, so every sibling of a group produces the same key.
   */
  struct EraseParentKey {
    std::int32_t                      parentLevel{0};
    std::array<long long, DIMENSIONS> parentIndex{};

    bool operator<(const EraseParentKey& other) const noexcept {
      if (parentLevel != other.parentLevel) {
        return parentLevel < other.parentLevel;
      }
      return parentIndex < other.parentIndex;
    }
  };

  /** Records one solver's erase vote for one child slot of a parent cell. */
  struct EraseVote {
    EraseParentKey key{};
    std::int32_t   solverNumber{0};
    std::int32_t   linearisedChildPosition{0};
  };

  long long floorDivideByThree(const long long value) noexcept { return value >= 0 ? value / 3 : -((-value + 2) / 3); }

  /**
   * Maps a leaf-sized erase request onto its Peano parent cell.
   *
   * Dividing the requesting cell's lattice position by three names its parent and the child's
   * slot within the parent's 3^d sibling group. The routine returns false for a request from
   * the root cell, which has no parent.
   */
  bool createEraseParent(
    const peano4::grid::GridControlEvent& event,
    EraseParentKey&                       key,
    int&                                  linearisedChildPosition
  ) {
    if (event.level < 1) {
      return false;
    }

    key.parentLevel         = event.level - 1;
    linearisedChildPosition = 0;
    int childStride         = 1;

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const long long cellIndex           = event.latticeIndex(axis);
      const long long parentIndex         = floorDivideByThree(cellIndex);
      const int       childPositionOnAxis = static_cast<int>(cellIndex - 3 * parentIndex);

      key.parentIndex[axis] = parentIndex;

      linearisedChildPosition += childPositionOnAxis * childStride;
      childStride *= 3;
    }

    return true;
  }
} // namespace

exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService instance;
  return instance;
}

exahype2::RefinementControlService::RefinementControlService() {
  _contributionsPerThread.resize(std::max(tarch::tasking::Runtime::getInstance().getNumberOfThreads(), 1));
}

/**
 * Runs before tarch shuts MPI down. The delta exchange uses predefined MPI
 * types only, so nothing has to be released.
 */
void exahype2::RefinementControlService::shutdown() {}

std::string exahype2::RefinementControlService::toString() const {
  std::size_t numberOfNewLocalEvents = 0;
  for (const auto& threadContributions : _contributionsPerThread) {
    for (const auto& contribution : threadContributions) {
      numberOfNewLocalEvents += contribution.size();
    }
  }

  std::ostringstream description;
  description << "RefinementControlService{";
  description << "#NewLocalEvents=" << numberOfNewLocalEvents;
  description << ", #CommittedEvents=" << _committedEvents.size();
  description << ", #LastRealisedEvents=" << _lastRealisedEvents.size();
  description << ", TopologyChangedSinceLastTimeStep=" << (_topologyChangedSinceLastTimeStep ? "true" : "false");
  description << ", #Solvers=" << _numberOfSolvers;
  description << "}";
  return description.str();
}

void exahype2::RefinementControlService::merge(const RefinementControl& control) {
  const int threadNumber = tarch::tasking::Runtime::getInstance().getThreadNumber();
  assertion(
    threadNumber >= 0 and threadNumber < static_cast<int>(_contributionsPerThread.size()),
    threadNumber,
    _contributionsPerThread.size()
  );
  _contributionsPerThread[threadNumber].push_back(control._newEvents);
}

/** Commits the events of a traversal that neither suppresses nor records them. */
void exahype2::RefinementControlService::finishStep() { finishStep(false, false); }

/**
 * Commits the events collected by a refinement criterion traversal.
 *
 * If the merged event set is identical to the last event set that a mesh
 * refinement cycle fully realised, the events are dropped: Reissuing the same
 * cell requests cannot change the mesh again.
 */
void exahype2::RefinementControlService::finishRefinementCriterionStep() {
  // A silenced criterion produces an empty request set, which says nothing about a
  // settled mesh.
  _lastCriterionStepWasSuppressed   = _topologyChangedSinceLastTimeStep;
  _topologyChangedSinceLastTimeStep = false;
  finishStep(true, false);
}

/**
 * Finishes a mesh refinement realisation traversal.
 *
 * The completed traversal consumed its committed event set. Recording that set
 * before suppression lets an unchanged criterion settle immediately, while a
 * request on a newly created cell remains as the next batch.
 */
void exahype2::RefinementControlService::finishDynamicMeshRefinementStep() { finishStep(true, true); }

/**
 * Records the committed set as realised and drops it. A caller uses it for a
 * committed set the refinement plan rejects, so no traversal runs for it and
 * the topology stays as it is. Every rank holds the same committed set, so
 * the record stays rank-uniform.
 */
void exahype2::RefinementControlService::realiseCommittedEventsWithoutSweep() {
  assertion(not _committedEvents.empty());
  _lastRealisedEvents = std::move(_committedEvents);
  _committedEvents.clear();
}

/**
 * Reduces the requests of one completed traversal into the committed event set.
 *
 * The raw requests are exchanged over MPI before the coarsening consensus runs,
 * so a sibling group whose children are spread over several ranks is reassembled
 * on each of them. A step whose contributions repeat those of the last step of
 * the same kind on every rank reuses that step's committed set.
 */
void exahype2::RefinementControlService::finishStep(
  const bool suppressEventsIfUnchanged,
  const bool recordRealisedEvents
) {
  const std::vector<peano4::grid::GridControlEvent> eventsUsedByCompletedTraversal = std::move(_committedEvents);
  _committedEvents.clear();

  if (recordRealisedEvents and not eventsUsedByCompletedTraversal.empty()) {
    _topologyChangedSinceLastTimeStep = true;
    _lastRealisedEvents               = eventsUsedByCompletedTraversal;
  }

  const auto stepKind    = recordRealisedEvents ? StepKind::Realise
       : suppressEventsIfUnchanged              ? StepKind::Criterion
                                                : StepKind::Plain;
  auto&      lastOutcome = _lastOutcomePerKind[static_cast<int>(stepKind)];

  // An empty contribution names no cell and drops here.
  std::vector<RefinementControl::NewEvents> contributions;
  for (auto& threadContributions : _contributionsPerThread) {
    for (auto& contribution : threadContributions) {
      if (not contribution.empty()) {
        contributions.push_back(std::move(contribution));
      }
    }
    threadContributions.clear();
  }
  std::sort(contributions.begin(), contributions.end());
  const bool localRequestsUnchanged = contributions == lastOutcome.contributions;
  lastOutcome.contributions         = std::move(contributions);

  std::vector<RawRequest> rawRequests;
  if (not localRequestsUnchanged) {
    for (const auto& contribution : lastOutcome.contributions) {
      for (const auto& newEvent : contribution) {
        rawRequests.push_back(RawRequest{newEvent.event, static_cast<std::int32_t>(newEvent.solverNumber)});
      }
    }
    logDebug("Activate " << rawRequests.size() << " local refinement/erase instruction(s) for the next grid sweep");
  }

  if (exchangeRawRequests(rawRequests, stepKind, localRequestsUnchanged)) {
    _committedEvents = lastOutcome.committedEvents;
  } else {
    // Observer clones may request the same cell, so the duplicates collapse here.
    _committedEvents = finaliseCoarseningEvents(rawRequests);
    std::sort(_committedEvents.begin(), _committedEvents.end());
    _committedEvents.erase(std::unique(_committedEvents.begin(), _committedEvents.end()), _committedEvents.end());
    lastOutcome.committedEvents = _committedEvents;
  }

  if (suppressEventsIfUnchanged and _committedEvents == _lastRealisedEvents) {
    logDebug("Drop " << _committedEvents.size() << " already realised refinement/erase instruction(s)");
    _committedEvents.clear();
  }
}

/**
 * Promotes patch-local erase requests to the parent cell they coarsen away.
 *
 * Every erase request a solver raises is mapped onto its Peano parent cell; a parent
 * survives only if every solver has requested the erase for the complete 3^d sibling
 * group. Incomplete groups are dropped.
 */
std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::finaliseCoarseningEvents(
  const std::vector<RawRequest>& requests
) const {
  assertion(_numberOfSolvers >= 1, _numberOfSolvers);
  std::vector<peano4::grid::GridControlEvent> finalisedEvents;
  finalisedEvents.reserve(requests.size());

  std::vector<EraseVote> eraseVotes;
  for (const auto& request : requests) {
    const auto& event = request.event;
    if (event.getRefinementControl() != peano4::grid::GridControlEvent::RefinementControl::Erase) {
      finalisedEvents.push_back(event);
      continue;
    }

    EraseParentKey parentKey;
    int            linearisedChildPosition = 0;
    if (createEraseParent(event, parentKey, linearisedChildPosition)) {
      eraseVotes.push_back({parentKey, request.solverNumber, static_cast<std::int32_t>(linearisedChildPosition)});
    }
  }

  // Sorting groups the votes by parent cell and solver, so the consensus reads
  // each group in one scan and the parents emit in ascending key order.
  std::sort(eraseVotes.begin(), eraseVotes.end(), [](const EraseVote& left, const EraseVote& right) {
    if (left.key < right.key) {
      return true;
    }
    if (right.key < left.key) {
      return false;
    }
    return left.solverNumber < right.solverNumber;
  });

  std::size_t voteNumber = 0;
  while (voteNumber < eraseVotes.size()) {
    const auto groupKey = eraseVotes[voteNumber].key;
    // Coarsening is unanimous: Every solver has to request the complete sibling group.
    int  numberOfVotingSolvers = 0;
    bool everySolverAgrees     = true;
    while (voteNumber < eraseVotes.size() and not(groupKey < eraseVotes[voteNumber].key)) {
      const auto                 solverNumber = eraseVotes[voteNumber].solverNumber;
      std::bitset<THREE_POWER_D> children;
      while (voteNumber < eraseVotes.size() and not(groupKey < eraseVotes[voteNumber].key)
             and eraseVotes[voteNumber].solverNumber == solverNumber) {
        children.set(eraseVotes[voteNumber].linearisedChildPosition);
        voteNumber++;
      }
      numberOfVotingSolvers++;
      everySolverAgrees = everySolverAgrees and children.count() == THREE_POWER_D;
    }
    if (everySolverAgrees and numberOfVotingSolvers == _numberOfSolvers) {
      peano4::grid::GridControlEvent parentEvent;
      parentEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
      parentEvent.level = groupKey.parentLevel;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        parentEvent.latticeIndex(axis) = static_cast<int>(groupKey.parentIndex[axis]);
      }
      finalisedEvents.push_back(parentEvent);
    }
  }

  return finalisedEvents;
}

/** Orders raw requests totally, so every rank's list has one canonical form. */
bool exahype2::RefinementControlService::rawRequestPrecedes(const RawRequest& left, const RawRequest& right) {
  if (left.event < right.event) {
    return true;
  }
  if (right.event < left.event) {
    return false;
  }
  return left.solverNumber < right.solverNumber;
}

/** Returns whether two raw requests carry the same content. */
bool exahype2::RefinementControlService::rawRequestsMatch(const RawRequest& left, const RawRequest& right) {
  return not rawRequestPrecedes(left, right) and not rawRequestPrecedes(right, left);
}

/**
 * Replaces the local raw requests by the union over all ranks and returns
 * whether every rank's list repeats the one it shipped at the last step of
 * the same kind.
 *
 * The collective carries only this rank's delta against the list it shipped at
 * the last step of the same kind, with removals as indices into that list and
 * additions as whole requests. Every rank applies all deltas to its copy of the
 * lists, so all ranks reconstruct identical lists and their concatenation in
 * rank order.
 */
bool exahype2::RefinementControlService::exchangeRawRequests(
  std::vector<RawRequest>& requests,
  const StepKind           stepKind,
  const bool               localRequestsUnchanged
) {
  if (not localRequestsUnchanged) {
    std::sort(requests.begin(), requests.end(), rawRequestPrecedes);
    requests.erase(std::unique(requests.begin(), requests.end(), rawRequestsMatch), requests.end());
  }

  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks == 1) {
    return localRequestsUnchanged;
  }
  const int rank = tarch::mpi::Rank::getInstance().getRank();

  auto& shippedRequestsOfRanks = _shippedRequestsPerKindAndRank[static_cast<int>(stepKind)];
  if (shippedRequestsOfRanks.empty()) {
    shippedRequestsOfRanks.resize(numberOfRanks);
  }
  const auto& baseline = shippedRequestsOfRanks[rank];

  // One sorted walk yields the delta: Baseline entries the current list lacks
  // leave as removal indices, current entries the baseline lacks as additions.
  std::vector<std::int32_t> removalIndices;
  std::vector<RawRequest>   additions;
  if (not localRequestsUnchanged) {
    std::size_t baselineNumber = 0;
    std::size_t currentNumber  = 0;
    while (baselineNumber < baseline.size() and currentNumber < requests.size()) {
      if (rawRequestPrecedes(baseline[baselineNumber], requests[currentNumber])) {
        removalIndices.push_back(static_cast<std::int32_t>(baselineNumber));
        baselineNumber++;
      } else if (rawRequestPrecedes(requests[currentNumber], baseline[baselineNumber])) {
        additions.push_back(requests[currentNumber]);
        currentNumber++;
      } else {
        baselineNumber++;
        currentNumber++;
      }
    }
    for (; baselineNumber < baseline.size(); baselineNumber++) {
      removalIndices.push_back(static_cast<std::int32_t>(baselineNumber));
    }
    for (; currentNumber < requests.size(); currentNumber++) {
      additions.push_back(requests[currentNumber]);
    }
  }

  // The delta travels as one byte blob: Two counts, the removal indices, and
  // the added requests, copied without alignment assumptions.
  const std::size_t localBytes =
    2 * sizeof(std::int32_t) + removalIndices.size() * sizeof(std::int32_t) + additions.size() * sizeof(RawRequest);
  if (localBytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    tarch::terminateWithError("The refinement-request delta exceeds the collective exchange's representable range.");
  }
  std::vector<char> localBlob(localBytes);
  {
    const auto  numberOfRemovals  = static_cast<std::int32_t>(removalIndices.size());
    const auto  numberOfAdditions = static_cast<std::int32_t>(additions.size());
    std::size_t writeOffset       = 0;
    std::memcpy(localBlob.data() + writeOffset, &numberOfRemovals, sizeof(std::int32_t));
    writeOffset += sizeof(std::int32_t);
    std::memcpy(localBlob.data() + writeOffset, &numberOfAdditions, sizeof(std::int32_t));
    writeOffset += sizeof(std::int32_t);
    if (not removalIndices.empty()) {
      std::memcpy(localBlob.data() + writeOffset, removalIndices.data(), removalIndices.size() * sizeof(std::int32_t));
      writeOffset += removalIndices.size() * sizeof(std::int32_t);
    }
    if (not additions.empty()) {
      std::memcpy(localBlob.data() + writeOffset, additions.data(), additions.size() * sizeof(RawRequest));
    }
  }

  const int        localBlobSize = static_cast<int>(localBytes);
  std::vector<int> blobSizes(numberOfRanks, 0);
  tarch::mpi::allgather(
    &localBlobSize,
    1,
    MPI_INT,
    blobSizes.data(),
    1,
    MPI_INT,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  constexpr auto   MaximumCollectiveBytes = static_cast<std::size_t>(std::numeric_limits<int>::max());
  constexpr auto   EmptyDeltaBytes        = static_cast<int>(2 * sizeof(std::int32_t));
  std::vector<int> displacements(numberOfRanks, 0);
  std::size_t      totalBytes      = 0;
  bool             everyDeltaEmpty = true;
  for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
    const auto rankBytes = static_cast<std::size_t>(blobSizes[rankIndex]);
    if (rankBytes > MaximumCollectiveBytes - totalBytes) {
      tarch::terminateWithError(
        "The global refinement-request delta exceeds the collective exchange's representable range."
      );
    }
    displacements[rankIndex] = static_cast<int>(totalBytes);
    totalBytes += rankBytes;
    everyDeltaEmpty = everyDeltaEmpty and blobSizes[rankIndex] == EmptyDeltaBytes;
  }
  if (everyDeltaEmpty) {
    return true;
  }

  std::vector<char> gatheredBlobs(totalBytes);
  tarch::mpi::allgatherv(
    localBlob.data(),
    localBlobSize,
    MPI_BYTE,
    gatheredBlobs.data(),
    blobSizes.data(),
    displacements.data(),
    MPI_BYTE,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  std::size_t totalRequestCount = 0;
  for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
    std::size_t  readOffset = static_cast<std::size_t>(displacements[rankIndex]);
    std::int32_t numberOfRemovals{};
    std::int32_t numberOfAdditions{};
    std::memcpy(&numberOfRemovals, gatheredBlobs.data() + readOffset, sizeof(std::int32_t));
    readOffset += sizeof(std::int32_t);
    std::memcpy(&numberOfAdditions, gatheredBlobs.data() + readOffset, sizeof(std::int32_t));
    readOffset += sizeof(std::int32_t);

    auto&                   shippedRequests = shippedRequestsOfRanks[rankIndex];
    std::vector<RawRequest> reconstructed;
    reconstructed.reserve(shippedRequests.size() + numberOfAdditions);

    // The removal indices and the additions are each sorted, so one merge over
    // the baseline reproduces the sender's canonical list.
    std::size_t  nextRemoval  = 0;
    std::int32_t removalIndex = -1;
    if (numberOfRemovals > 0) {
      std::memcpy(&removalIndex, gatheredBlobs.data() + readOffset, sizeof(std::int32_t));
    }
    const std::size_t additionsOffset = readOffset + numberOfRemovals * sizeof(std::int32_t);
    RawRequest        nextAddition;
    std::size_t       additionNumber = 0;
    if (numberOfAdditions > 0) {
      std::memcpy(&nextAddition, gatheredBlobs.data() + additionsOffset, sizeof(RawRequest));
    }
    for (std::size_t baselineNumber = 0; baselineNumber < shippedRequests.size(); baselineNumber++) {
      if (removalIndex == static_cast<std::int32_t>(baselineNumber)) {
        nextRemoval++;
        if (nextRemoval < static_cast<std::size_t>(numberOfRemovals)) {
          std::memcpy(
            &removalIndex,
            gatheredBlobs.data() + readOffset + nextRemoval * sizeof(std::int32_t),
            sizeof(std::int32_t)
          );
        } else {
          removalIndex = -1;
        }
        continue;
      }
      while (additionNumber < static_cast<std::size_t>(numberOfAdditions)
             and rawRequestPrecedes(nextAddition, shippedRequests[baselineNumber])) {
        reconstructed.push_back(nextAddition);
        additionNumber++;
        if (additionNumber < static_cast<std::size_t>(numberOfAdditions)) {
          std::memcpy(
            &nextAddition,
            gatheredBlobs.data() + additionsOffset + additionNumber * sizeof(RawRequest),
            sizeof(RawRequest)
          );
        }
      }
      reconstructed.push_back(shippedRequests[baselineNumber]);
    }
    for (; additionNumber < static_cast<std::size_t>(numberOfAdditions); additionNumber++) {
      std::memcpy(
        &nextAddition,
        gatheredBlobs.data() + additionsOffset + additionNumber * sizeof(RawRequest),
        sizeof(RawRequest)
      );
      reconstructed.push_back(nextAddition);
    }

    shippedRequests = std::move(reconstructed);
    // Every rank reconstructs every rank's list, and the suppression, consensus
    // and redelivery decisions only agree while all of them are canonical.
    assertion(std::is_sorted(shippedRequests.begin(), shippedRequests.end(), rawRequestPrecedes), rankIndex);
    assertion(
      std::adjacent_find(shippedRequests.begin(), shippedRequests.end(), rawRequestsMatch) == shippedRequests.end(),
      rankIndex
    );
    totalRequestCount += shippedRequests.size();
  }

  std::vector<RawRequest> gatheredRequests;
  gatheredRequests.reserve(totalRequestCount);
  for (int rankIndex = 0; rankIndex < numberOfRanks; rankIndex++) {
    gatheredRequests.insert(
      gatheredRequests.end(),
      shippedRequestsOfRanks[rankIndex].begin(),
      shippedRequestsOfRanks[rankIndex].end()
    );
  }
  requests = std::move(gatheredRequests);
  return false;
}

/**
 * Delivers the requests active for the next mesh traversal.
 *
 * The committed set is identical on every rank, and the suppression and
 * redelivery decisions rely on that. Each rank derives the same refinement and
 * ownership plans from it, then materialises its rank-local view.
 */
std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents() const {
  return _committedEvents;
}

/**
 * Returns whether a committed grid-control event exists for the next mesh traversal.
 *
 * The predicate distinguishes a sweep that produced mesh work from one that did not,
 * so a caller can restrict AMR-only sweeps to the steps that need them.
 */
bool exahype2::RefinementControlService::hasGridControlEvents() const { return not _committedEvents.empty(); }

/**
 * Returns whether every rank holds the same committed event set. One hash of
 * the set reduces over the ranks to its minimum and its maximum, so the check
 * costs one collective per call and serves an assertion at the start of a
 * sweep.
 */
bool exahype2::RefinementControlService::committedEventsAgreeAcrossRanks() const {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    return true;
  }
  constexpr unsigned long long FnvOffsetBasis = 1469598103934665603ULL;
  constexpr unsigned long long FnvPrime       = 1099511628211ULL;
  unsigned long long           localHash      = FnvOffsetBasis;
  const auto                   mix            = [&localHash](const unsigned long long value) {
    localHash ^= value;
    localHash *= FnvPrime;
  };
  mix(static_cast<unsigned long long>(_committedEvents.size()));
  for (const auto& event : _committedEvents) {
    mix(static_cast<unsigned long long>(static_cast<int>(event.getRefinementControl())));
    mix(static_cast<unsigned long long>(event.level));
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      mix(static_cast<unsigned long long>(event.latticeIndex(axis)));
    }
  }
  unsigned long long minimumHash  = 0;
  unsigned long long maximumHash  = 0;
  const auto         communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(&localHash, &minimumHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, communicator);
  tarch::mpi::allreduce(&localHash, &maximumHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, communicator);
  return minimumHash == maximumHash;
}

/**
 * Returns whether a topology change has been realised since the last time step.
 *
 * True while the first time step after a realised topology change runs, so the face
 * roll-over compensates the Updated entries the mesh-update sweeps cleared.
 */
bool exahype2::RefinementControlService::hasRealisedTopologyChangesSinceLastTimeStep() const {
  return _topologyChangedSinceLastTimeStep;
}

/**
 * Returns whether the criterion traversal that finished last collected nothing because the
 * mesh had changed.
 *
 * A caller that keeps updating the mesh until the criterion asks for nothing has to
 * treat such a traversal as undecided.
 */
bool exahype2::RefinementControlService::lastCriterionStepWasSuppressed() const {
  return _lastCriterionStepWasSuppressed;
}

/**
 * Sets the anchor and extent of the cell lattice.
 *
 * A request names the cell it was raised for by level and lattice position, and both
 * follow from the cell's centre and width against this domain. The service therefore
 * needs it before the first traversal evaluates a refinement criterion.
 */
void exahype2::RefinementControlService::setDomain(
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainWidth
) {
  _domainOffset = domainOffset;
  _domainWidth  = domainWidth;
}

/**
 * Sets the number of solvers whose consent a coarsening requires.
 *
 * A registration that never submits requests must not be counted: It can never
 * consent and would veto every coarsening.
 */
void exahype2::RefinementControlService::setNumberOfSolvers(const int numberOfSolvers) {
  assertion(numberOfSolvers >= 1);
  _numberOfSolvers    = numberOfSolvers;
  _lastOutcomePerKind = {};
}
