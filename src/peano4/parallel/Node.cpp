// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Node.h"

#include "StartTraversalMessage.h"
#include "TreeManagementMessage.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for
#include <unordered_set>

tarch::logging::Log    peano4::parallel::Node::_log("peano4::parallel::Node");
peano4::parallel::Node peano4::parallel::Node::_singleton;

void peano4::parallel::Node::initMPIDatatypes() {
#ifdef WITH_MPI
  StartTraversalMessage::initDatatype();
  TreeManagementMessage::initDatatype();

  peano4::datamanagement::CellMarker::initDatatype();

  peano4::grid::AutomatonState::initDatatype();
  peano4::grid::GridVertex::initDatatype();
  peano4::grid::GridStatistics::initDatatype();
  peano4::grid::GridControlEvent::initDatatype();
#endif
}

void peano4::parallel::Node::shutdownMPIDatatypes() {
#ifdef WITH_MPI
  StartTraversalMessage::shutdownDatatype();
  TreeManagementMessage::shutdownDatatype();

  peano4::datamanagement::CellMarker::shutdownDatatype();

  peano4::grid::AutomatonState::shutdownDatatype();
  peano4::grid::GridVertex::shutdownDatatype();
  peano4::grid::GridControlEvent::shutdownDatatype();
  peano4::grid::GridStatistics::shutdownDatatype();
#endif
}

void peano4::parallel::Node::init() {
  _currentProgramStep   = UndefProgramStep;
  _rankOrchestrationTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::Node - rank orchestration");
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    registerId(0, -1);
  }
  for (int i = 0; i < MaxSpacetreesPerRank; ++i) {
    _dataExchangeCommunicators[i] = MPI_COMM_NULL;
    _needCommunicator[i]          = false;
  }
}

std::string peano4::parallel::Node::toString(ExchangeMode mode) {
  switch (mode) {
  case ExchangeMode::HorizontalData:
    return "horizontal-data";
  case ExchangeMode::VerticalData:
    return "vertical-data";
  }
  return "undef";
}

bool peano4::parallel::Node::isGlobalMaster(int treeId) { return treeId == 0; }

peano4::parallel::Node& peano4::parallel::Node::getInstance() { return _singleton; }

int peano4::parallel::Node::reserveId(int rank, int forTreeId) {
  logDebug(
    "reserveId()",
    "Trying to find and reserve new global id for master id " << forTreeId << " on rank " << rank
  );

  int localTreeId = 0;
  int result      = -1;
  while (result == -1) {
    if (_treeEntries.count(getId(rank, localTreeId)) == 0) {
      result = getId(rank, localTreeId);
    } else {
      logDebug(
        "reserveId()",
        "Local tree "
          << localTreeId << " (global id=" << getId(rank, localTreeId) << ") on rank " << rank << " is already in use"
      );
      localTreeId++;
    }

    if (localTreeId >= MaxSpacetreesPerRank) {
      logWarning(
        "reserveId()",
        "Exceeded limit for maximal available spacetrees per rank (" << MaxSpacetreesPerRank << ")"
      );
      result = -1;
      break;
    }
  }

  if (result != -1) {
    registerId(result, forTreeId);
  } else {
    logError("reserveId()", "Could not reserve a new tree id. No free slot found or limit reached. Abort.");
    std::abort();
  }

  return result;
}

void peano4::parallel::Node::registerId(int id, int masterId) {
  logDebug("registerId()", "Register new global id " << id << " for master id " << masterId);
  tarch::multicore::Lock lock(_semaphore);
  assertion2(_treeEntries.count(id) == 0, id, masterId);
  assertion2(id != masterId, id, masterId);
#ifndef WITH_MPI
  assertion(isGlobalMaster(id) or _treeEntries.count(masterId) == 1);
#endif

  TreeEntry newEntry;

  newEntry.setId(id);
  newEntry.setMaster(masterId);

  _treeEntries.insert(std::pair<int, TreeEntry>(id, newEntry));

  _needCommunicator[getLocalTreeId(id)] = true;
}

void peano4::parallel::Node::createPendingCommunicators() {
#ifdef WITH_MPI
  const int        n = MaxSpacetreesPerRank;
  std::vector<int> local(n), global(n);
  for (int i = 0; i < n; ++i) {
    local[i] = (_needCommunicator[i] and _dataExchangeCommunicators[i] == MPI_COMM_NULL) ? 1 : 0;
  }

  MPI_Allreduce(local.data(), global.data(), n, MPI_INT, MPI_LOR, tarch::mpi::Rank::getInstance().getCommunicator());

  for (int i = 0; i < n; ++i) {
    if (global[i] and _dataExchangeCommunicators[i] == MPI_COMM_NULL) {
      MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_dataExchangeCommunicators[i]);
    }
    _needCommunicator[i] = false;
  }
