// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/services/ServiceRepository.h"

#ifdef PEANO_WITH_MPI
namespace {
  enum WordIndex : int {
    SumInnerCells = 0,
    SumOuterCells,
    SumVertices,
    SumHangingVertices,
    SumSplits,
    SumTrees,
    SumRanksWithTrees,
    MinTreeLoad,
    MaxTreeLoad,
    MaxTreeLoadOwnerRank,
    SumMemBytes,
    SumProcessMemBytes,
    MinDepth,
    MaxDepth,
    MinHBits,
    MinStationarySweeps,
    MinTreeOuterLoad,
    MaxTreeOuterLoad,
    MinRankLoad,
    MaxRankLoad,
    SumNodeTotalMemBytes,
    SumNodeFreeMemBytes
  };

  inline std::uint64_t toBits(double x) {
    static_assert(sizeof(double) == sizeof(std::uint64_t), "double must be 64-bit");
    std::uint64_t u = 0;
    std::memcpy(&u, &x, sizeof(u));
    return u;
  }

  inline double fromBits(std::uint64_t u) {
    double x = 0.0;
    std::memcpy(&x, &u, sizeof(x));
    return x;
  }

  MPI_Op gReduceOp = MPI_OP_NULL;
  bool   gReady    = false;

  void reduceRecord(void* invec, void* inoutvec, int* len, MPI_Datatype*) {
    auto* in    = static_cast<std::uint64_t*>(invec);
    auto* inout = static_cast<std::uint64_t*>(inoutvec);

    [[maybe_unused]] const int n = *len;
    assertion(
      n == toolbox::loadbalancing::Statistics::NumberOfWords,
      n,
      toolbox::loadbalancing::Statistics::NumberOfWords
    );

    // SUM fields
    inout[SumInnerCells] += in[SumInnerCells];
    inout[SumOuterCells] += in[SumOuterCells];
    inout[SumVertices] += in[SumVertices];
    inout[SumHangingVertices] += in[SumHangingVertices];
    inout[SumSplits] += in[SumSplits];
    inout[SumTrees] += in[SumTrees];
    inout[SumRanksWithTrees] += in[SumRanksWithTrees];
    inout[SumMemBytes] += in[SumMemBytes];
    inout[SumProcessMemBytes] += in[SumProcessMemBytes];
    inout[SumNodeTotalMemBytes] += in[SumNodeTotalMemBytes];
    inout[SumNodeFreeMemBytes] += in[SumNodeFreeMemBytes];

    // MIN/MAX fields
    inout[MinTreeLoad] = std::min(inout[MinTreeLoad], in[MinTreeLoad]);
    if (in[MaxTreeLoad] > inout[MaxTreeLoad]) {
      inout[MaxTreeLoad]          = in[MaxTreeLoad];
      inout[MaxTreeLoadOwnerRank] = in[MaxTreeLoadOwnerRank];
    } else if (in[MaxTreeLoad] == inout[MaxTreeLoad]) {
      inout[MaxTreeLoadOwnerRank] = std::min(inout[MaxTreeLoadOwnerRank], in[MaxTreeLoadOwnerRank]);
    }
    inout[MinTreeOuterLoad] = std::min(inout[MinTreeOuterLoad], in[MinTreeOuterLoad]);
    inout[MaxTreeOuterLoad] = std::max(inout[MaxTreeOuterLoad], in[MaxTreeOuterLoad]);
    inout[MinRankLoad]      = std::min(inout[MinRankLoad], in[MinRankLoad]);
    inout[MaxRankLoad]      = std::max(inout[MaxRankLoad], in[MaxRankLoad]);
    inout[MinDepth]         = std::min(inout[MinDepth], in[MinDepth]);
    inout[MaxDepth]         = std::max(inout[MaxDepth], in[MaxDepth]);

    // MIN for stationary sweeps
    inout[MinStationarySweeps] = std::min(inout[MinStationarySweeps], in[MinStationarySweeps]);

    // MIN for double stored as bits
    const double ha = fromBits(in[MinHBits]);
    const double hb = fromBits(inout[MinHBits]);
    if (ha < hb) {
      inout[MinHBits] = in[MinHBits];
    }
  }

