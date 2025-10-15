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
  _isViewConsistent(false) {

#ifdef WITH_MPI
  _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
  _globalNumberOfSplitsRequest                        = nullptr;
  _globalNumberOfTreesRequest                         = nullptr;
  _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
#endif
}

void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
#ifdef WITH_MPI
  if (_globalNumberOfInnerUnrefinedCellsRequest != nullptr) {
    tarch::mpi::wait(*_globalNumberOfInnerUnrefinedCellsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfSplitsRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfTreesRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");
    tarch::mpi::wait(*_globalNumberOfRanksWithEnabledLoadBalancingRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");

    delete _globalNumberOfInnerUnrefinedCellsRequest;
    delete _globalNumberOfSplitsRequest;
    delete _globalNumberOfTreesRequest;
    delete _globalNumberOfRanksWithEnabledLoadBalancingRequest;

    _globalNumberOfInnerUnrefinedCellsRequest           = nullptr;
    _globalNumberOfSplitsRequest                        = nullptr;
    _globalNumberOfTreesRequest                         = nullptr;
    _globalNumberOfRanksWithEnabledLoadBalancingRequest = nullptr;
  }
#endif
}

void toolbox::loadbalancing::Statistics::updateGlobalView() {
  _localNumberOfInnerUnrefinedCells = peano4::parallel::SpacetreeSet::getInstance()
                                        .getGridStatistics()
                                        .getNumberOfLocalUnrefinedCells();
  _localNumberOfTrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();

  _isViewConsistent = true;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    _globalNumberOfInnerUnrefinedCells           = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplits                        = _localNumberOfSplits;
    _globalNumberOfTrees                         = _localNumberOfTrees;
    _globalNumberOfRanksWithEnabledLoadBalancing = _isLocalLoadBalancingEnabled ? 1 : 0;
  } else {
#ifdef WITH_MPI
    waitForGlobalDataExchange();

    _globalNumberOfInnerUnrefinedCells           = _globalNumberOfInnerUnrefinedCellsBufferIn;
    _globalNumberOfSplits                        = _globalNumberOfSplitsIn;
    _globalNumberOfTrees                         = _globalNumberOfTreesIn;
    _globalNumberOfRanksWithEnabledLoadBalancing = _globalNumberOfRanksWithEnabledLoadBalancingIn;

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

    _globalNumberOfInnerUnrefinedCellsBufferOut     = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfSplitsOut                        = _localNumberOfSplits;
    _globalNumberOfTreesOut                         = _localNumberOfTrees;
    _globalNumberOfRanksWithEnabledLoadBalancingOut = _isLocalLoadBalancingEnabled ? 1 : 0;

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
