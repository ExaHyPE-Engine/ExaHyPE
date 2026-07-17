// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/services/ServiceRepository.h"

#ifdef PEANO_WITH_MPI
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

  MPI_Op       reductionOperation = MPI_OP_NULL;
  MPI_Datatype recordDatatype     = MPI_DATATYPE_NULL;
  bool         isReady            = false;

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

  void
  reduceRecords(void* const inputBuffer, void* const resultBuffer, int* const numberOfRecords, MPI_Datatype* const) {
    const auto* const input  = static_cast<const std::uint64_t*>(inputBuffer);
    auto* const       result = static_cast<std::uint64_t*>(resultBuffer);
    for (int recordIndex = 0; recordIndex < *numberOfRecords; ++recordIndex) {
      const auto offset = recordIndex * toolbox::loadbalancing::Statistics::NumberOfWords;
      reduceRecord(input + offset, result + offset);
    }
  }

  /**
   * Registers the packed words as one MPI datatype, so all statistics share one collective and one ordering point.
   * MPI therefore passes a record count to reduceRecords(); treating it as a word count would overrun the buffers.
   */
  void initialiseReduction() {
    if (isReady) {
      return;
    }

    int mpiIsInitialised = 0;
    MPI_Initialized(&mpiIsInitialised);
    if (not mpiIsInitialised) {
      return;
    }

    MPI_Type_contiguous(toolbox::loadbalancing::Statistics::NumberOfWords, MPI_UINT64_T, &recordDatatype);
    MPI_Type_commit(&recordDatatype);
    MPI_Op_create(&reduceRecords, 1, &reductionOperation);
    isReady = true;
  }

  void shutdownReduction() {
    if (not isReady) {
      return;
    }

    int mpiIsInitialised = 0;
    MPI_Initialized(&mpiIsInitialised);
    if (not mpiIsInitialised) {
      return;
    }

    int mpiIsFinalised = 0;
    MPI_Finalized(&mpiIsFinalised);
    if (mpiIsFinalised) {
      return;
    }

    MPI_Op_free(&reductionOperation);
    MPI_Type_free(&recordDatatype);
    reductionOperation = MPI_OP_NULL;
    recordDatatype     = MPI_DATATYPE_NULL;
    isReady            = false;
  }
} // namespace
#endif

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
  _globalUniformGridComplete(false)
#ifdef PEANO_WITH_MPI
  ,
  _globalExchangeRequest(MPI_REQUEST_NULL),
  _globalExchangeInFlight(false)
#endif
{
}

toolbox::loadbalancing::Statistics::~Statistics() {
  completePendingReduction();
  shutdownMPI();
}

/** A live request retains its communicator and buffers, so the runner completes it before MPI finalisation. */
void toolbox::loadbalancing::Statistics::completePendingReduction() {
#ifdef PEANO_WITH_MPI
  int mpiIsInitialised = 0;
  MPI_Initialized(&mpiIsInitialised);
  if (mpiIsInitialised) {
    int mpiIsFinalised = 0;
    MPI_Finalized(&mpiIsFinalised);
    if (not mpiIsFinalised and _globalExchangeInFlight and _globalExchangeRequest != MPI_REQUEST_NULL) {
      tarch::mpi::wait(&_globalExchangeRequest);
      _globalExchangeRequest  = MPI_REQUEST_NULL;
      _globalExchangeInFlight = false;
    }
  }
#endif
}

void toolbox::loadbalancing::Statistics::shutdownMPI() {
#ifdef PEANO_WITH_MPI
  shutdownReduction();
#endif
}

/**
 * Publishes the preceding asynchronous reduction. The progress callback services other messages while the
 * collective completes, which prevents this wait from starving communication needed by another rank.
 */
void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
#ifdef PEANO_WITH_MPI
  if (not _globalExchangeInFlight) {
    return;
  }

  tarch::mpi::wait(&_globalExchangeRequest, []() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

  _globalExchangeRequest  = MPI_REQUEST_NULL;
  _globalExchangeInFlight = false;

  const auto& reducedStatistics          = _globalExchangeRecv;
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
#endif
}

/** Controllers and summaries call this before they require the latest global snapshot. */
void toolbox::loadbalancing::Statistics::synchroniseGlobalView() { waitForGlobalDataExchange(); }

/** Starts a decomposition epoch from the completed cell and tree snapshot while clearing its split-specific state. */
void toolbox::loadbalancing::Statistics::resetInitialDecompositionState() {
  waitForGlobalDataExchange();
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
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    initialiseReduction();
    if (isReady) {
      publishLocalAsGlobal = not _hasPublishedGlobalView;
      if (_globalExchangeInFlight) {
        waitForGlobalDataExchange();
        publishLocalAsGlobal = false;
      }
    }
  }
#endif

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
 * Starts one packed non-blocking reduction for the current sweep. Empty ranks contribute neutral minima so they do
 * not pin load, depth, or stationary-sweep results.
 */
void toolbox::loadbalancing::Statistics::startGlobalViewExchange() {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    initialiseReduction();
    if (isReady) {
      assertion(not _globalExchangeInFlight);
      auto&      localStatistics = _globalExchangeSend;
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
        isEmptyRank ? std::numeric_limits<std::int64_t>::max()
                    : static_cast<std::int64_t>(_localNumberOfStationarySweeps)
      );
      localStatistics[OrUniformGridComplete] = _localUniformGridComplete ? 1 : 0;

      tarch::mpi::iallreduce(
        _globalExchangeSend.data(),
        _globalExchangeRecv.data(),
        1,
        recordDatatype,
        reductionOperation,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &_globalExchangeRequest
      );
      _globalExchangeInFlight = true;
      return;
    }
  }
#endif
  _globalNumberOfSplits = _localNumberOfSplits;
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