#endif
}

int peano4::parallel::Node::getNumberOfRegisteredTrees() const { return _treeEntries.size(); }

void peano4::parallel::Node::deregisterId(int id) {
  assertion1(_treeEntries.count(id) == 1, id);

  tarch::multicore::Lock lock(_semaphore);
  _treeEntries.erase(id);

  logDebug("deregisterId()", "Removed tree " << id << " from list of trees");
}

bool peano4::parallel::Node::isStorageStackNumber(int number) {
  return number < grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance;
}

int peano4::parallel::Node::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(int outputStack) {
  int firstPeriodicBCStack = grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
                             - grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks;
  int outputStackMinusOtherStacks = outputStack - firstPeriodicBCStack;
  int mirroredStack = grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks - outputStackMinusOtherStacks - 1;
  return mirroredStack + firstPeriodicBCStack;
}

int peano4::parallel::Node::getOutputStackNumberForHorizontalDataExchange(int id) {
  return grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance + id * StacksPerCommunicationPartner;
}

int peano4::parallel::Node::getInputStackNumberForHorizontalDataExchange(int id) {
  return grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance + id * StacksPerCommunicationPartner + 1;
}

int peano4::parallel::Node::getOutputStackNumberForVerticalDataExchange(int id) {
  return grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance + id * StacksPerCommunicationPartner + 2;
}

int peano4::parallel::Node::getInputStackNumberForVerticalDataExchange(int id) {
  return grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance + id * StacksPerCommunicationPartner + 3;
}

std::bitset<2 * DIMENSIONS> peano4::parallel::Node::getPeriodicBoundaryNumber(
  const tarch::la::Vector<TWO_POWER_D, int>& flags
) {
  std::bitset<2 * DIMENSIONS> result;

  for (int d = 0; d < 2 * DIMENSIONS; d++) {
    bool                                        allFlagsSet = true;
    dfor2(k) tarch::la::Vector<DIMENSIONS, int> entry;
    entry                 = k;
    entry(d % DIMENSIONS) = d / DIMENSIONS;
    allFlagsSet &= flags(utils::dLinearised(entry, 2)) == grid::Spacetree::RankOfPeriodicBoundaryCondition;
    enddforx if (allFlagsSet) { result.set(d); }
  }
  return result;
}

int peano4::parallel::Node::getOutputStackForPeriodicBoundaryExchange(int faceNumber) {
  tarch::la::Vector<DIMENSIONS, int> neighbour(1);
  int                                normal    = faceNumber % DIMENSIONS;
  bool                               lookRight = faceNumber < DIMENSIONS;
  neighbour(normal) += lookRight ? 1 : -1;
  int linearisedIndex = utils::dLinearised(neighbour, 3);
  if (linearisedIndex >= THREE_POWER_D / 2)
    linearisedIndex--;
  const int baseIndex = grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
                        - grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks;
  return baseIndex + linearisedIndex;
}

std::string peano4::parallel::Node::toString(const std::set<PeriodicBoundaryStackIdentifier>& data) {
  std::ostringstream msg;

  msg << "{";
  bool firstEntry = true;
  for (auto& p : data) {
    if (firstEntry) {
      firstEntry = false;
    } else {
      msg << ",";
    }
    msg << "(stack=" << p.first << ",bnd=" << std::bitset<DIMENSIONS>(p.second) << ")";
  }
  msg << "}";

  return msg.str();
}

