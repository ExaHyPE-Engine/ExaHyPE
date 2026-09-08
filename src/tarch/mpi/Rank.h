// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  void abort(const int errorCode);
  class Rank;
} // namespace tarch::mpi

/**
 * Represents one program instance within a cluster, i.e. one MPI rank. This
 * class is a singleton.
 *
 * A rank owns the MPI environment: init() brings it up and determines the
 * rank number, the number of ranks, the node layout and the supported thread
 * level, while shutdown() tears it down again. The class furthermore hands
 * out the message tags through reserveFreeTag(). The supervision of the
 * message exchange, i.e. the timeout warnings and the deadlock abort, lives
 * in tarch::mpi::Diagnostics.
 *
 * A detected MPI failure either stops initialisation or terminates the job.
 *
 * @author Tobias Weinzierl
 */
class tarch::mpi::Rank final {
public:
  static tarch::mpi::Rank& getInstance();

  Rank(const Rank&)            = delete;
  Rank(Rank&&)                 = delete;
  Rank& operator=(const Rank&) = delete;
  Rank& operator=(Rank&&)      = delete;

  static int getGlobalMasterRank();

  static int reserveFreeTag(const std::string& fullQualifiedMessageName, const int numberOfTags = 1);

  bool init(int* const argc, char*** const argv);

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

  static int _maximumTagValue;
  static int _nextFreeTag;

  bool _initIsCalled{false};
  int  _rank{-1};
  int  _numberOfRanks{-1};
  int  _providedThreadLevelSupport{-1};

  // Node layout, computed once in init() so later queries need no collective.
  int  _numberOfNodes{1};
  bool _isNodeMaster{true};

  MPI_Comm _communicator{MPI_COMM_WORLD};

  Rank() = default;

  ~Rank() = default;

  static bool validateReservedTagsAreSupported();
};
