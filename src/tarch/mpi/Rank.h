// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  void abort(int errorCode);
  class Rank;
} // namespace tarch::mpi

/**
 * Represents one program instance within a cluster, i.e. one MPI rank. This
 * class is a singleton.
 *
 * A rank owns the MPI environment: init() brings it up and determines the
 * rank id, the number of ranks, the node layout and the supported thread
 * level, while shutdown() tears it down again. The class furthermore hands
 * out the message tags through reserveFreeTag(). The supervision of the
 * message exchange, i.e. the timeout warnings and the deadlock abort, lives
 * in Diagnostics.
 *
 * Every MPI error is fatal, i.e. the whole parallel application is shut down
 * as soon as one occurs.
 *
 * @author Tobias Weinzierl
 */
class tarch::mpi::Rank final {
public:
  static Rank& getInstance();

  Rank(const Rank&)            = delete;
  Rank(Rank&&)                 = delete;
  Rank& operator=(const Rank&) = delete;
  Rank& operator=(Rank&&)      = delete;

  static int getGlobalMasterRank();

  static int reserveFreeTag(const std::string& fullQualifiedMessageName, int numberOfTags = 1);

  bool init(int* argc, char*** argv);

  void shutdown();

  bool isInitialised() const;

  int getRank() const;

  bool isGlobalMaster() const;

  MPI_Comm getCommunicator() const;

  int getNumberOfRanks() const;

  int getNumberOfNodes() const;

  bool isNodeMaster() const;

  int getProvidedThreadLevelSupport() const;

private:
  static Rank _singleton;

  /**
   * Set by init() and stores the number of valid tags.
   */
  static int _maxTags;

  /**
   * Counts the tags that have already been handed out.
   */
  static int _tagCounter;

  /**
   * Is set true if init() is called.
   */
  bool _initIsCalled{false};

  /**
   * Rank (id) of this process.
   */
  int _rank{-1};

  /**
   * Number of ranks in the communicator.
   */
  int _numberOfProcessors{-1};

  int _providedThreadLevelSupport{-1};

  // Node layout, computed once in init() so later queries need no collective.
  int  _numberOfNodes{1};
  bool _isNodeMaster{true};

  /**
   * MPI Communicator this process belongs to.
   */
  MPI_Comm _communicator{MPI_COMM_WORLD};

  /**
   * The rank and the number of processors stay invalid until init() has
   * determined them.
   */
  Rank() = default;

  ~Rank() = default;

  static bool validateMaxTagIsSupported();
};