std::set<peano4::parallel::Node::PeriodicBoundaryStackIdentifier> peano4::parallel::Node::
  getOutputStacksForPeriodicBoundaryExchange(const tarch::la::Vector<TWO_POWER_D, int>& flags) {
  std::bitset<2 * DIMENSIONS> boundaryNumbers = getPeriodicBoundaryNumber(flags);
  logTraceInWith2Arguments("getOutputStacksForPeriodicBoundaryExchange()", flags, boundaryNumbers);

  std::unordered_set<std::bitset<2 * DIMENSIONS>> powerSet;
  for (int i = 0; i < 2 * DIMENSIONS; i++) {
    if (boundaryNumbers[i]) {
      std::bitset<2 * DIMENSIONS> newEntry(0);
      newEntry.set(i);
      powerSet.insert(newEntry);

      for (auto p : powerSet) {
        newEntry = p;
        newEntry.set(i);
        powerSet.insert(newEntry);
      }
    }
  }

  std::set<peano4::parallel::Node::PeriodicBoundaryStackIdentifier> result;
  int                                                               currentStackNumber = 0;
  dfor3(neighbour) if (neighbour != tarch::la::Vector<DIMENSIONS, int>(1)) {
    std::bitset<2 * DIMENSIONS> requiredEntryForThisDirection(0);
    std::bitset<DIMENSIONS>     direction(0);
    for (int d = 0; d < DIMENSIONS; d++) {
      if (neighbour(d) > 1) {
        requiredEntryForThisDirection[d] = true;
        direction[d]                     = true;
      }
      if (neighbour(d) < 1) {
        requiredEntryForThisDirection[d + DIMENSIONS] = true;
        direction[d]                                  = true;
      }
    }
    if (powerSet.count(requiredEntryForThisDirection) > 0) {
      peano4::parallel::Node::PeriodicBoundaryStackIdentifier newEntry(
        peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
          - peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks + currentStackNumber,
        direction.to_ulong()
      );
      assertion5(
        isPeriodicBoundaryExchangeOutputStackNumber(newEntry.first),
        newEntry.first,
        newEntry.second,
        flags,
        peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance,
        peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks
      );
      result.insert(newEntry);
    }
    currentStackNumber++;
  }
  enddforx

    /*
      for (auto p: powerSet) {
        std::bitset<DIMENSIONS> stackAnnotation(0);
        for (int i=0; i<DIMENSIONS; i++) {
          stackAnnotation.set( i, std::bitset<2*DIMENSIONS>(p)[i] or std::bitset<2*DIMENSIONS>(p)[i+DIMENSIONS] );
        }
        peano4::parallel::Node::PeriodicBoundaryStackIdentifier
      newEntry(getOutputStackForPeriodicBoundaryExchange(p),stackAnnotation.to_ulong()); assertion5(
          isPeriodicBoundaryExchangeOutputStackNumber(newEntry.first),
          newEntry.first, newEntry.second, flags,
          peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance,
      peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks
        );
        result.insert( newEntry );
      }
    */

    logTraceOutWith2Arguments(
      "getOutputStacksForPeriodicBoundaryExchange()",
      result.size(),
      (result.size() == 1 ? std::to_string(result.begin()->first) : "")
    );
  return result;
}

int peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(int outputStackNumber) {
  logTraceInWith4Arguments(
    "getPeriodicBoundaryExchangeInputStackNumberForOutputStack()",
    outputStackNumber,
    peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance,
    peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks,
    peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks
  );
  assertion(isPeriodicBoundaryExchangeOutputStackNumber(outputStackNumber));

  int result = outputStackNumber + peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks / 2;
  logTraceOutWith1Argument("getPeriodicBoundaryExchangeInputStackNumberForOutputStack()", result);
  return result;
}

peano4::parallel::Node::PeriodicBoundaryStackIdentifier peano4::parallel::Node::
  getPeriodicBoundaryExchangeInputStackNumberForOutputStack(PeriodicBoundaryStackIdentifier outputStackIdentifier) {
  logTraceInWith5Arguments(
    "getPeriodicBoundaryExchangeInputStackNumberForOutputStack()",
    outputStackIdentifier.first,
    outputStackIdentifier.second,
    peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance,
    peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks,
    peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks
  );
  assertion(isPeriodicBoundaryExchangeOutputStackNumber(outputStackIdentifier.first));

  PeriodicBoundaryStackIdentifier result;
  result.first  = outputStackIdentifier.first + peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks / 2;
  result.second = outputStackIdentifier.second;
  logTraceOutWith2Arguments("getPeriodicBoundaryExchangeInputStackNumberForOutputStack()", result.first, result.second);
  return result;
}

bool peano4::parallel::Node::isPeriodicBoundaryExchangeOutputStackNumber(int id) {
  const bool periodicBoundary = id >= peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
                                        - peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks
                                and id < peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
                                           - peano4::grid::PeanoCurve::NumberOfPeriodicBoundaryConditionStacks / 2;
  return periodicBoundary;
}

bool peano4::parallel::Node::isHorizontalDataExchangeOutputStackNumber(int id) {
  const bool domainBoundary
    = id >= peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
      and ((id - peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance) % StacksPerCommunicationPartner == 0);
  return domainBoundary;
}

