// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  void abort(const int errorCode);
  class Rank;
} // namespace tarch::mpi

/**
 * Singleton of one MPI rank. init() initialises MPI and reads the rank number, the rank count, the
 * node layout and the thread level, and shutdown() finalises MPI. reserveFreeTag() hands out the
 * message tags.
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

  int getNumberOfRanksOnNode() const;

  int getRankOnNode() const;

  bool isNodeMaster() const;

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
  int  _numberOfRanksOnNode{1};
  int  _rankOnNode{0};
  bool _isNodeMaster{true};

  MPI_Comm _communicator{MPI_COMM_WORLD};

  Rank() = default;

  ~Rank() = default;

  static bool validateReservedTagsAreSupported();
};
