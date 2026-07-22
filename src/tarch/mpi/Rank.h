// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/mpi.h"

namespace tarch {
  namespace mpi {
    void abort(int errorCode);
    class Rank;
  } // namespace mpi
} // namespace tarch

/**
 * Represents one program instance within a cluster, i.e. one MPI rank. This
 * class is a singleton.
 *
 * A rank owns the MPI environment: init() brings it up and determines the
 * rank id, the number of ranks, the node layout and the supported thread
 * level, while shutdown() tears it down again. The class furthermore hands
 * out the message tags through reserveFreeTag() and supervises the message
 * exchange, so a rank which waits too long for a message first writes a
 * timeout warning and eventually terminates the whole application.
 *
 * Every MPI error is fatal, i.e. the whole parallel application is shut down
 * as soon as one occurs.
 *
 * @author Tobias Weinzierl
 */
class tarch::mpi::Rank {
public:
  static const int DeadlockExitCode = -2;

private:
  static Rank _singleton;

  /**
   * Is set true if init() is called.
   */
  bool _initIsCalled;

  /**
   * Rank (id) of this process.
   */
  int _rank;

  /**
   * Number of processors available.
   */
  int _numberOfProcessors;

  int _providedThreadLevelSupport;

  // Node layout, computed once in init() so later queries need no collective.
  int  _numberOfNodes;
  bool _isNodeMaster;

#ifdef PEANO_WITH_MPI
  /**
   * MPI Communicator this process belongs to.
   */
  MPI_Comm _communicator;
#endif

  /**
   * Timeout warning
   *
   * How long shall the application wait until it writes a time-out warning.
   * Different to _deadlockTimeout, this value changes over time: every
   * warning message increases it. It can hence start off rather small, and
   * the code widens it with each warning so the terminal is not flooded. The
   * growth never pushes it beyond _deadlockTimeout though.
   *
   * Despite the growth discussion, you can use a timeout warning that is
   * greater than zero and set _deadlockTimeout to zero. In this case, you
   * will get warnings that the code thinks it runs into a deadlock, but you
   * do not get a timeout ever.
   *
   * @see writeTimeoutWarning()
   * @see _deadlockTimeout
   */
  std::chrono::duration<double> _timeoutWarning;

  /**
   * Time to timeout
   *
   * This counter specifies the seconds until a timeout is triggered. If the
   * field equals zero, no timeouts will ever happen.
   */
  std::chrono::duration<double> _deadlockTimeout;

  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutWarning;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutDeadlock;

  /**
   * Global toggle to enable/disable timeouts
   *
   * This flag helps user codes to enable/disable timeouts (temporarily).
   * Setting _deadlockTimeout to zero disables them as well.
   */
  bool _areTimeoutsEnabled;

  /**
   * Enables detailed request tracking for MPI deadlock diagnostics.
   *
   * Timeout warnings and timeout termination work without this flag. When this
   * flag is enabled, tarch::mpi additionally records the origin, partner and
   * tag of nonblocking MPI requests, which is useful for diagnostics but adds
   * bookkeeping overhead to every registered request.
   */
  bool _deadlockDetectionEnabled;

  /**
   * Set by init() and stores the number of valid tags.
   */
  static int _maxTags;

  /**
   * Counts the tags that have already been handed out.
   */
  static int _tagCounter;

  /**
   * The standard constructor assigns the attributes their default values.
   * Without PEANO_WITH_MPI, the rank is 0 and there is one processor. With
   * MPI, both values stay invalid until init() has determined them.
   */
  Rank();

  /**
   * You may not copy a singleton.
   */
  Rank(const Rank& node) = delete;

  /**
   * Receive any message pending in the MPI receive buffers
   *
   * We poll MPI only every k iterations. Once we have found a message, we
   * wait k iterations again. If the threshold is exceeded and no message is
   * found, we do not reset k: this routine is typically called when we enter
   * a critical phase in which we urgently wait for incoming MPI messages, so
   * throttling the polling down again right there would be
   * counterproductive. We reset the counter only when data drops in, as we
   * may then assume that we are about to leave the critical phase.
   */
  void receiveDanglingMessagesFromReceiveBuffers();

public:
  /**
   * Find out whether a tag is supported. This works after init() has been
   * called; before that, the check always passes. If the tag is not
   * supported, we write a warning and return false.
   */
  static bool validateMaxTagIsSupported();

