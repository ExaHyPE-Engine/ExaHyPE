// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Rank.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/multicore/multicore.h"

int tarch::mpi::Rank::_maxTags    = -1;
int tarch::mpi::Rank::_tagCounter = 0;

tarch::mpi::Rank tarch::mpi::Rank::_singleton;

/**
 * Return a free tag
 *
 * Returns a free tag to be used for a new message type. Each result is
 * delivered exactly once, and the string argument serves logging purposes.
 *
 * ## Implementation details
 *
 * Many tags are booked before an application has set its log filters up, and
 * the routine is static just like the class' log devices, while C++ has no
 * mechanism to define which static entity is instantiated first. Reserving a
 * tag therefore must not depend on the log device being up. The underlying
 * issue is known as the static initialisation order problem:
 *
 *     https://isocpp.org/wiki/faq/ctors#static-init-order
 */
int tarch::mpi::Rank::reserveFreeTag(
  [[maybe_unused]] const std::string& fullQualifiedMessageName,
  [[maybe_unused]] int                numberOfTags
) {
  assertion(numberOfTags >= 1, fullQualifiedMessageName, numberOfTags);
  const int result = _tagCounter;
  _tagCounter += numberOfTags;

  validateMaxTagIsSupported();

  return result;
}

bool tarch::mpi::Rank::isInitialised() const { return _initIsCalled; }

/**
 * Shuts down the application. Should be the last operation called by the
 * overall application.
 *
 * ## Rationale
 *
 * The MPI shutdown is an explicit call before the final return statement of
 * main, not part of the destructor. Compilers differ in when they destroy
 * static objects: some do so before main returns, others after the return
 * statement, and in the latter case a destructor-based shutdown makes
 * Scalasca's instrumentation report an error.
 */
void tarch::mpi::Rank::shutdown() {
  assertion(_rank != -1);

  int errorCode = MPI_Finalize();
  if (errorCode) {
    logError(MPIReturnValueToString(errorCode));
  }

  _rank         = -1;
  _initIsCalled = false;
}

/**
 * Get the global master
 *
 * The ranks form a logical tree topology. Its root, i.e. the rank which is
 * responsible for all other ranks, is the global master.
 *
 * @return 0
 */
int tarch::mpi::Rank::getGlobalMasterRank() { return 0; }

bool tarch::mpi::Rank::isGlobalMaster() const {
  assertion(_initIsCalled);
  return getRank() == getGlobalMasterRank();
}

/**
 * Find out whether a tag is supported. This works after init() has been called;
 * before that, the check always passes. If the tag is not supported, we write a
 * warning and return false.
 */
bool tarch::mpi::Rank::validateMaxTagIsSupported() {
  if (_maxTags <= 0 or _tagCounter < _maxTags) {
    return true;
  } else {
    logWarning(
      "The maximum tag value is " << _maxTags << " though " << _tagCounter
                                  << " tags are needed, so the code is likely to crash."
    );
    return false;
  }
}

/**
 * An MPI version without support for MPI_THREAD_MULTIPLE yields a prominent
 * complaint, but execution continues: the code stores the thread support level,
 * which can be queried here at any time.
 */
int tarch::mpi::Rank::getProvidedThreadLevelSupport() const { return _providedThreadLevelSupport; }

/**
 * Initialises the MPI environment and the program instance. The argv and argc
 * parameters are both in and out parameters: the MPI environment sets them, and
 * afterwards they hold the original parameters that were passed in.
 *
 * ## Implementation details
 *
 * init never uses the log device to report any errors, as the log device in turn
 * uses this class' getters. The _initIsCalled flag has to be set before the log
 * state operation is invoked for the same reason.
 *
 * ## Upper tag bound
 *
 * We determine how many tags the system supports. The MPI standard hands this
 * limit out through a pointer to a void pointer, from which we then read the
 * value, i.e. we get a pointer to a system variable back rather than the system
 * variable itself.
 *
 * @return true if initialisation has been successful
 *
 * @see shutdown
 */
bool tarch::mpi::Rank::init([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv) {
  int result = MPI_SUCCESS;

#if defined(PEANO_WITH_MULTITHREADING)
  result = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &_providedThreadLevelSupport);
  if (_providedThreadLevelSupport != MPI_THREAD_MULTIPLE) {
    std::cerr << "Warning: the MPI implementation does not support MPI_THREAD_MULTIPLE. The supported "
                 "multithreading level is "
              << _providedThreadLevelSupport << " instead of " << MPI_THREAD_MULTIPLE
              << ". Disable MultipleThreadsMayTriggerMPICalls in the compiler-specific settings or "
                 "via -DnoMultipleThreadsMayTriggerMPICalls."
              << std::endl;
  }
#else
  result = MPI_Init(argc, argv);
#endif

  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + MPIReturnValueToString(result)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  result = MPI_Comm_size(MPI_COMM_WORLD, &_numberOfProcessors);
  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + MPIReturnValueToString(result)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  result = MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + MPIReturnValueToString(result)
        + " (no logging available yet)"
              << std::endl;
    return false;
  }

  int   answerFlag;
  void* rawMaxTag;
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &rawMaxTag, &answerFlag);
  if (answerFlag) {
    _maxTags = *(int*)rawMaxTag;
  } else {
    std::cerr << "init(int*,char***)\t Could not query the maximum tag value" << std::endl;
    return false;
  }

  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  // Node layout: split by shared memory; the lowest rank per node is its master, and the number of
  // masters is the number of nodes.
  MPI_Comm nodeComm = MPI_COMM_NULL;
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, _rank, MPI_INFO_NULL, &nodeComm);
  int nodeRank = 0;
  MPI_Comm_rank(nodeComm, &nodeRank);
  MPI_Comm_free(&nodeComm);
  _isNodeMaster            = (nodeRank == 0);
  const int nodeMasterFlag = _isNodeMaster ? 1 : 0;
  MPI_Allreduce(&nodeMasterFlag, &_numberOfNodes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  _initIsCalled = true;
  return _initIsCalled;
}

/**
 * @return Number of distinct shared-memory nodes the ranks run on.
 */
int tarch::mpi::Rank::getNumberOfNodes() const { return _numberOfNodes; }

/**
 * @return Whether this rank is the lowest-ranked process on its node, which
 *   lets a caller count per-node data exactly once.
 */
bool tarch::mpi::Rank::isNodeMaster() const { return _isNodeMaster; }

int tarch::mpi::Rank::getRank() const { return _rank; }

int tarch::mpi::Rank::getNumberOfRanks() const {
  assertion(_initIsCalled);
  return _numberOfProcessors;
}

/**
 * @return The singleton instance. Before using it, one has to call init() on it.
 */
tarch::mpi::Rank& tarch::mpi::Rank::getInstance() { return _singleton; }

MPI_Comm tarch::mpi::Rank::getCommunicator() const {
  assertion(_initIsCalled);
  return _communicator;
}

/**
 * Tears the application down from any rank. MPI_Abort reaches every rank of the
 * job, and the explicit exit covers the case where MPI is not up (yet).
 */
void tarch::mpi::abort([[maybe_unused]] int errorCode) {
  std::cout.flush();
  std::cerr.flush();
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised) {
    int mpiFinalised = 0;
    MPI_Finalized(&mpiFinalised);
    if (not mpiFinalised) {
      MPI_Abort(MPI_COMM_WORLD, errorCode);
    }
  }
  std::_Exit(errorCode);
}