bool peano4::parallel::Node::isHorizontalDataExchangeInputStackNumber(int id) {
  const bool domainBoundary
    = id >= peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
      and ((id - peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance) % StacksPerCommunicationPartner == 1);
  return domainBoundary;
}

bool peano4::parallel::Node::isVerticalDataExchangeOutputStackNumber(int id) {
  return id >= peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
         and ((id - peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance) % StacksPerCommunicationPartner == 2);
}

bool peano4::parallel::Node::isVerticalDataExchangeInputStackNumber(int id) {
  return id >= peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance
         and ((id - peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance) % StacksPerCommunicationPartner == 3);
}

int peano4::parallel::Node::getTreeNumberTiedToExchangeStackNumber(int number) {
  return (number - peano4::grid::PeanoCurve::MaxNumberOfCoreStacksPerSpacetreeInstance) / StacksPerCommunicationPartner;
}

bool peano4::parallel::Node::continueToRun() {
#ifdef WITH_MPI
  logTraceIn("continueToRun()");
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    for (int i = 1; i < tarch::mpi::Rank::getInstance().getNumberOfRanks(); i++) {
      StartTraversalMessage message;
      message.setStepIdentifier(_currentProgramStep);
      logDebug("continueToRun()", "Send out " << message.toString() << " to rank " << i);
      StartTraversalMessage::sendAndPollDanglingMessages(message, i, _rankOrchestrationTag);
    }
  } else {
    StartTraversalMessage message;
    StartTraversalMessage::
      receiveAndPollDanglingMessages(message, tarch::mpi::Rank::getGlobalMasterRank(), _rankOrchestrationTag);
    logDebug("continueToRun()", "received message " << message.toString());
    _currentProgramStep = message.getStepIdentifier();
  }
  logTraceOutWith1Argument("continueToRun()", _currentProgramStep);
#endif
  return _currentProgramStep != Terminate;
}

void peano4::parallel::Node::setNextProgramStep(int number) {
  assertion1(number == Terminate or number >= 0, number);
  _currentProgramStep = number;
}

int peano4::parallel::Node::getCurrentProgramStep() const { return _currentProgramStep; }

void peano4::parallel::Node::shutdown() {
  logTraceIn("shutdown()");
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    setNextProgramStep(peano4::parallel::Node::Terminate);
    continueToRun();
  }

  tarch::mpi::Rank::getInstance().barrier([&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

#ifdef WITH_MPI
  for (int i = 0; i < MaxSpacetreesPerRank; i++) {
    if (_dataExchangeCommunicators[i] != MPI_COMM_NULL) {
      MPI_Comm_free(&(_dataExchangeCommunicators[i]));
      _dataExchangeCommunicators[i] = MPI_COMM_NULL;
      _needCommunicator[i]          = false;
    }
  }
#endif

  logTraceOut("shutdown()");
}

peano4::parallel::Node::GridDataExchangeMetaInformation peano4::parallel::Node::
  getGridDataExchangeMetaInformation(int sendingTreeId, int receivingTreeId, ExchangeMode exchange) const {
  logTraceInWith3Arguments("getGridDataExchangeTag()", sendingTreeId, receivingTreeId, toString(exchange));

  assertion(sendingTreeId >= 0);
  assertion(sendingTreeId < MaxSpacetreesPerRank);
  // assertion2(getLocalTreeId(sendingTreeId) <= getNumberOfRegisteredTrees(), getLocalTreeId(sendingTreeId),
  // getNumberOfRegisteredTrees());

  int tag = StacksPerCommunicationPartner / 2 * getLocalTreeId(receivingTreeId);
  switch (exchange) {
  case ExchangeMode::HorizontalData:
    tag += 0;
    break;
  case ExchangeMode::VerticalData:
    tag += 1;
    break;
  }

  logTraceOutWith1Argument("getGridDataExchangeTag()", tag);
  return peano4::parallel::Node::GridDataExchangeMetaInformation(
    tag,
    _dataExchangeCommunicators[getLocalTreeId(sendingTreeId)]
  );
}

std::string peano4::parallel::Node::getSemanticsForTag(int tag) {
  if (tag == getInstance()._rankOrchestrationTag) {
    return "rank orchestration tag";
  }

  assertion(tag >= 0);

  std::string result = "data exchange tag for ";

  switch (tag % (StacksPerCommunicationPartner / 2)) {
  case 0:
    result += "horizontal data";
    break;
  case 1:
    result += "vertical data";
    break;
  }

  return result;
}
