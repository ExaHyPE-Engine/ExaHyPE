// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"

tarch::logging::Log toolbox::loadbalancing::Statistics::_log("toolbox::loadbalancing::Statistics");

toolbox::loadbalancing::Statistics::Statistics():
  _localNumberOfInnerUnrefinedCells(0),
  _globalNumberOfInnerUnrefinedCells(0),
  _localNumberOfOuterUnrefinedCells(0),
  _globalNumberOfOuterUnrefinedCells(0),
  _localNumberOfVertices(0),
  _globalNumberOfVertices(0),
  _localNumberOfHangingVertices(0),
  _globalNumberOfHangingVertices(0),
  _localNumberOfTrees(0),
  _globalNumberOfTrees(1), // Globally we always have at least the root/master tree on rank 0
  _isLocalLoadBalancingEnabled(true),
  _globalNumberOfRanksWithEnabledLoadBalancing(0),
  _localNumberOfSplits(0),
  _globalNumberOfSplits(0),
  _numberOfStateUpdatesWithoutAnySplit(0),
  _localMinTreeLoad(0),
  _localMaxTreeLoad(0),
  _globalMinTreeLoad(0),
  _globalMaxTreeLoad(0),
  _localMemoryUsageInBytes(0),
  _globalMemoryUsageInBytes(0),
  _localMinDepth(std::numeric_limits<int>::max()),
  _localMaxDepth(0),
  _globalMinDepth(std::numeric_limits<int>::max()),
  _globalMaxDepth(0),
  _localMinH(std::numeric_limits<double>::max()),
  _globalMinH(std::numeric_limits<double>::max()),
  _localNumberOfStationarySweeps(0),
  _globalNumberOfStationarySweeps(0),
  _isViewConsistent(false),
  _globalNumberOfInnerUnrefinedCellsBufferIn(0),
  _globalNumberOfInnerUnrefinedCellsBufferOut(0),
  _globalNumberOfOuterUnrefinedCellsBufferIn(0),
  _globalNumberOfOuterUnrefinedCellsBufferOut(0),
  _globalNumberOfVerticesBufferIn(0),
  _globalNumberOfVerticesBufferOut(0),
  _globalNumberOfHangingVerticesBufferIn(0),
  _globalNumberOfHangingVerticesBufferOut(0),
  _globalNumberOfSplitsIn(0),
  _globalNumberOfSplitsOut(0),
  _globalNumberOfTreesIn(0),
  _globalNumberOfTreesOut(0),
  _globalNumberOfRanksWithEnabledLoadBalancingIn(0),
  _globalNumberOfRanksWithEnabledLoadBalancingOut(0),
  _globalMinTreeLoadIn(std::numeric_limits<int>::max()),
  _globalMinTreeLoadOut(std::numeric_limits<int>::max()),
  _globalMaxTreeLoadIn(0),
  _globalMaxTreeLoadOut(0),
  _globalMemoryUsageInBytesIn(0),
  _globalMemoryUsageInBytesOut(0),
  _globalMinDepthIn(std::numeric_limits<int>::max()),
  _globalMinDepthOut(std::numeric_limits<int>::max()),
  _globalMaxDepthIn(0),
  _globalMaxDepthOut(0),
  _globalMinHIn(std::numeric_limits<double>::max()),
  _globalMinHOut(std::numeric_limits<double>::max()),
  _globalNumberOfStationarySweepsIn(0),
  _globalNumberOfStationarySweepsOut(0) {

#ifdef WITH_MPI
  _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
  _globalNumberOfOuterUnrefinedCellsRequest           = nullptr;
  _globalNumberOfVerticesRequest                      = nullptr;
  _globalNumberOfHangingVerticesRequest               = nullptr;
  _globalNumberOfSplitsRequest                        = nullptr;
  _globalNumberOfTreesRequest                         = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  _globalMinTreeLoadRequest                           = nullptr;
  _globalMaxTreeLoadRequest                           = nullptr;
  _globalMemoryUsageInBytesRequest                    = nullptr;
  _globalMinDepthRequest                              = nullptr;
  _globalMaxDepthRequest                              = nullptr;
  _globalMinHRequest                                  = nullptr;
  _globalNumberOfStationarySweepsRequest              = nullptr;
#endif
}

