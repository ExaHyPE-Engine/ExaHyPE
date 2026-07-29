// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace {
  enum WordIndex : int {
    SumInnerUnrefinedCells = 0,
    SumInnerRefinedCells,
    SumOuterUnrefinedCells,
    SumVertices,
    SumHangingVertices,
    SumSplits,
    SumTrees,
    SumRanksWithTrees,
    MinTreeLoad,
    MaxTreeLoad,
    MaxTreeLoadOwnerRank,
    SumMemoryUsageInBytes,
    SumProcessMemoryUsageInBytes,
    MinDepth,
    MaxDepth,
    MinMeshWidthBits,
    MinStationarySweeps,
    OrUniformGridComplete,
    MinTreeOuterLoad,
    MaxTreeOuterLoad,
    MinRankLoad,
    MaxRankLoad,
    SumNodeTotalMemoryInBytes,
    SumNodeFreeMemoryInBytes
  };

  static_assert(SumNodeFreeMemoryInBytes + 1 == toolbox::loadbalancing::Statistics::NumberOfWords);

  std::uint64_t toBits(const double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t), "Double must use 64 bits.");
    std::uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  double fromBits(const std::uint64_t value) {
    double result = 0.0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  std::uint64_t encodeSigned(const std::int64_t value) {
    std::uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  std::int64_t decodeSigned(const std::uint64_t value) {
    std::int64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  void reduceRecord(const std::uint64_t* const input, std::uint64_t* const result) {
    result[SumInnerUnrefinedCells] += input[SumInnerUnrefinedCells];
    result[SumInnerRefinedCells] += input[SumInnerRefinedCells];
    result[SumOuterUnrefinedCells] += input[SumOuterUnrefinedCells];
    result[SumVertices] += input[SumVertices];
    result[SumHangingVertices] += input[SumHangingVertices];
    result[SumSplits] += input[SumSplits];
    result[SumTrees] += input[SumTrees];
    result[SumRanksWithTrees] += input[SumRanksWithTrees];
    result[SumMemoryUsageInBytes] += input[SumMemoryUsageInBytes];
    result[SumProcessMemoryUsageInBytes] += input[SumProcessMemoryUsageInBytes];
    result[SumNodeTotalMemoryInBytes] += input[SumNodeTotalMemoryInBytes];
    result[SumNodeFreeMemoryInBytes] += input[SumNodeFreeMemoryInBytes];

    result[MinTreeLoad] = std::min(result[MinTreeLoad], input[MinTreeLoad]);
    if (input[MaxTreeLoad] > result[MaxTreeLoad]) {
      result[MaxTreeLoad]          = input[MaxTreeLoad];
      result[MaxTreeLoadOwnerRank] = input[MaxTreeLoadOwnerRank];
    } else if (input[MaxTreeLoad] == result[MaxTreeLoad]) {
      result[MaxTreeLoadOwnerRank] = std::min(result[MaxTreeLoadOwnerRank], input[MaxTreeLoadOwnerRank]);
    }
    result[MinTreeOuterLoad] = std::min(result[MinTreeOuterLoad], input[MinTreeOuterLoad]);
    result[MaxTreeOuterLoad] = std::max(result[MaxTreeOuterLoad], input[MaxTreeOuterLoad]);
    result[MinRankLoad]      = std::min(result[MinRankLoad], input[MinRankLoad]);
    result[MaxRankLoad]      = std::max(result[MaxRankLoad], input[MaxRankLoad]);
    result[MinDepth]         = std::min(result[MinDepth], input[MinDepth]);
    result[MaxDepth]         = std::max(result[MaxDepth], input[MaxDepth]);
    result[MinStationarySweeps] =
      encodeSigned(std::min(decodeSigned(result[MinStationarySweeps]), decodeSigned(input[MinStationarySweeps])));
    result[OrUniformGridComplete] = std::max(result[OrUniformGridComplete], input[OrUniformGridComplete]);

    const auto inputMinimumMeshWidth  = fromBits(input[MinMeshWidthBits]);
    const auto resultMinimumMeshWidth = fromBits(result[MinMeshWidthBits]);
    if (inputMinimumMeshWidth < resultMinimumMeshWidth) {
      result[MinMeshWidthBits] = input[MinMeshWidthBits];
    }
  }

  /**
   * The record mixes sums, minima, maxima and one argument maximum, so MPI reduces it through
   * a routine of ours rather than a predefined operation. One record is one element of the
   * reduction, so the loop over the length is what lets MPI hand over several of them at once.
   */
  void reduceRecords(void* incoming, void* accumulator, int* length, MPI_Datatype*) {
    const auto* const input  = static_cast<const std::uint64_t*>(incoming);
    auto* const       result = static_cast<std::uint64_t*>(accumulator);
    for (int record = 0; record < *length; record++) {
      const auto offset = static_cast<std::size_t>(record) * toolbox::loadbalancing::Statistics::NumberOfWords;
      reduceRecord(input + offset, result + offset);
    }
  }

  MPI_Datatype recordType = MPI_DATATYPE_NULL;
  MPI_Op       recordOp   = MPI_OP_NULL;
} // namespace

toolbox::loadbalancing::Statistics::Statistics() :
  _isViewConsistent(false),
  _hasPublishedGlobalView(false),
  _localNumberOfInnerUnrefinedCells(0),
  _globalNumberOfInnerUnrefinedCells(0),
  _localNumberOfInnerRefinedCells(0),
  _globalNumberOfInnerRefinedCells(0),
  _localNumberOfOuterUnrefinedCells(0),
  _globalNumberOfOuterUnrefinedCells(0),
  _localNumberOfVertices(0),
  _globalNumberOfVertices(0),
  _localNumberOfHangingVertices(0),
  _globalNumberOfHangingVertices(0),
  _localNumberOfTrees(0),
  _globalNumberOfTrees(1),
  _globalNumberOfRanksWithAtLeastOneTree(1),
  _localNumberOfSplits(0),
  _globalNumberOfSplits(0),
  _localMinTreeLoad(0),
  _localMaxTreeLoad(0),
  _globalMinTreeLoad(0),
  _globalMaxTreeLoad(0),
  _globalRankWithMaxTreeLoad(0),
  _localMinTreeOuterLoad(0),
  _localMaxTreeOuterLoad(0),
  _globalMinTreeOuterLoad(0),
  _globalMaxTreeOuterLoad(0),
  _globalMinRankLoad(0),
  _globalMaxRankLoad(0),
  _localMemoryUsageInBytes(0),
  _globalMemoryUsageInBytes(0),
  _localProcessMemoryUsageInBytes(0),
  _globalProcessMemoryUsageInBytes(0),
  _localTotalMemoryInBytes(0),
  _localFreeMemoryInBytes(0),
  _globalTotalMemoryInBytes(0),
  _globalFreeMemoryInBytes(0),
  _localMinDepth(std::numeric_limits<int>::max()),
  _localMaxDepth(0),
  _globalMinDepth(std::numeric_limits<int>::max()),
  _globalMaxDepth(0),
  _localMinH(std::numeric_limits<double>::max()),
  _globalMinH(std::numeric_limits<double>::max()),
  _localNumberOfStationarySweeps(0),
  _globalNumberOfStationarySweeps(0),
  _localUniformGridComplete(false),
  _globalUniformGridComplete(false) {}

toolbox::loadbalancing::Statistics::~Statistics() { shutdownMPI(); }

/**
 * Builds the record type and the reduction routine
 *
 * Both are local to the rank, so a caller may build them whenever it first needs them.
 * Repeated calls do nothing.
 */
void toolbox::loadbalancing::Statistics::initialiseExchange() {
  if (recordType == MPI_DATATYPE_NULL) {
    MPI_Type_contiguous(NumberOfWords, MPI_UINT64_T, &recordType);
    MPI_Type_commit(&recordType);
  }
  if (recordOp == MPI_OP_NULL) {
    // The record combines pairwise in any order, so MPI may reduce it along whichever tree
    // it prefers.
    MPI_Op_create(&reduceRecords, 1, &recordOp);
  }
}

void toolbox::loadbalancing::Statistics::shutdownMPI() {
  if (recordOp != MPI_OP_NULL) {
    MPI_Op_free(&recordOp);
    recordOp = MPI_OP_NULL;
  }
  if (recordType != MPI_DATATYPE_NULL) {
    MPI_Type_free(&recordType);
    recordType = MPI_DATATYPE_NULL;
  }
}

/**
 * Unpacks the record the preceding sweep reduced
 *
 * The reduction has already run, so this is pure local work and a caller that only needs the
 * current view never enters MPI.
 */
void toolbox::loadbalancing::Statistics::publishGlobalView() {
  if (not _hasPendingRecord) {
    return;
  }
  _hasPendingRecord = false;

  const auto* const reducedStatistics    = _reductionRecord.data();
  _globalNumberOfInnerUnrefinedCells     = reducedStatistics[SumInnerUnrefinedCells];
  _globalNumberOfInnerRefinedCells       = reducedStatistics[SumInnerRefinedCells];
  _globalNumberOfOuterUnrefinedCells     = reducedStatistics[SumOuterUnrefinedCells];
  _globalNumberOfVertices                = reducedStatistics[SumVertices];
  _globalNumberOfHangingVertices         = reducedStatistics[SumHangingVertices];
  _globalNumberOfSplits                  = static_cast<int>(reducedStatistics[SumSplits]);
  _globalNumberOfTrees                   = static_cast<int>(reducedStatistics[SumTrees]);
  _globalNumberOfRanksWithAtLeastOneTree = static_cast<int>(reducedStatistics[SumRanksWithTrees]);
  _globalMinTreeLoad                     = static_cast<int>(reducedStatistics[MinTreeLoad]);
  _globalMaxTreeLoad                     = static_cast<int>(reducedStatistics[MaxTreeLoad]);
  _globalRankWithMaxTreeLoad             = static_cast<int>(reducedStatistics[MaxTreeLoadOwnerRank]);
  _globalMinTreeOuterLoad                = static_cast<int>(reducedStatistics[MinTreeOuterLoad]);
  _globalMaxTreeOuterLoad                = static_cast<int>(reducedStatistics[MaxTreeOuterLoad]);
  _globalMinRankLoad                     = reducedStatistics[MinRankLoad];
  _globalMaxRankLoad                     = reducedStatistics[MaxRankLoad];
  _globalMemoryUsageInBytes              = reducedStatistics[SumMemoryUsageInBytes];
  _globalProcessMemoryUsageInBytes       = reducedStatistics[SumProcessMemoryUsageInBytes];
  _globalTotalMemoryInBytes              = reducedStatistics[SumNodeTotalMemoryInBytes];
  _globalFreeMemoryInBytes               = reducedStatistics[SumNodeFreeMemoryInBytes];
  _globalMinDepth                        = static_cast<int>(reducedStatistics[MinDepth]);
  _globalMaxDepth                        = static_cast<int>(reducedStatistics[MaxDepth]);
  _globalMinH                            = fromBits(reducedStatistics[MinMeshWidthBits]);
  _globalNumberOfStationarySweeps        = static_cast<int>(std::min(
    decodeSigned(reducedStatistics[MinStationarySweeps]),
    static_cast<std::int64_t>(std::numeric_limits<int>::max())
  ));
  _globalUniformGridComplete             = reducedStatistics[OrUniformGridComplete] != 0;
  _hasPublishedGlobalView                = true;

  // A lagging reduction can report fewer cells than this rank already owns.
  if (_globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells) {
    logDebug(
      "Local cell count " << _localNumberOfInnerUnrefinedCells << " exceeds global cell count "
                          << _globalNumberOfInnerUnrefinedCells << "."
    );
    _isViewConsistent                  = false;
    _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCells;
    _globalMinDepth                    = _localMinDepth;
    _globalMaxDepth                    = _localMaxDepth;
  } else {
    _isViewConsistent = true;
  }
}

/** Controllers and summaries call this before they require the latest global snapshot. */
void toolbox::loadbalancing::Statistics::synchroniseGlobalView() { publishGlobalView(); }

/** Starts a decomposition epoch from the completed cell and tree snapshot while clearing its split-specific state. */
void toolbox::loadbalancing::Statistics::resetInitialDecompositionState() {
  publishGlobalView();
  _localNumberOfSplits       = 0;
  _globalNumberOfSplits      = 0;
  _localUniformGridComplete  = false;
  _globalUniformGridComplete = false;
}

/**
 * Refreshes local statistics and consumes the preceding global exchange. The next exchange starts after the load
 * balancer decision so forks issued in the current sweep enter the same snapshot. A rank can own enough trees for
 * their combined cell and vertex counts to exceed int, so each per-tree counter is widened before addition.
 */
void toolbox::loadbalancing::Statistics::updateGlobalView() {
  const auto& spacetreeSet = peano4::parallel::SpacetreeSet::getInstance();
  const auto& statistics   = spacetreeSet.getGridStatistics();
  const auto& spacetrees   = spacetreeSet.getLocalSpacetrees();

  _localNumberOfTrees = static_cast<int>(spacetrees.size());

  _localNumberOfInnerUnrefinedCells = 0;
  _localNumberOfInnerRefinedCells   = 0;
  _localNumberOfOuterUnrefinedCells = 0;
  _localNumberOfVertices            = 0;
  _localNumberOfHangingVertices     = 0;
  _localMemoryUsageInBytes          = 0;

  int minimumTreeLoad      = std::numeric_limits<int>::max();
  int maximumTreeLoad      = 0;
  int minimumTreeOuterLoad = std::numeric_limits<int>::max();
  int maximumTreeOuterLoad = 0;
  for (const auto treeId : spacetrees) {
    const auto& treeStatistics = spacetreeSet.getGridStatistics(treeId);
    const auto  treeLoad       = treeStatistics.numberOfLocalUnrefinedCells;
    const auto  treeOuterLoad  = treeStatistics.numberOfRemoteUnrefinedCells;

    _localNumberOfInnerUnrefinedCells += static_cast<std::uint64_t>(treeLoad);
    _localNumberOfInnerRefinedCells += static_cast<std::uint64_t>(treeStatistics.numberOfLocalRefinedCells);
    _localNumberOfOuterUnrefinedCells += static_cast<std::uint64_t>(treeOuterLoad);
    _localNumberOfVertices += static_cast<std::uint64_t>(treeStatistics.numberOfVertices);
    _localNumberOfHangingVertices += static_cast<std::uint64_t>(treeStatistics.numberOfHangingVertices);
    _localMemoryUsageInBytes += static_cast<std::uint64_t>(treeStatistics.getMemoryUsageInBytes());

    minimumTreeLoad      = std::min(minimumTreeLoad, treeLoad);
    maximumTreeLoad      = std::max(maximumTreeLoad, treeLoad);
    minimumTreeOuterLoad = std::min(minimumTreeOuterLoad, treeOuterLoad);
    maximumTreeOuterLoad = std::max(maximumTreeOuterLoad, treeOuterLoad);
  }
  _localMinTreeLoad      = minimumTreeLoad;
  _localMaxTreeLoad      = maximumTreeLoad;
  _localMinTreeOuterLoad = minimumTreeOuterLoad;
  _localMaxTreeOuterLoad = maximumTreeOuterLoad;

  _localProcessMemoryUsageInBytes = tarch::getMemoryUsageInBytes();
  _localTotalMemoryInBytes        = tarch::getTotalMemoryInBytes();
  _localFreeMemoryInBytes         = tarch::getFreeMemoryInBytes();

  _localMinDepth = statistics.minDepth;
  _localMaxDepth = statistics.maxDepth;

  _localMinH                     = tarch::la::max(statistics.minH);
  _localNumberOfStationarySweeps = statistics.numberOfStationarySweeps;

  bool publishLocalAsGlobal = true;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    publishLocalAsGlobal = not _hasPublishedGlobalView;
    if (_hasPendingRecord) {
      publishGlobalView();
      publishLocalAsGlobal = false;
    }
  }

  if (publishLocalAsGlobal) {
    _isViewConsistent = true;

    _globalNumberOfInnerUnrefinedCells     = _localNumberOfInnerUnrefinedCells;
    _globalNumberOfInnerRefinedCells       = _localNumberOfInnerRefinedCells;
    _globalNumberOfOuterUnrefinedCells     = _localNumberOfOuterUnrefinedCells;
    _globalNumberOfVertices                = _localNumberOfVertices;
    _globalNumberOfHangingVertices         = _localNumberOfHangingVertices;
    _globalNumberOfSplits                  = _localNumberOfSplits;
    _globalNumberOfTrees                   = _localNumberOfTrees;
    _globalNumberOfRanksWithAtLeastOneTree = (_localNumberOfTrees > 0) ? 1 : 0;
    _globalMinTreeLoad                     = _localMinTreeLoad;
    _globalMaxTreeLoad                     = _localMaxTreeLoad;
    _globalRankWithMaxTreeLoad             = tarch::mpi::Rank::getInstance().getRank();
    _globalMinTreeOuterLoad                = _localMinTreeOuterLoad;
    _globalMaxTreeOuterLoad                = _localMaxTreeOuterLoad;
    _globalMinRankLoad                     = _localNumberOfInnerUnrefinedCells;
    _globalMaxRankLoad                     = _localNumberOfInnerUnrefinedCells;
    _globalMemoryUsageInBytes              = _localMemoryUsageInBytes;
    _globalProcessMemoryUsageInBytes       = _localProcessMemoryUsageInBytes;
    _globalTotalMemoryInBytes              = _localTotalMemoryInBytes;
    _globalFreeMemoryInBytes               = _localFreeMemoryInBytes;
    _globalMinDepth                        = _localMinDepth;
    _globalMaxDepth                        = _localMaxDepth;
    _globalMinH                            = _localMinH;
    _globalNumberOfStationarySweeps        = _localNumberOfStationarySweeps;
    _globalUniformGridComplete             = _localUniformGridComplete;
  }

  _localNumberOfSplits = 0;
}

/**
 * Reduces this sweep's numbers, which the next sweep publishes
 *
 * The load-balancing decision has already run, so the record carries the forks this sweep
 * issued together with the statistics that led to them. Empty ranks contribute neutral minima
 * so they do not pin load, depth, or stationary-sweep results.
 */
void toolbox::loadbalancing::Statistics::startGlobalViewExchange() {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    _globalNumberOfSplits = _localNumberOfSplits;
    return;
  }

  initialiseExchange();
  assertion(not _hasPendingRecord);

  auto&      localStatistics = _reductionRecord;
  const auto isEmptyRank     = _localNumberOfTrees == 0;

  localStatistics[SumInnerUnrefinedCells] = _localNumberOfInnerUnrefinedCells;
  localStatistics[SumInnerRefinedCells]   = _localNumberOfInnerRefinedCells;
  localStatistics[SumOuterUnrefinedCells] = _localNumberOfOuterUnrefinedCells;
  localStatistics[SumVertices]            = _localNumberOfVertices;
  localStatistics[SumHangingVertices]     = _localNumberOfHangingVertices;
  localStatistics[SumSplits]              = static_cast<std::uint64_t>(_localNumberOfSplits);
  localStatistics[SumTrees]               = static_cast<std::uint64_t>(_localNumberOfTrees);
  localStatistics[SumRanksWithTrees]      = isEmptyRank ? 0 : 1;
  localStatistics[MinTreeLoad] =
    isEmptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinTreeLoad);
  localStatistics[MaxTreeLoad]          = static_cast<std::uint64_t>(_localMaxTreeLoad);
  localStatistics[MaxTreeLoadOwnerRank] = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
  localStatistics[MinTreeOuterLoad] =
    isEmptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinTreeOuterLoad);
  localStatistics[MaxTreeOuterLoad] = static_cast<std::uint64_t>(_localMaxTreeOuterLoad);
  localStatistics[MinRankLoad] =
    isEmptyRank ? std::numeric_limits<std::uint64_t>::max() : _localNumberOfInnerUnrefinedCells;
  localStatistics[MaxRankLoad]                  = _localNumberOfInnerUnrefinedCells;
  localStatistics[SumMemoryUsageInBytes]        = _localMemoryUsageInBytes;
  localStatistics[SumProcessMemoryUsageInBytes] = _localProcessMemoryUsageInBytes;
  // One contributor per node prevents shared host memory from being counted once per rank.
  const auto isNodeMaster                    = tarch::mpi::Rank::getInstance().isNodeMaster();
  localStatistics[SumNodeTotalMemoryInBytes] = isNodeMaster ? _localTotalMemoryInBytes : 0;
  localStatistics[SumNodeFreeMemoryInBytes]  = isNodeMaster ? _localFreeMemoryInBytes : 0;
  localStatistics[MinDepth] =
    isEmptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinDepth);
  localStatistics[MaxDepth]         = static_cast<std::uint64_t>(_localMaxDepth);
  localStatistics[MinMeshWidthBits] = toBits(_localMinH);
  // Negative values mark settling trees, so the unsigned record decodes them before the minimum comparison.
  localStatistics[MinStationarySweeps] = encodeSigned(
    isEmptyRank ? std::numeric_limits<std::int64_t>::max() : static_cast<std::int64_t>(_localNumberOfStationarySweeps)
  );
  localStatistics[OrUniformGridComplete] = _localUniformGridComplete ? 1 : 0;

  tarch::mpi::allreduce(
    MPI_IN_PLACE,
    localStatistics.data(),
    1,
    recordType,
    recordOp,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  _hasPendingRecord = true;
}

