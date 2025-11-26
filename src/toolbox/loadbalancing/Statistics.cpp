// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"

tarch::logging::Log toolbox::loadbalancing::Statistics::_log("toolbox::loadbalancing::Statistics");

toolbox::loadbalancing::Statistics::Statistics():
  _localNumberOfInnerUnrefinedCells(0),
  _globalNumberOfInnerUnrefinedCells(0),
  _localNumberOfTrees(1),
  _globalNumberOfTrees(1),
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
  _isViewConsistent(false) {

#ifdef WITH_MPI
  _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
  _globalNumberOfSplitsRequest                        = nullptr;
  _globalNumberOfTreesRequest                         = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  _globalMinTreeLoadRequest                           = nullptr;
  _globalMaxTreeLoadRequest                           = nullptr;
  _globalMemoryUsageInBytesRequest                    = nullptr;
#endif
}

void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
#ifdef WITH_MPI
  if (_globalNumberOfInnerUnrefinedCellsRequest != nullptr) {
    tarch::mpi::wait(*_globalNumberOfInnerUnrefinedCellsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfSplitsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfTreesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfRanksWithEnabledLoadBalancingRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMinTreeLoadRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMaxTreeLoadRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalMemoryUsageInBytesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");

    delete _globalNumberOfInnerUnrefinedCellsRequest;
    delete _globalNumberOfSplitsRequest;
    delete _globalNumberOfTreesRequest;
    delete _globalNumberOfRanksWithEnabledLoadBalancingRequest;
    delete _globalMinTreeLoadRequest;
    delete _globalMaxTreeLoadRequest;
    delete _globalMemoryUsageInBytesRequest;

    _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
    _globalNumberOfSplitsRequest                        = nullptr;
    _globalNumberOfTreesRequest                         = nullptr;
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
    _globalMinTreeLoadRequest                           = nullptr;
    _globalMaxTreeLoadRequest                           = nullptr;
    _globalMemoryUsageInBytesRequest                    = nullptr;
  }
#endif
}

void toolbox::loadbalancing::Statistics::updateGlobalView() {
  _localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance()
                                        .getGridStatistics()
                                        .numberOfLocalUnrefinedCells;
  _localNumberOfTrees      = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
  _localMemoryUsageInBytes = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMemoryUsageInBytes();

  {
    const auto& trees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
    if (trees.empty()) {
      _localMinTreeLoad = 0;
      _localMaxTreeLoad = 0;
    } else {
      int localMin = std::numeric_limits<int>::max();
      int localMax = 0;
      for (int treeId : trees) {
        const auto& stats = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(treeId);
        const int   load  = stats.numberOfLocalUnrefinedCells;
        if (load < localMin) {
          localMin = load;
        }
        if (load > localMax) {
          localMax = load;
        }
      }
      _localMinTreeLoad = (localMin == std::numeric_limits<int>::max() ? 0 : localMin);
      _localMaxTreeLoad = localMax;
    }
  }

  _isViewConsistent = true;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = _localNumberOfTrees;
    _globalNumberOfRanksWithEnabledLoadBalancing = _isLocalLoadBalancingEnabled ? 1 : 0;
    _globalMinTreeLoad                           = _localMinTreeLoad;
    _globalMaxTreeLoad                           = _localMaxTreeLoad;
    _globalMemoryUsageInBytes                    = _localMemoryUsageInBytes;
  } else {
#ifdef WITH_MPI
    waitForGlobalDataExchange();

    _globalNumberOfInnerUnrefinedCells           = _globalNumberOfInnerUnrefinedCellsBufferIn;
    _globalNumberOfSplits                        = _globalNumberOfSplitsIn;
    _globalNumberOfTrees                         = _globalNumberOfTreesIn;
    _globalNumberOfRanksWithEnabledLoadBalancing = _globalNumberOfRanksWithEnabledLoadBalancingIn;
    _globalMinTreeLoad                           = _globalMinTreeLoadIn;
    _globalMaxTreeLoad                           = _globalMaxTreeLoadIn;
    _globalMemoryUsageInBytes                    = _globalMemoryUsageInBytesIn;

    if (_globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells) {
      logDebug(
        "updateGlobalView()",
        "Local number of cells ("
          << _localNumberOfInnerUnrefinedCells << ") is bigger than global cell count ("
          << _globalNumberOfInnerUnrefinedCells
          << "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Statistics might have inconsistent view of world"
      );
      _isViewConsistent                  = false;
      _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCells;
    }

    _globalNumberOfInnerUnrefinedCellsRequest           = new MPI_Request();
    _globalNumberOfSplitsRequest                        = new MPI_Request();
    _globalNumberOfTreesRequest                         = new MPI_Request();
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = new MPI_Request();
    _globalMinTreeLoadRequest                           = new MPI_Request();
    _globalMaxTreeLoadRequest                           = new MPI_Request();
    _globalMemoryUsageInBytesRequest                    = new MPI_Request();

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplitsOut                        = _localNumberOfSplits;
    _globalNumberOfTreesOut                         = _localNumberOfTrees;
    _globalNumberOfRanksWithEnabledLoadBalancingOut = _isLocalLoadBalancingEnabled ? 1 : 0;
    _globalMinTreeLoadOut                           = _localMinTreeLoad;
    _globalMaxTreeLoadOut                           = _localMaxTreeLoad;
    _globalMemoryUsageInBytesOut                    = _localMemoryUsageInBytes;

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

double toolbox::loadbalancing::Statistics::getGlobalTreeLoadRatioInPercentage() const {
  if (_globalMaxTreeLoad <= 0) {
    return 100.0; // Nothing to balance -> perfect
  }
  const double ratio = static_cast<double>(_globalMinTreeLoad) / static_cast<double>(_globalMaxTreeLoad);
  double       pct   = ratio * 100.0;
  if (pct < 0.0) {
    pct = 0.0;
  }
  if (pct > 100.0) {
    pct = 100.0;
  }
  return pct;
}

std::size_t toolbox::loadbalancing::Statistics::getLocalMemoryUsageInBytes() const { return _localMemoryUsageInBytes; }

std::size_t toolbox::loadbalancing::Statistics::getGlobalMemoryUsageInBytes() const {
  return _globalMemoryUsageInBytes;
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