  /**
   * Return a free tag
   *
   * Returns a free tag to be used for a new datatype. Each result is
   * delivered exactly once. The string argument serves logging purposes.
   *
   * ## Implementation details
   *
   * The routine reports through std::cout rather than through the log
   * device. It is static, and so are the class' log devices, while C++ has
   * no mechanism to define which static entity is instantiated first. On
   * some systems the tag registration therefore runs before the logging
   * device is up. The underlying issue is known as the static
   * initialisation order problem:
   *
   *     https://isocpp.org/wiki/faq/ctors#static-init-order
   */
  static int  reserveFreeTag(const std::string& fullQualifiedMessageName, int numberOfTags = 1);
  static void releaseTag(int tag);

  /**
   * Check whether a message is pending on a tag
   *
   * The check covers the tag alone. MPI matches on the number of bytes, so a
   * caller has to know which type drops in once there is a message on a tag.
   */
  bool isMessageInQueue(int tag) const;

  /**
   * Logs the status of the process onto the log device.
   */
  void logStatus() const;

  /**
   * This operation returns the singleton instance. Before using this
   * instance, one has to call the init() operation on the instance returned.
   *
   * @return The singleton instance
   */
  static Rank& getInstance();

  /**
   * The destructor leaves the MPI environment untouched. shutdown() is the
   * operation which calls MPI_Finalize().
   */
  virtual ~Rank();

  /**
   * This operation initialises the MPI environment and the program instance.
   * The argv and argc parameters are both in and out parameters: the MPI
   * environment sets them, and afterwards they hold the original parameters
   * the user passed in.
   *
   * ## Implementation details
   *
   * init never uses the log device to report any errors, as the log device
   * in turn uses this class' getters. The _initIsCalled flag has to be set
   * before the log state operation is invoked for the same reason.
   *
   * ## Upper tag bound
   *
   * We determine how many tags the system supports. The MPI standard hands
   * this limit out through a pointer to a void pointer, from which we then
   * read the value, i.e. we get a pointer to a system variable back rather
   * than the system variable itself.
   *
   * @return true if initialisation has been successful
   *
   * @see shutdown
   */
  bool init(int* argc, char*** argv);

  /**
   * Shuts down the application. Should be the last operation called by the
   * overall application.
   *
   * ## Rationale
   *
   * The MPI shutdown is an explicit call before the final return statement
   * of main, not part of the destructor. Compilers differ in when they
   * destroy static objects: some do so before main returns, others after the
   * return statement, and in the latter case a destructor-based shutdown
   * makes Scalasca's instrumentation report an error.
   */
  void shutdown();

  /**
   * Return rank of this node. In the serial version, i.e. without MPI, this
   * operation always returns 0.
   *
   * @return Rank of this node
   */
  int getRank() const;

  /**
   * Get the global master
   *
   * Peano sets up a logical tree topology on all the ranks. The root of this
   * logical tree, i.e. the rank which is responsible for all other ranks, is
   * the global master. Every rank in addition has a local master which tells
   * it what to do, namely its parent within the topology tree.
   *
   * @return 0
   */
  static int getGlobalMasterRank();

#ifdef PEANO_WITH_MPI
  /**
   * @return Communicator of Peano
   */
  MPI_Comm getCommunicator() const;
#endif

  inline int getNumberOfRanks() const {
#ifdef PEANO_WITH_MPI
    assertion(_initIsCalled);
#endif
    return _numberOfProcessors;
  }

  /** Number of distinct shared-memory nodes the ranks run on (1 without MPI). */
  int getNumberOfNodes() const;

  /** Whether this rank is the lowest-ranked process on its node; use it to count per-node data once. */
  bool isNodeMaster() const;

