// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MPI_RANK_H_
#define _TARCH_MPI_RANK_H_


#include "tarch/logging/Log.h"

#include <ctime>
#include <functional>


#include "mpi.h"



namespace tarch {
  namespace mpi {
    class Rank;

    #ifdef Parallel
    /**
     * Returns a string representation of the mpi status. For a detailed
     * description of the mpi status itself see the file mpi.h.
     */
    std::string MPIStatusToString( const MPI_Status& status );

    std::string MPIReturnValueToString( int result );
    #endif
  }
}

/**
 * Represents a program instance within a cluster. Thus, this class is a
 * singleton.
 *
 * The parallel concept is a client - server model (process 0 is the server),
 * where all active nodes act as servers deploying calculations on demand. So
 * the basic activities of a parallel node are
 *
 * - receive new root element to work on
 * - pass back space-tree
 * - perform additive cycle
 * - perform multiplicative cycle
 *
 * The two perform commands have to return several statistic records. Among them
 * are time needed, some residual characteristics and flags defining if the
 * domain has been refined. Furthermore the number of communication partners is
 * an interesting detail.
 *
 * In the near future, this class should become responsible for the error
 * handling. Right now, the error handle set is the fatal error handler. That
 * is the whole parallel application is shut down as soon as an mpi error
 * occurs.
 *
 * @author Tobias Weinzierl
 * @version $Revision: 1.51 $
 */
class tarch::mpi::Rank {
  public:
    static const int DEADLOCK_EXIT_CODE = -2;

  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    static Rank  _singleton;

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

    #ifdef Parallel
    /**
     * MPI Communicator this process belongs to.
     */
    MPI_Comm _communicator;
    #endif

    /**
     * How long shall the application wait until it writes a time-out warning
     */
    std::chrono::seconds _timeOutWarning;

    std::chrono::seconds _deadlockTimeOut;

    bool    _areTimeoutsEnabled;

    /**
     * The standard constructor assignes the attributes default values and
     * checks whether the program is compiled using the -DParallel option.
     * If this is not the case, a warning is logged.
     */
    Rank();

    /**
     * You may not copy a singleton.
     */
    Rank( const Rank& node ) = delete;

    /**
     * Receive any Message Pending in the MPI/Receive Buffers
     *
     * We do poll MPI only every k iterations. Once we have found a message, we 
     * wait again k iterations. If the threshold is exceeded and no message is 
     * found we however do not reset k (I've tried this and it gives worse 
     * runtime): receiveDanglingMessages is typically called when we enter a 
     * critical phase of the simulation and we urgently wait for incoming MPI
     * messages. So once we are in that critical regime and have already 
     * exceeded the k it would be harakiri to reset this one to 0 again - we 
     * only do so if data drops in and we thus may assume that we'll leave the 
     * critical phase anyway.
     */
    void receiveDanglingMessagesFromReceiveBuffers();

  public:
    /**
     * Return a Free Tag
     *
     * Returns a free tag to be used for a new datatype. Each result is
     * delivered exactly once. The string argument is just for logging.
     *
     *
     * \section  Implementation details
     *
     * This operation should write something to the log devices. However, it
     * is static and the class' log devices are static, too. C++ has no
     * mechanism to define which static entity has to be instantiated first.
     * On some systems, it hence happened that this tag registration got
     * called before the logging device had been up and running. The problem
     * is known as static initialization order problem:
     *
     *     	https://isocpp.org/wiki/faq/ctors#static-init-order
     *
     * So what I do is that I log to std::cout only. This eliminated problems
     * here.
     */
    static int reserveFreeTag(const std::string& fullQualifiedMessageName, int numberOfTags = 1);
    static void releaseTag(int tag);