void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
#ifdef WITH_MPI
  if (_globalNumberOfInnerUnrefinedCellsRequest != nullptr) {
    tarch::mpi::wait(
      *_globalNumberOfInnerUnrefinedCellsRequest,
      "toolbox::loadbalancing::Statistics",
      "waitForGlobalDataExchange"
    );
    tarch::mpi::wait(
      *_globalNumberOfOuterUnrefinedCellsRequest,
      "toolbox::loadbalancing::Statistics",
      "waitForGlobalDataExchange"
    );
    tarch::mpi::
      wait(*_globalNumberOfVerticesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::
      wait(*_globalNumberOfHangingVerticesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfSplitsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfTreesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(
      *_globalNumberOfRanksWithEnabledLoadBalancingRequest,
      "toolbox::loadbalancing::Statistics",
      "waitForGlobalDataExchange"
    );
    tarch::mpi::wait(*_globalMinTreeLoadRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMaxTreeLoadRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::
      wait(*_globalMemoryUsageInBytesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMinDepthRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMaxDepthRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMinHRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::
      wait(*_globalNumberOfStationarySweepsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");

    delete _globalNumberOfInnerUnrefinedCellsRequest;
    delete _globalNumberOfOuterUnrefinedCellsRequest;
    delete _globalNumberOfVerticesRequest;
    delete _globalNumberOfHangingVerticesRequest;
    delete _globalNumberOfSplitsRequest;
    delete _globalNumberOfTreesRequest;
    delete _globalNumberOfRanksWithEnabledLoadBalancingRequest;
    delete _globalMinTreeLoadRequest;
    delete _globalMaxTreeLoadRequest;
    delete _globalMemoryUsageInBytesRequest;
    delete _globalMinDepthRequest;
    delete _globalMaxDepthRequest;
    delete _globalMinHRequest;
    delete _globalNumberOfStationarySweepsRequest;

    _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
    _globalNumberOfOuterUnrefinedCellsRequest           = nullptr;
    _globalNumberOfVerticesRequest                      = nullptr;
    _globalNumberOfHangingVerticesRequest               = nullptr;
    _globalNumberOfSplitsRequest                        = nullptr;
    _globalNumberOfTreesRequest                         = nullptr;
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
    _globalMinTreeLoadRequest                           = nullptr;
    _globalMaxTreeLoadRequest                           = nullptr;
    _globalMemoryUsageInBytesRequest                    = nullptr;
    _globalMinDepthRequest                              = nullptr;
    _globalMaxDepthRequest                              = nullptr;
    _globalMinHRequest                                  = nullptr;
    _globalNumberOfStationarySweepsRequest              = nullptr;
  }
#endif
}

void toolbox::loadbalancing::Statistics::updateGlobalView() {
  const auto& statistics = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  const auto& spacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  _localNumberOfTrees               = spacetrees.size();
  _localNumberOfInnerUnrefinedCells = statistics.numberOfLocalUnrefinedCells;
  _localNumberOfOuterUnrefinedCells = statistics.numberOfRemoteUnrefinedCells;
  _localNumberOfVertices            = statistics.numberOfVertices;
  _localNumberOfHangingVertices     = statistics.numberOfHangingVertices;
  _localMemoryUsageInBytes          = statistics.getMemoryUsageInBytes();
  _localMinDepth                    = statistics.minDepth;
  _localMaxDepth                    = statistics.maxDepth;
  _localMinH                        = tarch::la::max(statistics.minH);
  _localNumberOfStationarySweeps    = statistics.numberOfStationarySweeps;

  {
    if (spacetrees.empty()) {
      _localMinTreeLoad = std::numeric_limits<int>::max();
      _localMaxTreeLoad = 0;
    } else {
      int localMin = std::numeric_limits<int>::max();
      int localMax = 0;
      for (auto tree : spacetrees) {
        const auto& stats = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(tree);
        // TODO: Could be changed to another metric in the future
        const auto load = stats.numberOfLocalUnrefinedCells;
        if (load < localMin) {
          localMin = load;
        }
        if (load > localMax) {
          localMax = load;
        }
      }
      _localMinTreeLoad = localMin;
      _localMaxTreeLoad = localMax;
    }
  }

  _isViewConsistent = true;

  _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCells;
  _globalNumberOfOuterUnrefinedCells           = _localNumberOfOuterUnrefinedCells;
  _globalNumberOfVertices                      = _localNumberOfVertices;
  _globalNumberOfHangingVertices               = _localNumberOfHangingVertices;
  _globalNumberOfSplits                        = _localNumberOfSplits;
  _globalNumberOfTrees                         = _localNumberOfTrees;
  _globalNumberOfRanksWithEnabledLoadBalancing = _isLocalLoadBalancingEnabled ? 1 : 0;
  _globalMinTreeLoad                           = _localMinTreeLoad;
  _globalMaxTreeLoad                           = _localMaxTreeLoad;
  _globalMemoryUsageInBytes                    = _localMemoryUsageInBytes;
  _globalMinDepth                              = _localMinDepth;
  _globalMaxDepth                              = _localMaxDepth;
  _globalMinH                                  = _localMinH;
  _globalNumberOfStationarySweeps              = _localNumberOfStationarySweeps;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
#ifdef WITH_MPI
    if (_globalNumberOfInnerUnrefinedCellsRequest != nullptr) {

      waitForGlobalDataExchange();

      _globalNumberOfInnerUnrefinedCells           = _globalNumberOfInnerUnrefinedCellsBufferIn;
      _globalNumberOfOuterUnrefinedCells           = _globalNumberOfOuterUnrefinedCellsBufferIn;
      _globalNumberOfVertices                      = _globalNumberOfVerticesBufferIn;
      _globalNumberOfHangingVertices               = _globalNumberOfHangingVerticesBufferIn;
      _globalNumberOfSplits                        = _globalNumberOfSplitsIn;
      _globalNumberOfTrees                         = _globalNumberOfTreesIn;
      _globalNumberOfRanksWithEnabledLoadBalancing = _globalNumberOfRanksWithEnabledLoadBalancingIn;
      _globalMinTreeLoad                           = _globalMinTreeLoadIn;
      _globalMaxTreeLoad                           = _globalMaxTreeLoadIn;
      _globalMemoryUsageInBytes                    = _globalMemoryUsageInBytesIn;
      _globalMinDepth                              = _globalMinDepthIn;
      _globalMaxDepth                              = _globalMaxDepthIn;
      _globalMinH                                  = _globalMinHIn;
      _globalNumberOfStationarySweeps              = _globalNumberOfStationarySweepsIn;

      if (_globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells) {
        logDebug(
          "updateGlobalView()",
          "Local number of cells ("
            << _localNumberOfInnerUnrefinedCells << ") is bigger than global cell count ("
            << _globalNumberOfInnerUnrefinedCells
            << "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Statistics might have inconsistent view of world."
        );
        _isViewConsistent                  = false;
        _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCells;
      }
    }

    _globalNumberOfInnerUnrefinedCellsRequest           = new MPI_Request();
    _globalNumberOfOuterUnrefinedCellsRequest           = new MPI_Request();
    _globalNumberOfVerticesRequest                      = new MPI_Request();
    _globalNumberOfHangingVerticesRequest               = new MPI_Request();
    _globalNumberOfSplitsRequest                        = new MPI_Request();
    _globalNumberOfTreesRequest                         = new MPI_Request();
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = new MPI_Request();
    _globalMinTreeLoadRequest                           = new MPI_Request();
    _globalMaxTreeLoadRequest                           = new MPI_Request();
    _globalMemoryUsageInBytesRequest                    = new MPI_Request();
    _globalMinDepthRequest                              = new MPI_Request();
    _globalMaxDepthRequest                              = new MPI_Request();
    _globalMinHRequest                                  = new MPI_Request();
    _globalNumberOfStationarySweepsRequest              = new MPI_Request();

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfOuterUnrefinedCellsBufferOut     = _localNumberOfOuterUnrefinedCells;
    _globalNumberOfVerticesBufferOut                = _localNumberOfVertices;
    _globalNumberOfHangingVerticesBufferOut         = _localNumberOfHangingVertices;
    _globalNumberOfSplitsOut                        = _localNumberOfSplits;
    _globalNumberOfTreesOut                         = _localNumberOfTrees;
    _globalNumberOfRanksWithEnabledLoadBalancingOut = _isLocalLoadBalancingEnabled ? 1 : 0;
    _globalMinTreeLoadOut                           = _localMinTreeLoad;
    _globalMaxTreeLoadOut                           = _localMaxTreeLoad;
    _globalMemoryUsageInBytesOut                    = _localMemoryUsageInBytes;
    _globalMinDepthOut                              = _localMinDepth;
    _globalMaxDepthOut                              = _localMaxDepth;
    _globalMinHOut                                  = _localMinH;
    _globalNumberOfStationarySweepsOut              = _localNumberOfStationarySweeps;

    MPI_Iallreduce(
      &_globalNumberOfRanksWithEnabledLoadBalancingOut, // send
      &_globalNumberOfRanksWithEnabledLoadBalancingIn,  // receive
      1,                                                // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfRanksWithEnabledLoadBalancingRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfTreesOut, // send
      &_globalNumberOfTreesIn,  // receive
      1,                        // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfTreesRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfInnerUnrefinedCellsBufferOut, // send
      &_globalNumberOfInnerUnrefinedCellsBufferIn,  // receive
      1,                                            // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfInnerUnrefinedCellsRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfOuterUnrefinedCellsBufferOut, // send
      &_globalNumberOfOuterUnrefinedCellsBufferIn,  // receive
      1,                                            // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfOuterUnrefinedCellsRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfVerticesBufferOut, // send
      &_globalNumberOfVerticesBufferIn,  // receive
      1,                                 // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfVerticesRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfHangingVerticesBufferOut, // send
      &_globalNumberOfHangingVerticesBufferIn,  // receive
      1,                                        // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfHangingVerticesRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfSplitsOut, // send
      &_globalNumberOfSplitsIn,  // receive
      1,                         // count
      MPI_INT,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfSplitsRequest
    );
    MPI_Iallreduce(
      &_globalMinTreeLoadOut,
      &_globalMinTreeLoadIn,
      1,
      MPI_INT,
      MPI_MIN,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMinTreeLoadRequest
    );
    MPI_Iallreduce(
      &_globalMaxTreeLoadOut,
      &_globalMaxTreeLoadIn,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMaxTreeLoadRequest
    );
    MPI_Iallreduce(
      &_globalMemoryUsageInBytesOut, // send
      &_globalMemoryUsageInBytesIn,  // receive
      1,                             // count
      MPI_UINT64_T,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMemoryUsageInBytesRequest
    );
    MPI_Iallreduce(
      &_globalMinDepthOut, // send
      &_globalMinDepthIn,  // receive
      1,                   // count
      MPI_INT,
      MPI_MIN,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMinDepthRequest
    );
    MPI_Iallreduce(
      &_globalMaxDepthOut, // send
      &_globalMaxDepthIn,  // receive
      1,                   // count
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMaxDepthRequest
    );
    MPI_Iallreduce(
      &_globalMinHOut, // send
      &_globalMinHIn,  // receive
      1,               // count
      MPI_DOUBLE,
      MPI_MIN,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalMinHRequest
    );
    MPI_Iallreduce(
      &_globalNumberOfStationarySweepsOut,
      &_globalNumberOfStationarySweepsIn,
      1,
      MPI_INT,
      MPI_MIN,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalNumberOfStationarySweepsRequest
    );
#endif
  }

  if (_globalNumberOfSplits == 0 and _localNumberOfSplits == 0 and _numberOfStateUpdatesWithoutAnySplit < 65536) {
    _numberOfStateUpdatesWithoutAnySplit++;
  } else if (_globalNumberOfSplits > 0 or _localNumberOfSplits > 0) {
    _numberOfStateUpdatesWithoutAnySplit = 0;
  }

  _localNumberOfSplits = 0;
}

int toolbox::loadbalancing::Statistics::getLocalNumberOfInnerUnrefinedCells() const {
  return _localNumberOfInnerUnrefinedCells;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfInnerUnrefinedCells() const {
  return _globalNumberOfInnerUnrefinedCells;
}

int toolbox::loadbalancing::Statistics::getLocalNumberOfOuterUnrefinedCells() const {
  return _localNumberOfOuterUnrefinedCells;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfOuterUnrefinedCells() const {
  return _globalNumberOfOuterUnrefinedCells;
}

int toolbox::loadbalancing::Statistics::getLocalNumberOfVertices() const { return _localNumberOfVertices; }

int toolbox::loadbalancing::Statistics::getGlobalNumberOfVertices() const { return _globalNumberOfVertices; }

int toolbox::loadbalancing::Statistics::getLocalNumberOfHangingVertices() const {
  return _localNumberOfHangingVertices;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfHangingVertices() const {
  return _globalNumberOfHangingVertices;
}

int toolbox::loadbalancing::Statistics::getLocalNumberOfTrees() const { return _localNumberOfTrees; }

int toolbox::loadbalancing::Statistics::getGlobalNumberOfTrees() const { return _globalNumberOfTrees; }

void toolbox::loadbalancing::Statistics::incLocalNumberOfSplits(int delta) {
  assertion(delta > 0);
  _localNumberOfSplits += delta;
}

int toolbox::loadbalancing::Statistics::getNumberOfStateUpdatesWithoutAnySplit() const {
  return _numberOfStateUpdatesWithoutAnySplit;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfRanksWithEnabledLoadBalancing() const {
  return _globalNumberOfRanksWithEnabledLoadBalancing;
}

int toolbox::loadbalancing::Statistics::getGlobalMinTreeLoad() const { return _globalMinTreeLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxTreeLoad() const { return _globalMaxTreeLoad; }

std::size_t toolbox::loadbalancing::Statistics::getLocalMemoryUsageInBytes() const { return _localMemoryUsageInBytes; }

std::size_t toolbox::loadbalancing::Statistics::getGlobalMemoryUsageInBytes() const {
  return _globalMemoryUsageInBytes;
}

int toolbox::loadbalancing::Statistics::getLocalMinDepth() const { return _localMinDepth; }

int toolbox::loadbalancing::Statistics::getGlobalMinDepth() const { return _globalMinDepth; }

int toolbox::loadbalancing::Statistics::getLocalMaxDepth() const { return _localMaxDepth; }

int toolbox::loadbalancing::Statistics::getGlobalMaxDepth() const { return _globalMaxDepth; }

double toolbox::loadbalancing::Statistics::getLocalMinH() const { return _localMinH; }

double toolbox::loadbalancing::Statistics::getGlobalMinH() const { return _globalMinH; }

int toolbox::loadbalancing::Statistics::getLocalNumberOfStationarySweeps() const {
  return _localNumberOfStationarySweeps;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfStationarySweeps() const {
  return _globalNumberOfStationarySweeps;
}

bool toolbox::loadbalancing::Statistics::hasConsistentViewOfWorld() const { return _isViewConsistent; }

void toolbox::loadbalancing::Statistics::notifyOfStateChange(State state) {
  _isLocalLoadBalancingEnabled = state != State::SwitchedOff;
}

std::string toolbox::loadbalancing::Statistics::toString() const {
  std::ostringstream msg;

  msg
    << "(LocalNumberOfInnerUnrefinedCells=" << _localNumberOfInnerUnrefinedCells
    << ",GlobalNumberOfInnerUnrefinedCells=" << _globalNumberOfInnerUnrefinedCells << ",LocalNumberOfTrees="
    << _localNumberOfTrees << ",GlobalNumberOfTrees=" << _globalNumberOfTrees << ",IsLocalLoadBalancingEnabled="
    << _isLocalLoadBalancingEnabled << ",GlobalNumberOfRanksWithEnabledLoadBalancing="
    << _globalNumberOfRanksWithEnabledLoadBalancing << ",LocalNumberOfSplits=" << _localNumberOfSplits
    << ",GlobalNumberOfSplits=" << _globalNumberOfSplits << ",NumberOfStateUpdatesWithoutAnySplit="
    << _numberOfStateUpdatesWithoutAnySplit << ",IsViewConsistent=" << _isViewConsistent << ")";

  return msg.str();
}