  /**
   * Is this node the global master process, i.e. does its rank equal
   * getGlobalMasterRank()? This operation always returns true if the code is
   * not compiled with PEANO_WITH_MPI.
   *
   * @return Is this node the global master process?
   */
  bool isGlobalMaster() const;

  /**
   * Triggers a time out and shuts down the cluster if a timeout is violated
   *
   * The routine evaluates exceededDeadlockThreshold() itself and returns
   * without any effect as long as the threshold has not been passed. Once it
   * has, the routine dumps the pending message queues and then terminates
   * the application with DeadlockExitCode. The termination goes through
   * abort(), so every rank goes down.
   *
   * @param className  Name of the class that triggers the deadlock shutdown.
   * @param methodName Name of the method that triggers the deadlock shutdown.
   * @param communicationPartnerRank Rank of the node the operation that
   *          should have sent a message but did not.
   */
  void triggerDeadlockTimeout(
    const std::string& className,
    const std::string& methodName,
    int                communicationPartnerRank,
    int                tag,
    int                numberOfExpectedMessages = 1,
    const std::string& comment                  = ""
  );

  /**
   * Writes a warning if relevant
   *
   * This operation writes a warning if the code might assume that it runs
   * into a timeout (or excessive wait). The routine assumes that timeouts
   * are enabled and that the user has called
   *
   * setDeadlockWarningTimeStamp()
   *
   * before. To avoid an excessive list of timeout warnings, a warning is
   * always followed by moving the next warning timestamp forward, and the
   * code also increases the time span it uses from hereon to report on
   * timeouts.
   */
  void writeTimeoutWarning(
    const std::string& className,
    const std::string& methodName,
    int                communicationPartnerRank,
    int                tag,
    int                numberOfExpectedMessages = 1
  );

  bool exceededTimeoutWarningThreshold() const;

  bool exceededDeadlockThreshold() const;

  void plotMessageQueues();

  /**
   * Ensure that there are no messages anymore from the specified rank.
   */
  void ensureThatMessageQueuesAreEmpty(int fromRank, int tag);

  /**
   * Memorise the time stamp from which on a timeout warning is written if no
   * message has been received meanwhile.
   */
  void setDeadlockWarningTimeStamp();

  /**
   * Memorise the time stamp from which on the application terminates because
   * of a timeout if no message has been received meanwhile.
   */
  void setDeadlockTimeoutTimeStamp();

  bool isInitialised() const;

  /**
   * Set time out warning
   *
   * Set after how much time a node waiting for an MPI message shall write a
   * warning that it is likely that it ran into a deadlock. If you pass 0,
   * that switches off this feature.
   */
  void setTimeoutWarning(double valueInSeconds);

  /**
   * Set deadlock time out
   *
   * Set after how much time a node waiting for an MPI message shall quit
   * and shutdown the whole application with an error report. If you pass 0,
   * that switches off this feature.
   *
   * By default, timeouts are disabled when we instantiate the Rank object. A
   * caller that sets the parallel environment up for us installs a default
   * timeout in the process.
   *
   * @param valueInSeconds Timeout duration in seconds, i.e. how long we wait
   *   at most for an incoming message.
   */
  void setDeadlockTimeout(double valueInSeconds);

  void setDeadlockDetection(bool enabled);

  void suspendTimeouts(bool timeoutsDisabled);

  bool areTimeoutsEnabled() const;

  bool isDeadlockDetectionEnabled() const;

  std::chrono::duration<double> getTimeoutWarning() const;

  std::chrono::duration<double> getDeadlockTimeout() const;

  /**
   * A proper abort in an active MPI context has to use MPI_Abort. Otherwise, only
   * the current rank goes down. If MPI is unavailable or already finalised, we
   * use std::_Exit() to avoid C++ runtime cleanup and core dump generation.
   */
  static void abort(int errorCode);

  /**
   * Information on supported thread-level support
   *
   * An MPI version without support for MPI_THREAD_MULTIPLE yields a
   * prominent complaint, but execution continues: the code stores the
   * thread support level, which can be queried here at any time.
   */
  int getProvidedThreadLevelSupport() const;
};