    /**
     * Global MPI barrier
     *
     * I provide a custom barrier. It semantically differs from a native
     * MPI_Barrier as MPI barriers would not be able to do anything while
     * they wait. I therefore make this barrier rely on MPI's non-blocking
     * barrier and give the user the opportunity to tell me what do while
     * I wait to be allowed to pass the barrier.
     *
     * The most common pattern how to use the barrier in Peano 4 is to pass
     * the following functor to the barrier as argument:
     *
     * <pre>

[&]() -> void {
  tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
}

      </pre>
     *
     * Please note that this barrier remains an MPI barrier. It does not act
     * as barrier between multiple threads. In particular: if you use this
     * barrer in a multithreaded code, then each thread will launch a barrier
     * on its own. If the number of threads/tasks per rank differs, deadlocks
     * might arise. Anyway, it is not a good idea to use this within a
     * multithreaded part of your code.
     *
     *
     * @param waitor is my functor that should be called while we wait. By
     *   default, it is empty, i.e. barrier degenerates to a blocking barrier
     *   in the MPI 1.3 sense.
     */
    void barrier(std::function<void()> waitor = []() -> void {} );

    /**
     * In older DaStGen version, I tried to find out whether a particular
     * message type is in the MPI queue. That is, I looked whether a message
     * on this tag does exist, and then I looked whether the memory footprint
     * matches via count. I think this is invalid. MPI really looks only into
     * the number of bytes, so you have to know which type drops in once there
     * is a message on a tag.
     */
    bool isMessageInQueue(int tag) const;

    /**
     * Logs the status of the process onto the log device.
     *
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
     * The standard destructor calls MPI_Finalize().
     */
    virtual ~Rank();

    /**
     * This operation initializes the MPI environment and the program instance.
     * Not that the argv and argc parameters are both in and out parameters.
     * Before you pass them to the operation, they are set by the mpi
     * environment. Afterwards the original parameters from the user are stored
     * within them.
     *
     * !! Implementation details
     *
     * init never uses the log device to report any errors as the log device
     * usually in turn uses Node's getters. Furthermore, the _initIsCalled flag
     * has thus to be set before the log state operation is invoked.
     *
     * @return true if initialisation has been successful
     */
    bool init(int* argc, char*** argv);

    /**
     * Shuts down the application. Should be the last operation called by the
     * overall application.
     *
     * !!! Rationale
     *
     * Originally, I put the shutdown operation into the destructor of Node.
     * The MPI environment consequently was shut down as soon as the operating
     * system terminates the application. However, Scalasca complained on the
     * BlueGene/P that the
     * destruction happened after the return statement of the main method. To
     * make Peano work with Scalasca, I hence moved the MPI shutdown into a
     * method called explicitely before the final return statement.
     *
     * This seems to be a CLX effect, i.e. the Intel and the GNU compilers
     * worked fine with Scalasca. Hence, I assume that Intel and GNU
     * executables destroy all static objects (singletons) before they return
     * from the main functions. CLX destroys the static objects after the
     * return statement and thus makes Scalasca's instrumentation report an
     * error.
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
     * logical tree, i.e. the rank that is reponsible for all other ranks, is
     * the global master. In contrast, every rank also has a local master that
     * tells him what to do. This is the master and the parent within the
     * topology tree. Use the NodePool to identify the rank of a rank's master.
     *
     * @return 0
     */
    static int getGlobalMasterRank();

    #ifdef Parallel
    /**
     * @return Communicator of Peano
     */
    MPI_Comm getCommunicator() const;
    #endif

    /**
     * @return Number of Nodes Available
     */
    int getNumberOfRanks() const;

    /**
     * Is this node the global master process, i.e. does its rank equal get
     * MasterProcessRank()? This operation returns always true if the code
     * is not compiled with -DParallel.
     *
     * @return Is this node the global master process?
     */
    bool isGlobalMaster() const;

