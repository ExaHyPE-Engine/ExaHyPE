// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"

#include <cstring>
#include <limits>

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
    MinDepth,
    MaxDepth,
    MinHBits,
    MinStationarySweeps
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

    // MIN/MAX fields
    inout[MinTreeLoad] = std::min(inout[MinTreeLoad], in[MinTreeLoad]);
    if (in[MaxTreeLoad] > inout[MaxTreeLoad]) {
      inout[MaxTreeLoad]          = in[MaxTreeLoad];
      inout[MaxTreeLoadOwnerRank] = in[MaxTreeLoadOwnerRank];
    } else if (in[MaxTreeLoad] == inout[MaxTreeLoad]) {
      inout[MaxTreeLoadOwnerRank] = std::min(inout[MaxTreeLoadOwnerRank], in[MaxTreeLoadOwnerRank]);
    }
    inout[MinDepth] = std::min(inout[MinDepth], in[MinDepth]);
    inout[MaxDepth] = std::max(inout[MaxDepth], in[MaxDepth]);

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
} // namespace
#endif

toolbox::loadbalancing::Statistics::Statistics():
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
  _localMemoryUsageInBytes(0),
  _globalMemoryUsageInBytes(0),
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
#ifdef PEANO_WITH_MPI
  // updateGlobalView() may leave a non-blocking Iallreduce in flight. If the
  // program/unit tests terminate right after the last update, MPI_Finalize()
  // can be called with outstanding requests, which may crash/hang.
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised) {
    int mpiFinalised = 0;
    MPI_Finalized(&mpiFinalised);
    if (not mpiFinalised and _globalExchangeInFlight and _globalExchangeRequest != MPI_REQUEST_NULL) {
      MPI_Wait(&_globalExchangeRequest, MPI_STATUS_IGNORE);
      _globalExchangeRequest  = MPI_REQUEST_NULL;
      _globalExchangeInFlight = false;
    }
  }
#endif
}

void toolbox::loadbalancing::Statistics::waitForGlobalDataExchange() {
#ifdef PEANO_WITH_MPI
  if (not _globalExchangeInFlight) {
    return;
  }

  tarch::mpi::wait(_globalExchangeRequest, "toolbox::loadbalancing::Statistics", "waitForGlobalDataExchange");

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
  _globalMemoryUsageInBytes              = static_cast<std::uint64_t>(g[SumMemBytes]);
  _globalMinDepth                        = static_cast<int>(g[MinDepth]);
  _globalMaxDepth                        = static_cast<int>(g[MaxDepth]);
  _globalMinH                            = fromBits(g[MinHBits]);
  _globalNumberOfStationarySweeps        = static_cast<int>(g[MinStationarySweeps]);

  // If the global representation (which lags behind by one non-blocking MPI step)
  // thinks there are fewer cells than we have locally, the global view is definitely
  // inconsistent/outdated.
  if (_globalNumberOfInnerUnrefinedCells < _localNumberOfInnerUnrefinedCells) {
    logDebug(
      "Local cell count "
      << _localNumberOfInnerUnrefinedCells << " exceeds global cell count " << _globalNumberOfInnerUnrefinedCells
      << ". Statistics view is inconsistent or outdated; this usually happens while a forking tree still has pending refinement events after spawning cells."
    );
    _isViewConsistent                  = false;
    _globalNumberOfInnerUnrefinedCells = _localNumberOfInnerUnrefinedCells;
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

  _localMinDepth = statistics.minDepth;
  _localMaxDepth = statistics.maxDepth;

  _localMinH                     = tarch::la::max(statistics.minH);
  _localNumberOfStationarySweeps = statistics.numberOfStationarySweeps;

  // Compute local min/max tree load based on hosted trees
  if (spacetrees.empty()) {
    _localMinTreeLoad = std::numeric_limits<int>::max();
    _localMaxTreeLoad = 0;
  } else {
    int localMin = std::numeric_limits<int>::max();
    int localMax = 0;
    for (auto tree : spacetrees) {
      const auto& s    = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(tree);
      const int   load = s.numberOfLocalUnrefinedCells;
      localMin         = std::min(localMin, load);
      localMax         = std::max(localMax, load);
    }
    _localMinTreeLoad = localMin;
    _localMaxTreeLoad = localMax;
  }
  bool publishLocalAsGlobal = true;
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    init();
    if (gReady) {
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

      local[MinTreeLoad]          = emptyRank ? std::numeric_limits<std::uint64_t>::max()
                                              : static_cast<std::uint64_t>(_localMinTreeLoad);
      local[MaxTreeLoad]          = static_cast<std::uint64_t>(_localMaxTreeLoad);
      local[MaxTreeLoadOwnerRank] = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
      local[SumMemBytes]          = static_cast<std::uint64_t>(_localMemoryUsageInBytes);
      local[MinDepth]             = emptyRank ? std::numeric_limits<std::uint64_t>::max()
                                              : static_cast<std::uint64_t>(_localMinDepth);
      local[MaxDepth]             = static_cast<std::uint64_t>(_localMaxDepth);
      local[MinHBits]             = toBits(_localMinH);
      local[MinStationarySweeps]  = static_cast<std::uint64_t>(_localNumberOfStationarySweeps);

      MPI_Iallreduce(
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
    _globalMemoryUsageInBytes              = _localMemoryUsageInBytes;
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

int toolbox::loadbalancing::Statistics::getGlobalRankWithMaxTreeLoad() const { return _globalRankWithMaxTreeLoad; }

std::size_t toolbox::loadbalancing::Statistics::getLocalMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_localMemoryUsageInBytes);
}

std::size_t toolbox::loadbalancing::Statistics::getGlobalMemoryUsageInBytes() const {
  return static_cast<std::size_t>(_globalMemoryUsageInBytes);
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