  void init() {
    if (gReady) {
      return;
    }

    int mpiInitialised = 0;
    MPI_Initialized(&mpiInitialised);
    if (not mpiInitialised) {
      return;
    }

    MPI_Op_create(&reduceRecord, /*commute=*/1, &gReduceOp);

    gReady = true;
  }

  void shutdownReduction() {
    if (not gReady or gReduceOp == MPI_OP_NULL) {
      return;
    }

    int mpiInitialised = 0;
    MPI_Initialized(&mpiInitialised);
    if (not mpiInitialised) {
      return;
    }

    int mpiFinalised = 0;
    MPI_Finalized(&mpiFinalised);
    if (mpiFinalised) {
      return;
    }

    MPI_Op_free(&gReduceOp);
    gReduceOp = MPI_OP_NULL;
    gReady    = false;
  }
} // namespace
#endif

toolbox::loadbalancing::Statistics::Statistics() :
  _isViewConsistent(false),
  _localNumberOfInnerUnrefinedCells(0),
  _globalNumberOfInnerUnrefinedCells(0),
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
  _numberOfStateUpdatesWithoutAnySplit(0),
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
  _globalNumberOfStationarySweeps(0)
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

void toolbox::loadbalancing::Statistics::completePendingReduction() {
#ifdef PEANO_WITH_MPI
  // Complete a still-in-flight non-blocking reduction so no request keeps a reference to the
  // communicator at MPI_Finalize. LoadBalancer::shutdown() calls this on every rank while MPI is still
  // up; relying on the destructor alone is too late (it runs after MPI_Finalize for the global LB).
  // Skipped once MPI is finalised. Per instance, so it must not be static.
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised) {
    int mpiFinalised = 0;
    MPI_Finalized(&mpiFinalised);
    if (not mpiFinalised and _globalExchangeInFlight and _globalExchangeRequest != MPI_REQUEST_NULL) {
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

  // Unpack the reduced record into the public/global fields
  const auto& g                          = _globalExchangeRecv;
  _globalNumberOfInnerUnrefinedCells     = static_cast<int>(g[SumInnerCells]);
  _globalNumberOfOuterUnrefinedCells     = static_cast<int>(g[SumOuterCells]);
  _globalNumberOfVertices                = static_cast<int>(g[SumVertices]);
  _globalNumberOfHangingVertices         = static_cast<int>(g[SumHangingVertices]);
  _globalNumberOfSplits                  = static_cast<int>(g[SumSplits]);
  _globalNumberOfTrees                   = static_cast<int>(g[SumTrees]);
  _globalNumberOfRanksWithAtLeastOneTree = static_cast<int>(g[SumRanksWithTrees]);
  _globalMinTreeLoad                     = static_cast<int>(g[MinTreeLoad]);
  _globalMaxTreeLoad                     = static_cast<int>(g[MaxTreeLoad]);
  _globalRankWithMaxTreeLoad             = static_cast<int>(g[MaxTreeLoadOwnerRank]);
  _globalMinTreeOuterLoad                = static_cast<int>(g[MinTreeOuterLoad]);
  _globalMaxTreeOuterLoad                = static_cast<int>(g[MaxTreeOuterLoad]);
  _globalMinRankLoad                     = static_cast<int>(g[MinRankLoad]);
  _globalMaxRankLoad                     = static_cast<int>(g[MaxRankLoad]);
  _globalMemoryUsageInBytes              = static_cast<std::uint64_t>(g[SumMemBytes]);
  _globalProcessMemoryUsageInBytes       = static_cast<std::uint64_t>(g[SumProcessMemBytes]);
  _globalTotalMemoryInBytes              = static_cast<std::uint64_t>(g[SumNodeTotalMemBytes]);
  _globalFreeMemoryInBytes               = static_cast<std::uint64_t>(g[SumNodeFreeMemBytes]);
  _globalMinDepth                        = static_cast<int>(g[MinDepth]);
  _globalMaxDepth                        = static_cast<int>(g[MaxDepth]);
  _globalMinH                            = fromBits(g[MinHBits]);
  _globalNumberOfStationarySweeps        = static_cast<int>(g[MinStationarySweeps]);

  // If the global representation (which lags behind by one non-blocking MPI step) thinks there are
  // fewer cells than we have locally, the global view is inconsistent/outdated.
  if (_globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells) {
    logDebug(
      "Local cell count " << _localNumberOfInnerUnrefinedCells << " exceeds global cell count "
                          << _globalNumberOfInnerUnrefinedCells
                          << ". Statistics view is inconsistent or outdated; this usually happens while a "
                             "forking tree still has pending refinement events after spawning cells."
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

void toolbox::loadbalancing::Statistics::updateGlobalView() {
  const auto& statistics = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  const auto& spacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  _localNumberOfTrees = static_cast<int>(spacetrees.size());

  _localNumberOfInnerUnrefinedCells = statistics.numberOfLocalUnrefinedCells;
  _localNumberOfOuterUnrefinedCells = statistics.numberOfRemoteUnrefinedCells;

  _localNumberOfVertices        = statistics.numberOfVertices;
  _localNumberOfHangingVertices = statistics.numberOfHangingVertices;

  _localMemoryUsageInBytes = statistics.getMemoryUsageInBytes();
  _localProcessMemoryUsageInBytes =
    static_cast<std::uint64_t>(tarch::getMemoryUsage(tarch::MemoryUsageFormat::KiB) * 1024.0);
  _localTotalMemoryInBytes = static_cast<std::uint64_t>(tarch::getTotalMemory(tarch::MemoryUsageFormat::KiB) * 1024.0);
  _localFreeMemoryInBytes  = static_cast<std::uint64_t>(tarch::getFreeMemory(tarch::MemoryUsageFormat::KiB) * 1024.0);

  _localMinDepth = statistics.minDepth;
  _localMaxDepth = statistics.maxDepth;

  _localMinH                     = tarch::la::max(statistics.minH);
  _localNumberOfStationarySweeps = statistics.numberOfStationarySweeps;

  // Compute local min/max tree load based on hosted trees
  if (spacetrees.empty()) {
    _localMinTreeLoad      = std::numeric_limits<int>::max();
    _localMaxTreeLoad      = 0;
    _localMinTreeOuterLoad = std::numeric_limits<int>::max();
    _localMaxTreeOuterLoad = 0;
  } else {
    int localMin      = std::numeric_limits<int>::max();
    int localMax      = 0;
    int localMinOuter = std::numeric_limits<int>::max();
    int localMaxOuter = 0;
    for (auto tree : spacetrees) {
      const auto& s     = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(tree);
      const int   load  = s.numberOfLocalUnrefinedCells;
      const int   outer = s.numberOfRemoteUnrefinedCells;
      localMin          = std::min(localMin, load);
      localMax          = std::max(localMax, load);
      localMinOuter     = std::min(localMinOuter, outer);
      localMaxOuter     = std::max(localMaxOuter, outer);
    }
    _localMinTreeLoad      = localMin;
    _localMaxTreeLoad      = localMax;
    _localMinTreeOuterLoad = localMinOuter;
    _localMaxTreeOuterLoad = localMaxOuter;
  }
  bool publishLocalAsGlobal = true;
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    init();
    if (gReady) {
      // Complete the reduction launched last sweep and publish its (one-sweep-stale) result before
      // we overwrite the receive buffer with this sweep's reduction.
      if (_globalExchangeInFlight) {
        waitForGlobalDataExchange();
        publishLocalAsGlobal = false;
      }

      auto& local               = _globalExchangeSend;
      local[SumInnerCells]      = static_cast<std::uint64_t>(_localNumberOfInnerUnrefinedCells);
      local[SumOuterCells]      = static_cast<std::uint64_t>(_localNumberOfOuterUnrefinedCells);
      local[SumVertices]        = static_cast<std::uint64_t>(_localNumberOfVertices);
      local[SumHangingVertices] = static_cast<std::uint64_t>(_localNumberOfHangingVertices);
      local[SumSplits]          = static_cast<std::uint64_t>(_localNumberOfSplits);
      local[SumTrees]           = static_cast<std::uint64_t>(_localNumberOfTrees);
      local[SumRanksWithTrees]  = (_localNumberOfTrees > 0) ? 1 : 0;

      const bool emptyRank = spacetrees.empty();

      local[MinTreeLoad] =
        emptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinTreeLoad);
      local[MaxTreeLoad]          = static_cast<std::uint64_t>(_localMaxTreeLoad);
      local[MaxTreeLoadOwnerRank] = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
      local[MinTreeOuterLoad] =
        emptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinTreeOuterLoad);
      local[MaxTreeOuterLoad]   = static_cast<std::uint64_t>(_localMaxTreeOuterLoad);
      local[MinRankLoad]        = emptyRank ? std::numeric_limits<std::uint64_t>::max()
                                            : static_cast<std::uint64_t>(_localNumberOfInnerUnrefinedCells);
      local[MaxRankLoad]        = static_cast<std::uint64_t>(_localNumberOfInnerUnrefinedCells);
      local[SumMemBytes]        = static_cast<std::uint64_t>(_localMemoryUsageInBytes);
      local[SumProcessMemBytes] = static_cast<std::uint64_t>(_localProcessMemoryUsageInBytes);
      // Only the node master contributes host RAM, so a node with several ranks is counted once.
      const bool nodeMaster       = tarch::mpi::Rank::getInstance().isNodeMaster();
      local[SumNodeTotalMemBytes] = nodeMaster ? _localTotalMemoryInBytes : 0;
      local[SumNodeFreeMemBytes]  = nodeMaster ? _localFreeMemoryInBytes : 0;
      local[MinDepth] =
        emptyRank ? std::numeric_limits<std::uint64_t>::max() : static_cast<std::uint64_t>(_localMinDepth);
      local[MaxDepth]            = static_cast<std::uint64_t>(_localMaxDepth);
      local[MinHBits]            = toBits(_localMinH);
      local[MinStationarySweeps] = static_cast<std::uint64_t>(_localNumberOfStationarySweeps);

      // Launch this sweep's reduction without blocking; the next sweep completes it. The first
      // sweep has nothing to publish yet, so it falls through to publishing the local view.
      tarch::mpi::iallreduce(
        _globalExchangeSend.data(),
        _globalExchangeRecv.data(),
        /*count=*/NumberOfWords,
        MPI_UINT64_T,
        gReduceOp,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &_globalExchangeRequest
      );

      _globalExchangeInFlight = true;
    }
  }
#endif

  if (publishLocalAsGlobal) {
    _isViewConsistent = true;

    _globalNumberOfInnerUnrefinedCells     = _localNumberOfInnerUnrefinedCells;
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
  }

  if (_globalNumberOfSplits == 0 and _localNumberOfSplits == 0 and _numberOfStateUpdatesWithoutAnySplit < 65536) {
    _numberOfStateUpdatesWithoutAnySplit++;
  } else if (_globalNumberOfSplits > 0 or _localNumberOfSplits > 0) {
    _numberOfStateUpdatesWithoutAnySplit = 0;
  }

  _localNumberOfSplits = 0;
}

bool toolbox::loadbalancing::Statistics::hasConsistentViewOfWorld() const { return _isViewConsistent; }

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
  _numberOfStateUpdatesWithoutAnySplit = 0;
}

