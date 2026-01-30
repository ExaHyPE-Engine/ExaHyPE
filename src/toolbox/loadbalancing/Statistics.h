// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "State.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

namespace toolbox::loadbalancing {
  class Statistics;
} // namespace toolbox::loadbalancing

/**
 * Statistics helper routine for load balancing
 *
 * Please note that we assume a SPMD paradigm, i.e. if one rank uses
 * statistics, the other ranks all have to use such an object, too. The
 * individual objects communicate with each other.
 */
class toolbox::loadbalancing::Statistics {
public:
  /**
   * Set all the MPI requests to nullptr.
   */
  Statistics();
  ~Statistics() = default;

  void updateGlobalView();

  int getLocalNumberOfInnerUnrefinedCells() const;
  int getGlobalNumberOfInnerUnrefinedCells() const;

  int getLocalNumberOfOuterUnrefinedCells() const;
  int getGlobalNumberOfOuterUnrefinedCells() const;

  int getLocalNumberOfVertices() const;
  int getGlobalNumberOfVertices() const;

  int getLocalNumberOfHangingVertices() const;
  int getGlobalNumberOfHangingVertices() const;

  int getLocalNumberOfTrees() const;
  int getGlobalNumberOfTrees() const;

  void incLocalNumberOfSplits(int delta = 1);
  int  getNumberOfStateUpdatesWithoutAnySplit() const;

  int getGlobalNumberOfRanksWithEnabledLoadBalancing() const;

  int getGlobalMinTreeLoad() const;
  int getGlobalMaxTreeLoad() const;

  std::size_t getLocalMemoryUsageInBytes() const;
  std::size_t getGlobalMemoryUsageInBytes() const;

  int getLocalMinDepth() const;
  int getGlobalMinDepth() const;

  int getLocalMaxDepth() const;
  int getGlobalMaxDepth() const;

  double getLocalMinH() const;
  double getGlobalMinH() const;

  int getLocalNumberOfStationarySweeps() const;
  int getGlobalNumberOfStationarySweeps() const;

  /**
   * If the stats spot some inconsistencies (local number of cells is bigger
   * than global number, e.g.) this means that the non-blocking data exchange
   * is lagging behind. In this case, this routine returns false;
   */
  bool hasConsistentViewOfWorld() const;
  void notifyOfStateChange(State state);

  std::string toString() const;

private:
  static tarch::logging::Log _log;

  int _localNumberOfInnerUnrefinedCells;
  int _globalNumberOfInnerUnrefinedCells;

  int _localNumberOfOuterUnrefinedCells;
  int _globalNumberOfOuterUnrefinedCells;

  int _localNumberOfVertices;
  int _globalNumberOfVertices;

  int _localNumberOfHangingVertices;
  int _globalNumberOfHangingVertices;

  int _localNumberOfTrees;
  int _globalNumberOfTrees;

  bool _isLocalLoadBalancingEnabled;
  int  _globalNumberOfRanksWithEnabledLoadBalancing;

  /**
   * This is my local accumulator where I keep track of how often I did
   * split in this iteration. At the end of each iteration, I roll this
   * one over into global or send it out.
   */
  int _localNumberOfSplits;

  /**
   * Lags behind global number by one iteration in an MPI world as data
   * will arrive with one iteration delay.
   */
  int _globalNumberOfSplits;

  int _numberOfStateUpdatesWithoutAnySplit;

  int _localMinTreeLoad;
  int _localMaxTreeLoad;
  int _globalMinTreeLoad;
  int _globalMaxTreeLoad;

  std::uint64_t _localMemoryUsageInBytes;
  std::uint64_t _globalMemoryUsageInBytes;

  int _localMinDepth;
  int _localMaxDepth;
  int _globalMinDepth;
  int _globalMaxDepth;

  double _localMinH;
  double _globalMinH;

  int _localNumberOfStationarySweeps;
  int _globalNumberOfStationarySweeps;

  bool _isViewConsistent;

#ifdef WITH_MPI
  MPI_Request* _globalNumberOfInnerUnrefinedCellsRequest;
  MPI_Request* _globalNumberOfOuterUnrefinedCellsRequest;
  MPI_Request* _globalNumberOfVerticesRequest;
  MPI_Request* _globalNumberOfHangingVerticesRequest;
  MPI_Request* _globalNumberOfSplitsRequest;
  MPI_Request* _globalNumberOfTreesRequest;
  MPI_Request* _globalNumberOfRanksWithEnabledLoadBalancingRequest;
  MPI_Request* _globalMinTreeLoadRequest;
  MPI_Request* _globalMaxTreeLoadRequest;
  MPI_Request* _globalMemoryUsageInBytesRequest;
  MPI_Request* _globalMinDepthRequest;
  MPI_Request* _globalMaxDepthRequest;
  MPI_Request* _globalMinHRequest;
  MPI_Request* _globalNumberOfStationarySweepsRequest;
#endif

  int           _globalNumberOfInnerUnrefinedCellsBufferIn;
  int           _globalNumberOfInnerUnrefinedCellsBufferOut;
  int           _globalNumberOfOuterUnrefinedCellsBufferIn;
  int           _globalNumberOfOuterUnrefinedCellsBufferOut;
  int           _globalNumberOfVerticesBufferIn;
  int           _globalNumberOfVerticesBufferOut;
  int           _globalNumberOfHangingVerticesBufferIn;
  int           _globalNumberOfHangingVerticesBufferOut;
  int           _globalNumberOfSplitsIn;
  int           _globalNumberOfSplitsOut;
  int           _globalNumberOfTreesIn;
  int           _globalNumberOfTreesOut;
  int           _globalNumberOfRanksWithEnabledLoadBalancingIn;
  int           _globalNumberOfRanksWithEnabledLoadBalancingOut;
  int           _globalMinTreeLoadIn;
  int           _globalMinTreeLoadOut;
  int           _globalMaxTreeLoadIn;
  int           _globalMaxTreeLoadOut;
  std::uint64_t _globalMemoryUsageInBytesIn;
  std::uint64_t _globalMemoryUsageInBytesOut;
  int           _globalMinDepthIn;
  int           _globalMinDepthOut;
  int           _globalMaxDepthIn;
  int           _globalMaxDepthOut;
  double        _globalMinHIn;
  double        _globalMinHOut;
  int           _globalNumberOfStationarySweepsIn;
  int           _globalNumberOfStationarySweepsOut;

  void waitForGlobalDataExchange();
};
