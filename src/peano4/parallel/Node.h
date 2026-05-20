// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "TreeEntry.h"
#include "peano4/grid/GridVertex.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <map>
#include <queue>
#include <set>

namespace peano4::parallel {
  class Node;

  namespace tests {
    class NodeTest;
  } // namespace tests
} // namespace peano4::parallel

/**
 * Node is Peano's abstraction for the hybrid of MPI/threads. It represents
 * one spacetree instance basically.
 *
 * @author Tobias Weinzierl
 */
class peano4::parallel::Node {
public:
#if !defined(WITH_MPI)
  typedef int MPI_Comm;
#ifndef MPI_COMM_NULL
#define MPI_COMM_NULL 0
#endif
#endif

  static constexpr int Terminate = -2;

  /**
   * Value for _currentProgramStep.
   */
  static constexpr int UndefProgramStep = -1;

  ~Node() = default;

  static Node& getInstance();

  static void initMPIDatatypes();
  static void shutdownMPIDatatypes();

  inline int getLocalTreeId(int treeId) const {
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    return treeId / numberOfRanks;
  }

  inline int getGlobalTreeId(int treeId) const {
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    return treeId * numberOfRanks + tarch::mpi::Rank::getInstance().getRank();
  }

  static bool isGlobalMaster(int treeId);

  int getNumberOfRegisteredTrees() const;

  /**
   * This operation is not const as it does some internal bookkeeping. It
   * internally invokes  registerId() on the result.
   *
   * @param forTreeId If we run a split, this is the rank that wants to
   *          split. In general, this always is the parent/master of the
   *          new rank. So if you move a rank, you have to ensure that
   *          the master is preserved.
   *
   * @return -1 If there are no ids left anymore. In principle, we do not
   *            limit the number of threads at all. However, we have to
   *            work with a limited number of tags in MPI, so we put a
   *            cap on the number of trees per node.
   * @see getId(int,int)
   */
  int reserveId(int rank, int forTreeId);

  /**
   * You hand in a tree number and the node tells you on which rank such a
   * tree is hosted.
   * @see getId(int,int)
   */
  inline int getRank(int treeId) const {
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    return treeId % numberOfRanks;
  }

  void deregisterId(int id);

  /**
   * You should call this operation only on the ranks >0 to find out whether
   * you should do more iteration/sweeps. The spacetree set internally
   * hijacks this operation also on rank 0 to trigger the send out of startup
   * messages. Therefore, the operation is not const.
   */
  bool continueToRun();

  /**
   * The user tells the set which program step to use, i.e. which adapter
   * to use for the next grid run or which step to run next. Should only
   * be called on global master.
   */
  void setNextProgramStep(int number);

  int getCurrentProgramStep() const;

  void init();

  /**
   * The shutdown is invoked by peano4::shutdownSingletons()
   *
   * If we run it on the global master, we set the program state to terminate
   * and distribute this message. For this, we hijack the routine continueToRun().
   * This is really a hack. We in particular have to be careful and may not
   * call the continue operation on the other ranks. They typically are stuck
   * in a continue while loop in their main routine and just wait for the
   * global master to send out this terminate command.
   *
   * It is important that this is the very first call when you want your
   * application to go down. Invoked on the global master, it triggers the
   * shutdown on all the other ranks. You will get deadlocks if you try to free
   * any common memory (RDMA) or free datatypes if you don't call the node's
   * shutdown first.
   *
   * Other ranks might struggle to receive the shutdown command as they might
   * still ask for further subtrees, e.g., or issue load balancing. Therefore,
   * I insert a barrier right after we've issued the shutdown. It makes
   * the rank that goes down handle all incoming data until we can be sure that
   * everybody is in shutdown mode.
   */
  void shutdown();

  void createPendingCommunicators();

  enum class ExchangeMode {
    HorizontalData,
    /**
     * Vertical data is data running from the master to the worker and the other
     * way round. Consequently, some tree management data are vertical data, too.
     * See for example peano4::parallel::SpacetreeSet::addSpacetree().
     */
    VerticalData
  };

  static std::string toString(ExchangeMode mode);

  typedef std::pair<int, MPI_Comm> GridDataExchangeMetaInformation;

  /**
   * I use two tags per spacetree per rank: one for boundary data
   * (horizontal) and one for up-down and synchronous (forks) data
   * exchanges (vertical and fork/join).
   *
   * The nice thing about Peano's new data management is that it is
   * stack-only. Furthermore, all stack data exchange is triggered via the
   * spacetree set, i.e. the order of the stacks is known externally, too.
   * So we effectively do not need that many tags even though we need
   * different tags per tree pair.
   */
  GridDataExchangeMetaInformation getGridDataExchangeMetaInformation(
    int          sendingTreeId,
    int          receivingTreeId,
    ExchangeMode exchange
  ) const;

  /**
   * I use this for debugging. When I have a tag, I can ask the node whether
   * the node is aware what this tag actually means.
   */
  static std::string getSemanticsForTag(int tag);

private:
  friend class peano4::parallel::tests::NodeTest;

  static Node _singleton;

  tarch::multicore::BooleanSemaphore _semaphore;

  MPI_Comm _dataExchangeCommunicators[MAX_SPACETREES_PER_RANK];
  bool     _needCommunicator[MAX_SPACETREES_PER_RANK];

  int _currentProgramStep;

  int _rankOrchestrationTag;

  std::map<int, TreeEntry> _treeEntries;

  /**
   * The standard constructor assigns the attributes default values and
   * checks whether the program is compiled using the -DParallel option.
   * If this is not the case, a warning is logged.
   */
  Node() = default;

  /**
   * The copy constructor is private.
   */
  Node(const Node& node) = delete;

  /**
   * Peano maps grid instance threads + MPI ranks onto global IDs through
   * this routine. To identify a global stack number, please use this
   * function and add it to your local stack number.
   */
  inline int getId(int rank, int localTreeId) const {
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    return numberOfRanks * localTreeId + rank;
  }

  /**
   * The operation is not thread-safe as we call it only internally,
   * i.e. you are not supposed to call this function directly.
   *
   * @see getId(int,int)
   */
  void registerId(int id, int masterId);
};