    /**
     * Triggers a time out and shuts down the cluster:
     *
     * The implementation does not use MPI_Abort, since it seems that this
     * operation requires all nodes running. Instead of,
     * getDeadlockWarningTimeStamp() uses the system exit function passing it
     * DEADLOCK_EXIT_CODE as exit code.
     *
     * The operation should be called only if the deadlock time-out is switched
     * on ( isTimeOutDeadlockEnabled() ) and the deadlock time-out has expired.
     * Use getDeadlockWarningTimeStamp() and the system operation clock() to
     * check the second requirement.
     *
     * @param className  Name of the class that triggers the deadlock shutdown.
     * @param methodName Name of the method that triggers the deadlock shutdown.
     * @param communicationPartnerRank Rank of the node the operation that
     *          should have sent a message but did not.
     */
    void triggerDeadlockTimeOut(
      const std::string&  className,
      const std::string&  methodName,
      int                 communicationPartnerRank,
      int                 tag,
      int                 numberOfExpectedMessages = 1,
      const std::string&  comment = ""
    );

    void writeTimeOutWarning(
      const std::string&  className,
      const std::string&  methodName,
      int                 communicationPartnerRank,
      int                 tag,
      int                 numberOfExpectedMessages = 1
    );

    void plotMessageQueues();

    /**
     * Ensure that there are no messages anymore from the specified rank.
     */
    void ensureThatMessageQueuesAreEmpty( int fromRank, int tag );

    /**
     * If you want to make any receive operation to be able to recognize a
     * time-out, you have to insert the following code snipped before your
     * receive call.
     *
     * \code
     *   MPI_Status  status;
     *   int         flag = 0;
     *   clock_t     timeOutWarning   = Rank::getInstance().getDeadlockWarningTimeStamp();
     *   clock_t     timeOutShutdown  = Rank::getInstance().getDeadlockTimeOutTimeStamp();
     *   bool        triggeredTimeoutWarning = false;
     *   MPI_Iprobe( source, CommunicationTag, Rank::getInstance().getCommunicator(), &flag, &status );
     *
     *   while (!flag) {
     *     if ( Rank::getInstance().isTimeOutWarningEnabled() && (std::chrono::system_clock::now()>timeOutWarning) && (!triggeredTimeoutWarning)) {
     *       Rank::getInstance().writeTimeOutWarning( "parallel::SendReceiveBuffer", "receiveAllMessages()", source );
     *       triggeredTimeoutWarning = true;
     *     }
     *     if ( Rank::getInstance().isTimeOutDeadlockEnabled() && (std::chrono::system_clock::now()>timeOutShutdown)) {
     *       Rank::getInstance().triggerDeadlockTimeOut( "parallel::SendReceiveBuffer", "receiveAllMessages()", source );
     *     }
     *     MPI_Iprobe( source, CommunicationTag, Rank::getInstance().getCommunicator(), &flag, &status );
     *   }
     * \endcode
     *
     * @return Time stamp when next warning should be written if no message has
     *         been received meanwhile.
     */
    std::chrono::system_clock::time_point getDeadlockWarningTimeStamp() const;

    /**
     * @return Time stamp when next application should terminate because of a
     *         time out if no message has been received meanwhile.
     */
    std::chrono::system_clock::time_point getDeadlockTimeOutTimeStamp() const;

    bool isTimeOutDeadlockEnabled() const;
    bool isTimeOutWarningEnabled() const;

    bool isInitialised() const;

    /**
     * Set time out warning
     *
     * Set after how much time a node waiting for an MPI message shall write a
     * warning that it is likely that it ran into a deadlock. If you pass 0,
     * that switches off this feature.
     */
    void setTimeOutWarning( int valueInSeconds );

    /**
     * Set deadlock time out
     *
     * Set after how much time a node waiting for an MPI message shall quit
     * and shutdown the whole application with an error report. If you pass 0,
     * that switches off this feature.
     */
    void setDeadlockTimeOut( int valueInSeconds );

    #ifdef Parallel
    /**
     * Set communicator to be used by Peano
     */
    void setCommunicator( MPI_Comm communicator );
    #endif

    void suspendTimeouts( bool timeoutsDisabled );

    /**
     * A proper abort in an MPI context has to use MPI_Abort. Otherwise, only the
     * current rank goes down.
     */
    static void abort(int errorCode);
};

#endif