int toolbox::loadbalancing::Statistics::getNumberOfStateUpdatesWithoutAnySplit() const {
  return _numberOfStateUpdatesWithoutAnySplit;
}

int toolbox::loadbalancing::Statistics::getGlobalNumberOfRanksWithAtLeastOneTree() const {
  return _globalNumberOfRanksWithAtLeastOneTree;
}

int toolbox::loadbalancing::Statistics::getGlobalMinTreeLoad() const { return _globalMinTreeLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxTreeLoad() const { return _globalMaxTreeLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMinTreeOuterLoad() const { return _globalMinTreeOuterLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxTreeOuterLoad() const { return _globalMaxTreeOuterLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMinRankLoad() const { return _globalMinRankLoad; }

int toolbox::loadbalancing::Statistics::getGlobalMaxRankLoad() const { return _globalMaxRankLoad; }

int toolbox::loadbalancing::Statistics::getGlobalRankWithMaxTreeLoad() const { return _globalRankWithMaxTreeLoad; }

std::size_t toolbox::loadbalancing::Statistics::getLocalMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_localMemoryUsageInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getGlobalMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_globalMemoryUsageInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getLocalProcessMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_localProcessMemoryUsageInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getGlobalProcessMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_globalProcessMemoryUsageInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getGlobalTotalMemoryInBytes() const {
  return static_cast<std::size_t>(_globalTotalMemoryInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getGlobalFreeMemoryInBytes() const {
  return static_cast<std::size_t>(_globalFreeMemoryInBytes);
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
