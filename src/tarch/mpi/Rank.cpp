// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Rank.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Diagnostics.h"

int tarch::mpi::Rank::_maximumTagValue = -1;
int tarch::mpi::Rank::_nextFreeTag     = 0;

tarch::mpi::Rank tarch::mpi::Rank::_singleton;

/**
 * Reserves numberOfTags consecutive message tags and returns the first one. A caller reserves tags
 * during static initialisation, so a reservation writes a log line only after init() has read the
 * upper tag bound.
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
 * Shuts the MPI environment down and runs before the final return statement of
 * main. Compilers differ in when they destroy a static object, so a shutdown from
 * a destructor can run after main returns, where Scalasca's instrumentation
 * reports an error.
 */
void
tarch::mpi::Rank::shutdown() {
  assertion(_rank != -1);

  const int errorCode = MPI_Finalize();
  if (errorCode != MPI_SUCCESS) {
    logError(tarch::mpi::mpiReturnValueToString(errorCode));
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
 * Returns whether the highest reserved tag fits the upper bound MPI reports, and
 * writes a warning when it does not. init() reads that bound, so the check passes
 * before init() runs.
 */
bool
tarch::mpi::Rank::validateReservedTagsAreSupported() {
  const int highestReservedTag = _nextFreeTag - 1;
  if (_maximumTagValue < 0 or highestReservedTag <= _maximumTagValue) {
    return true;
  }

  logWarning(
    "The maximum MPI tag value is " << _maximumTagValue << " but tag " << highestReservedTag << " has been reserved."
  );
  return false;
}

/**
 * Initialises the MPI environment and the program instance. argc and argv are in
 * and out parameters, because an MPI implementation removes its own arguments
 * before it returns.
 *
 * @return Whether initialisation succeeded.
 *
 * @see shutdown
 */
bool
tarch::mpi::Rank::init(int* const argc, char*** const argv) {
  int mpiResult = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &_providedThreadLevelSupport);
  if (mpiResult == MPI_SUCCESS and _providedThreadLevelSupport != MPI_THREAD_MULTIPLE) {
    std::cerr << "Error: MPI implementation does not support MPI_THREAD_MULTIPLE. The supported multithreading level "
                 "is "
              << _providedThreadLevelSupport << ", the required level is " << MPI_THREAD_MULTIPLE << "." << std::endl;
    std::cerr << "The application requires MPI_THREAD_MULTIPLE. Aborting." << std::endl;
    MPI_Abort(MPI_COMM_WORLD, -1);
    return false;
  }

  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + tarch::mpi::mpiReturnValueToString(mpiResult)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  mpiResult = MPI_Comm_size(MPI_COMM_WORLD, &_numberOfRanks);
  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + tarch::mpi::mpiReturnValueToString(mpiResult)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  mpiResult = MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
  if (mpiResult != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + tarch::mpi::mpiReturnValueToString(mpiResult)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  // MPI_Comm_get_attr hands the upper tag bound out through a pointer to a void
  // pointer, so the value is read from the pointer that comes back.
  int   attributeFound{0};
  void* rawMaximumTag{nullptr};
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, static_cast<void*>(&rawMaximumTag), &attributeFound);
  if (attributeFound != 0) {
    _maximumTagValue = *static_cast<int*>(rawMaximumTag);
  } else {
    std::cerr << "init(int*,char***)\t Could not query the maximum tag value" << std::endl;
    return false;
  }

  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  // Node layout: The split groups the ranks that address the same memory, the lowest rank of a group
  // is the master of its node, and the number of masters is the number of nodes.
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

/**
 * @return Number of distinct compute nodes the ranks run on.
 */
int
tarch::mpi::Rank::getNumberOfNodes() const {
  return _numberOfNodes;
}

/**
 * @return Number of ranks on the node of this rank, this rank included.
 */
int
tarch::mpi::Rank::getNumberOfRanksOnNode() const {
  return _numberOfRanksOnNode;
}

/**
 * @return Rank of this process among the ranks of its node, counted from zero.
 */
int
tarch::mpi::Rank::getRankOnNode() const {
  return _rankOnNode;
}

/**
 * @return Whether this rank is the lowest-ranked process on its node, which
 *   lets a caller count per-node data exactly once.
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

/**
 * Returns the singleton instance. init() establishes its MPI state.
 */
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
 * Stops every rank of the job from any rank. MPI_Abort reaches every rank, and
 * std::_Exit() stops this rank while MPI is not initialised.
 */
void
tarch::mpi::abort(const int errorCode) {
  std::cout.flush();
  std::cerr.flush();
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