bool toolbox::loadbalancing::Statistics::hasConsistentViewOfWorld() const { return _isViewConsistent; }

std::uint64_t toolbox::loadbalancing::Statistics::getLocalNumberOfInnerUnrefinedCells() const {
  return _localNumberOfInnerUnrefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalNumberOfInnerUnrefinedCells() const {
  return _globalNumberOfInnerUnrefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getLocalNumberOfInnerRefinedCells() const {
  return _localNumberOfInnerRefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalNumberOfInnerRefinedCells() const {
  return _globalNumberOfInnerRefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getLocalNumberOfOuterUnrefinedCells() const {
  return _localNumberOfOuterUnrefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalNumberOfOuterUnrefinedCells() const {
  return _globalNumberOfOuterUnrefinedCells;
}

std::uint64_t toolbox::loadbalancing::Statistics::getLocalNumberOfVertices() const { return _localNumberOfVertices; }

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalNumberOfVertices() const { return _globalNumberOfVertices; }

std::uint64_t toolbox::loadbalancing::Statistics::getLocalNumberOfHangingVertices() const {
  return _localNumberOfHangingVertices;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalNumberOfHangingVertices() const {
  return _globalNumberOfHangingVertices;
}

int toolbox::loadbalancing::Statistics::getLocalNumberOfTrees() const { return _localNumberOfTrees; }

int toolbox::loadbalancing::Statistics::getGlobalNumberOfTrees() const { return _globalNumberOfTrees; }

void toolbox::loadbalancing::Statistics::incLocalNumberOfSplits(const int delta) {
  assertion(delta > 0);
  _localNumberOfSplits += delta;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfSplits() const { return _globalNumberOfSplits; }

int toolbox::loadbalancing::Statistics::getGlobalNumberOfRanksWithAtLeastOneTree() const {
  return _globalNumberOfRanksWithAtLeastOneTree;
}

int toolbox::loadbalancing::Statistics::getGlobalMinTreeLoad() const { return _globalMinTreeLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxTreeLoad() const { return _globalMaxTreeLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMinTreeOuterLoad() const { return _globalMinTreeOuterLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxTreeOuterLoad() const { return _globalMaxTreeOuterLoad; }

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalMinRankLoad() const { return _globalMinRankLoad; }

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalMaxRankLoad() const { return _globalMaxRankLoad; }

int toolbox::loadbalancing::Statistics::getGlobalRankWithMaxTreeLoad() const { return _globalRankWithMaxTreeLoad; }

std::uint64_t toolbox::loadbalancing::Statistics::getLocalMemoryUsageInBytes() const {
  return _localMemoryUsageInBytes;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalMemoryUsageInBytes() const {
  return _globalMemoryUsageInBytes;
}

std::uint64_t toolbox::loadbalancing::Statistics::getLocalProcessMemoryUsageInBytes() const {
  return _localProcessMemoryUsageInBytes;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalProcessMemoryUsageInBytes() const {
  return _globalProcessMemoryUsageInBytes;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalTotalMemoryInBytes() const {
  return _globalTotalMemoryInBytes;
}

std::uint64_t toolbox::loadbalancing::Statistics::getGlobalFreeMemoryInBytes() const {
  return _globalFreeMemoryInBytes;
}

int toolbox::loadbalancing::Statistics::getLocalMinDepth() const { return _localMinDepth; }

int toolbox::loadbalancing::Statistics::getGlobalMinDepth() const { return _globalMinDepth; }

int toolbox::loadbalancing::Statistics::getLocalMaxDepth() const { return _localMaxDepth; }

int toolbox::loadbalancing::Statistics::getGlobalMaxDepth() const { return _globalMaxDepth; }

/** Tree-boundary hanging vertices also occur on uniform decompositions, so depth range identifies AMR. */
bool toolbox::loadbalancing::Statistics::hasAdaptiveMesh() const {
  return _globalNumberOfInnerUnrefinedCells > 0 and _globalMinDepth != _globalMaxDepth;
}

double toolbox::loadbalancing::Statistics::getLocalMinH() const { return _localMinH; }

double toolbox::loadbalancing::Statistics::getGlobalMinH() const { return _globalMinH; }

int toolbox::loadbalancing::Statistics::getLocalNumberOfStationarySweeps() const {
  return _localNumberOfStationarySweeps;
}

void toolbox::loadbalancing::Statistics::notifyUniformGridComplete() { _localUniformGridComplete = true; }

bool toolbox::loadbalancing::Statistics::hasUniformGridCompleted() const {
  return _globalUniformGridComplete or _localUniformGridComplete;
}

bool toolbox::loadbalancing::Statistics::hasGlobalUniformGridCompleted() const { return _globalUniformGridComplete; }

int toolbox::loadbalancing::Statistics::getGlobalNumberOfStationarySweeps() const {
  return _globalNumberOfStationarySweeps;
}
