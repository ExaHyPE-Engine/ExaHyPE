// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Rank.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Diagnostics.h"

namespace {
  std::atomic<int> errorCodeOfAbort{EXIT_FAILURE};
} // namespace

int tarch::mpi::Rank::_maximumTagValue = -1;
int tarch::mpi::Rank::_nextFreeTag     = 0;

tarch::mpi::Rank tarch::mpi::Rank::_singleton;

/**
 * Reserves numberOfTags consecutive message tags and returns the first one. A reservation during
 * static initialisation, before init() reads the upper tag bound, skips the bound check.
 */
int
tarch::mpi::Rank::reserveFreeTag(const std::string& fullQualifiedMessageName, const int numberOfTags) {
  assertion(numberOfTags >= 1, fullQualifiedMessageName, numberOfTags);
  const int firstReservedTag = _nextFreeTag;
  _nextFreeTag += numberOfTags;

  validateReservedTagsAreSupported();

  return firstReservedTag;
}

bool
tarch::mpi::Rank::isInitialised() const {
  return _initIsCalled;
}

/**
 * Finalises MPI. A caller calls it before main() returns. Scalasca reports an error for an MPI
 * shutdown from a static destructor.
 */
void
tarch::mpi::Rank::shutdown() {
  assertion(_rank != -1);

  const int errorCode = MPI_Finalize();
  if (errorCode != MPI_SUCCESS) {
    logError("The MPI library cannot shut down: " << tarch::mpi::mpiReturnValueToString(errorCode));
  }

  _rank         = -1;
  _initIsCalled = false;
}

int
tarch::mpi::Rank::getGlobalMasterRank() {
  return 0;
}

bool
tarch::mpi::Rank::isGlobalMaster() const {
  assertion(_initIsCalled);
  return getRank() == getGlobalMasterRank();
}

/**
 * Returns whether the highest reserved tag fits the upper bound that init() reads from MPI. Writes
 * a warning when it does not. The check passes before init() runs.
 */
bool
tarch::mpi::Rank::validateReservedTagsAreSupported() {
  const int highestReservedTag = _nextFreeTag - 1;
  if (_maximumTagValue < 0 or highestReservedTag <= _maximumTagValue) {
    return true;
  }

  logWarning(
    "The MPI library provides message tags up to " << _maximumTagValue << " only. The reserved tags reach "
                                                   << highestReservedTag << "."
  );
  return false;
}

/**
 * Initialises MPI and returns whether it succeeded. MPI removes its own arguments from argc and argv.
 */
bool
tarch::mpi::Rank::init(int* const argc, char*** const argv) {
  int mpiResult = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &_providedThreadLevelSupport);
  if (mpiResult == MPI_SUCCESS and _providedThreadLevelSupport != MPI_THREAD_MULTIPLE) {
    std::cerr << "The MPI library must support calls from several threads at once." << std::endl;
    MPI_Abort(MPI_COMM_WORLD, -1);
    return false;
  }

  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "The MPI library cannot start: " + tarch::mpi::mpiReturnValueToString(mpiResult) << std::endl;
    return false;
  }

  mpiResult = MPI_Comm_size(MPI_COMM_WORLD, &_numberOfRanks);
  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "The MPI library cannot count the ranks: " + tarch::mpi::mpiReturnValueToString(mpiResult)
              << std::endl;
    return false;
  }

  mpiResult = MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "The MPI library cannot report the rank of this process: "
        + tarch::mpi::mpiReturnValueToString(mpiResult)
              << std::endl;
    return false;
  }

  // MPI_Comm_get_attr() returns a pointer to the upper tag bound.
  int   attributeFound{0};
  void* rawMaximumTag{nullptr};
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, static_cast<void*>(&rawMaximumTag), &attributeFound);
  if (attributeFound != 0) {
    _maximumTagValue = *static_cast<int*>(rawMaximumTag);
  } else {
    std::cerr << "The MPI library cannot report its largest message tag." << std::endl;
    return false;
  }

  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  // The split groups the ranks of one node. The lowest rank of a node is its master and the
  // masters count the nodes.
  MPI_Comm nodeCommunicator{MPI_COMM_NULL};
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, _rank, MPI_INFO_NULL, &nodeCommunicator);
  int nodeRank{0};
  MPI_Comm_rank(nodeCommunicator, &nodeRank);
  MPI_Comm_size(nodeCommunicator, &_numberOfRanksOnNode);
  MPI_Comm_free(&nodeCommunicator);
  _rankOnNode              = nodeRank;
  _isNodeMaster            = (nodeRank == 0);
  const int nodeMasterFlag = _isNodeMaster ? 1 : 0;
  MPI_Allreduce(&nodeMasterFlag, &_numberOfNodes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  _initIsCalled = true;
  return _initIsCalled;
}

int
tarch::mpi::Rank::getNumberOfNodes() const {
  return _numberOfNodes;
}

int
tarch::mpi::Rank::getNumberOfRanksOnNode() const {
  return _numberOfRanksOnNode;
}

int
tarch::mpi::Rank::getRankOnNode() const {
  return _rankOnNode;
}

/**
 * Returns whether this rank is the lowest rank on its node.
 */
bool
tarch::mpi::Rank::isNodeMaster() const {
  return _isNodeMaster;
}

int
tarch::mpi::Rank::getRank() const {
  return _rank;
}

int
tarch::mpi::Rank::getNumberOfRanks() const {
  assertion(_initIsCalled);
  return _numberOfRanks;
}

tarch::mpi::Rank&
tarch::mpi::Rank::getInstance() {
  return _singleton;
}

MPI_Comm
tarch::mpi::Rank::getCommunicator() const {
  assertion(_initIsCalled);
  return _communicator;
}

/**
 * Stops every rank of the job from any rank. The MPI library may end this rank through exit(). An
 * exit handler then ends the rank before a destructor of a static object fails again and waits
 * for this call forever.
 */
void
tarch::mpi::abort(const int errorCode) {
  std::cout.flush();
  std::cerr.flush();
  errorCodeOfAbort.store(errorCode);
  if (std::atexit(tarch::mpi::endRankWithErrorCodeOfAbort) != 0) {
    std::_Exit(errorCode);
  }
  int mpiInitialised{0};
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised != 0) {
    int mpiFinalised{0};
    MPI_Finalized(&mpiFinalised);
    if (mpiFinalised == 0) {
      MPI_Abort(MPI_COMM_WORLD, errorCode);
    }
  }
  std::_Exit(errorCode);
}

/**
 * Ends this rank through std::_Exit() with the error code of abort(). A rank without a call of
 * abort() ends with EXIT_FAILURE. The process ends before the destructors of static objects run.
 */
void
tarch::mpi::endRankWithErrorCodeOfAbort() {
  std::cout.flush();
  std::cerr.flush();
  std::_Exit(errorCodeOfAbort.load());
}
