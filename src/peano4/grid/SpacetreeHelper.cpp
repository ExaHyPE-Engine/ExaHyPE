// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeHelper.h"

#include "GridTraversalObserver.h"
#include "peano4/Loop.h"

#include <tuple>
#include <utility>

namespace {
  struct FaceAdjacentRanks {
    int first;
    int second;
  };

  INLINE_METHOD FaceAdjacentRanks readFaceAdjacentRanks(
    peano4::grid::GridVertex fineGridVertices[TWO_POWER_D],
    int                      faceNumber,
    int                      vertexIndex,
    bool                     calledByReceivingProcess
  ) {
    const auto& T      = peano4::grid::SpacetreeHelper::getFaceAdjTables();
    const int   normal = faceNumber % DIMENSIONS;

    const peano4::grid::GridVertex& v       = fineGridVertices[vertexIndex];
    const auto                      state   = v.getState();
    const bool                      invalid = calledByReceivingProcess
                                                ? (state == peano4::grid::GridVertex::State::HangingVertex
                              or state == peano4::grid::GridVertex::State::New)
                                                : (state == peano4::grid::GridVertex::State::HangingVertex
                              or state == peano4::grid::GridVertex::State::Delete);
    const auto&                     adj     = calledByReceivingProcess ? v.backupOfAdjacentRanks : v.adjacentRanks;
    const int                       i0      = T.adjIdx0[normal][vertexIndex];
    const int                       i1      = T.adjIdx1[normal][vertexIndex];

    return {invalid ? -1 : adj[i0], invalid ? -1 : adj[i1]};
  }

  INLINE_METHOD void insertSortedUnique(peano4::grid::SpacetreeHelper::NeighbourTrees& neighbourIds, int rank) {
    int insertPos = 0;
    while (insertPos < neighbourIds.count and neighbourIds.ids[insertPos] < rank) {
      insertPos++;
    }
    if (insertPos == neighbourIds.count or neighbourIds.ids[insertPos] != rank) {
      for (int j = neighbourIds.count; j > insertPos; --j) {
        neighbourIds.ids[j] = neighbourIds.ids[j - 1];
      }
      neighbourIds.ids[insertPos] = rank;
      neighbourIds.count++;
    }
  }
} // namespace

const peano4::grid::SpacetreeHelper::Patch3x3IndexTable peano4::grid::SpacetreeHelper::_patch3x3Table = [] {
  Patch3x3IndexTable t{};
  for (int k = 0; k < TWO_POWER_D; ++k) {
    for (int j = 0; j < TWO_POWER_D; ++j) {
      int linear = 0;
      int stride = 1;
      for (int d = 0; d < DIMENSIONS; ++d) {
        const int kd = (k >> d) & 1; // bit of k in dim d
        const int jd = (j >> d) & 1; // bit of j in dim d
        const int v  = kd + jd;      // 0..2
        linear += v * stride;
        stride *= 3;
      }
      t.idx[k][j] = linear;
    }
  }
  return t;
}();

const peano4::grid::SpacetreeHelper::FaceAdjTables peano4::grid::SpacetreeHelper::_faceAdjTable = [] {
  FaceAdjTables t{};
  // Fill faceVerts
  for (int face = 0; face < TWO_TIMES_D; ++face) {
    const int  normal = face % DIMENSIONS;
    const bool upper  = face >= DIMENSIONS;
    int        c      = 0;
    for (int v = 0; v < TWO_POWER_D; ++v) {
      const bool onFace = ((v >> normal) & 1) == (upper ? 1 : 0);
      if (onFace) {
        t.faceVerts[face][c++] = v;
      }
    }
  }
  // Fill adjIdx0/adjIdx1
  for (int n = 0; n < DIMENSIONS; ++n) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      const int base  = (TWO_POWER_D - 1) - v;
      t.adjIdx0[n][v] = static_cast<std::uint8_t>(base & ~(1 << n));
      t.adjIdx1[n][v] = static_cast<std::uint8_t>(base | (1 << n));
    }
  }
  return t;
}();

const peano4::grid::SpacetreeHelper::Patch3x3IndexTable& peano4::grid::SpacetreeHelper::getPatch3x3IndexTable() {
  return _patch3x3Table;
}

const peano4::grid::SpacetreeHelper::FaceAdjTables& peano4::grid::SpacetreeHelper::getFaceAdjTables() {
  return _faceAdjTable;
}

void peano4::grid::SpacetreeHelper::getAdjacentRanksOfFace(
  GridVertex fineGridVertices[TWO_POWER_D],
  int        faceNumber,
  bool       calledByReceivingProcess,
  int        out[TWO_POWER_D]
) {
  assertion(faceNumber >= 0 and faceNumber < TWO_TIMES_D);
  const auto& T     = getFaceAdjTables();
  const int*  verts = T.faceVerts[faceNumber];

  int counter = 0;

  if (calledByReceivingProcess) {
    // Use backupOfAdjacentRanks for all contributing vertices
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const auto adjacentRanks = readFaceAdjacentRanks(fineGridVertices, faceNumber, verts[k], true);
      out[counter++]           = adjacentRanks.first;
      out[counter++]           = adjacentRanks.second;
    }
  } else {
    // Use (current) adjacentRanks for all contributing vertices
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const auto adjacentRanks = readFaceAdjacentRanks(fineGridVertices, faceNumber, verts[k], false);
      out[counter++]           = adjacentRanks.first;
      out[counter++]           = adjacentRanks.second;
    }
  }
}

bool peano4::grid::SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
  int                       rank,
  int                       treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joining,
  bool                      calledByReceivingProcess,
  SpacetreeState            treeState
) {
  const bool receiverCriteria = (joining.count(rank) == 0) and (splitting.count(rank) == 0);
  const bool senderCriteria   = (joining.count(rank) == 0) and (splitTriggered.count(rank) == 0)
                              and treeState != SpacetreeState::Joining;

  const bool mandatoryCriteria = rank != treeNumber and rank != -1 and rank != RankOfPeriodicBoundaryCondition
                                 and treeState != SpacetreeState::EmptyRun;

  return calledByReceivingProcess ? (mandatoryCriteria and receiverCriteria) : (mandatoryCriteria and senderCriteria);
}

int peano4::grid::SpacetreeHelper::getNeighbourTreeAlongFace(
  GridVertex                fineGridVertices[TWO_POWER_D],
  int                       faceNumber,
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joining,
  bool                      calledByReceivingProcess,
  SpacetreeState            treeState,
  bool                      noParallelTopologyChanges
) {
  const auto& T     = getFaceAdjTables();
  const int*  verts = T.faceVerts[faceNumber];

  if (noParallelTopologyChanges) {
    bool       isAdjacentToLocalRank = false;
    int        neighbour             = GridTraversalObserver::NoData;
    const bool mayExchange           = calledByReceivingProcess ? (treeState != SpacetreeState::NewFromSplit)
                                                                : (treeState != SpacetreeState::Joining);

    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const auto adjacentRanks = readFaceAdjacentRanks(fineGridVertices, faceNumber, verts[k], calledByReceivingProcess);
      const int r0 = adjacentRanks.first;
      const int r1 = adjacentRanks.second;

      isAdjacentToLocalRank |= r0 == treeNumber or r1 == treeNumber;

      const auto trySetNeighbour = [&](int rank) {
        if (mayExchange and treeState != SpacetreeState::EmptyRun and rank != treeNumber and rank != -1
            and rank != RankOfPeriodicBoundaryCondition) {
          neighbour = rank;
        }
      };

      trySetNeighbour(r0);
      trySetNeighbour(r1);
    }

    return isAdjacentToLocalRank ? neighbour : GridTraversalObserver::NoData;
  }

  bool       isAdjacentToLocalRank = false;
  int        neighbour             = GridTraversalObserver::NoData;
  const bool checkSplit            = not splitTriggered.empty();
  const bool checkSplitting        = not splitting.empty();
  int        cachedRanks[2 * TWO_POWER_D];
  bool       cachedIsSplit[2 * TWO_POWER_D];
  bool       cachedIsSplitting[2 * TWO_POWER_D];
  int        cachedCount = 0;

  auto getCachedFlags = [&](int rank) {
    for (int c = 0; c < cachedCount; ++c) {
      if (cachedRanks[c] == rank) {
        return std::pair<bool, bool>(cachedIsSplit[c], cachedIsSplitting[c]);
      }
    }

    const bool isSplit     = checkSplit and splitTriggered.count(rank) == 1;
    const bool isSplitting = checkSplitting and splitting.count(rank) == 1;
    if (cachedCount < 2 * TWO_POWER_D) {
      cachedRanks[cachedCount]       = rank;
      cachedIsSplit[cachedCount]     = isSplit;
      cachedIsSplitting[cachedCount] = isSplitting;
      cachedCount++;
    }
    return std::pair<bool, bool>(isSplit, isSplitting);
  };

  auto processAdjacentRank = [&](int adjacentRank) {
    const auto [adjacentIsSplit, adjacentIsSplitting] = getCachedFlags(adjacentRank);
    const bool mandatoryCriteria                      = adjacentRank != treeNumber and adjacentRank != -1
                                   and adjacentRank != RankOfPeriodicBoundaryCondition
                                   and treeState != SpacetreeState::EmptyRun;

    if (calledByReceivingProcess) {
      isAdjacentToLocalRank |= adjacentRank == treeNumber or adjacentIsSplit or adjacentIsSplitting;

      const bool receiverCriteria = not adjacentIsSplit and not adjacentIsSplitting
                                    and treeState != SpacetreeState::NewFromSplit;
      if (mandatoryCriteria and receiverCriteria) {
        neighbour = adjacentRank;
      }
    } else {
      isAdjacentToLocalRank |= adjacentRank == treeNumber or adjacentIsSplit;
      const bool senderCriteria = not adjacentIsSplit;
      if (mandatoryCriteria and senderCriteria) {
        neighbour = adjacentRank;
      }
    }
  };

  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const auto adjacentRanks = readFaceAdjacentRanks(fineGridVertices, faceNumber, verts[k], calledByReceivingProcess);
    const int  r0            = adjacentRanks.first;
    const int  r1            = adjacentRanks.second;

    processAdjacentRank(r0);
    processAdjacentRank(r1);
  }

  return isAdjacentToLocalRank ? neighbour : GridTraversalObserver::NoData;
}

peano4::grid::SpacetreeHelper::NeighbourTrees peano4::grid::SpacetreeHelper::getNeighbourTreesAlongVertex(
  const GridVertex&         vertex,
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joining,
  bool                      calledByReceivingProcess,
  SpacetreeState            treeState,
  bool                      noParallelTopologyChanges
) {
  NeighbourTrees neighbourIds;
  neighbourIds.ids.fill(-1);
  if (vertex.getState() == GridVertex::State::HangingVertex) {
    return neighbourIds;
  }

  if (noParallelTopologyChanges) {
    bool        isLocalVertex         = false;
    const auto& exchangeAdjacentRanks = calledByReceivingProcess ? vertex.backupOfAdjacentRanks : vertex.adjacentRanks;
    const bool  mayExchange           = calledByReceivingProcess ? (treeState != SpacetreeState::NewFromSplit)
                                                                 : (treeState != SpacetreeState::Joining);

    for (int i = 0; i < TWO_POWER_D; i++) {
      const int localRank = vertex.adjacentRanks[i];
      isLocalVertex |= localRank == treeNumber;

      const int rank = exchangeAdjacentRanks[i];
      if (not mayExchange or treeState == SpacetreeState::EmptyRun or rank == treeNumber or rank == -1
          or rank == RankOfPeriodicBoundaryCondition) {
        continue;
      }

      insertSortedUnique(neighbourIds, rank);
    }

    if (not isLocalVertex) {
      neighbourIds.count = 0;
    }

    return neighbourIds;
  }

  const bool checkSplit     = not splitTriggered.empty();
  const bool checkSplitting = calledByReceivingProcess and not splitting.empty();
  const bool checkJoining   = (not calledByReceivingProcess) and not joining.empty();
  int        cachedRanks[2 * TWO_POWER_D];
  bool       cachedIsSplit[2 * TWO_POWER_D];
  bool       cachedIsSplitting[2 * TWO_POWER_D];
  bool       cachedIsJoining[2 * TWO_POWER_D];
  int        cachedCount = 0;

  auto getCachedLocalFlags = [&](int rank) {
    for (int c = 0; c < cachedCount; ++c) {
      if (cachedRanks[c] == rank) {
        return std::tuple<bool, bool, bool>(cachedIsSplit[c], cachedIsSplitting[c], cachedIsJoining[c]);
      }
    }

    const bool inSplit     = checkSplit and splitTriggered.count(rank) == 1;
    const bool inSplitting = checkSplitting and splitting.count(rank) == 1;
    const bool inJoining   = checkJoining and joining.count(rank) == 1;
    if (cachedCount < 2 * TWO_POWER_D) {
      cachedRanks[cachedCount]       = rank;
      cachedIsSplit[cachedCount]     = inSplit;
      cachedIsSplitting[cachedCount] = inSplitting;
      cachedIsJoining[cachedCount]   = inJoining;
      cachedCount++;
    }
    return std::tuple<bool, bool, bool>(inSplit, inSplitting, inJoining);
  };

  bool        isLocalVertex         = false;
  const auto& exchangeAdjacentRanks = calledByReceivingProcess ? vertex.backupOfAdjacentRanks : vertex.adjacentRanks;
  for (int i = 0; i < TWO_POWER_D; i++) {
    const int localRank                          = vertex.adjacentRanks[i];
    const auto [inSplit, inSplitting, inJoining] = getCachedLocalFlags(localRank);
    isLocalVertex |= localRank == treeNumber or inSplit or inSplitting or inJoining;

    const int rank = exchangeAdjacentRanks[i];
    if (not doesRankIdentifyHorizontalDataExchange(
          rank,
          treeNumber,
          splitTriggered,
          splitting,
          joining,
          calledByReceivingProcess,
          treeState
        )) {
      continue;
    }

    insertSortedUnique(neighbourIds, rank);
  }

  if (not isLocalVertex) {
    neighbourIds.count = 0;
  }

  return neighbourIds;
}

std::bitset<TWO_POWER_D> peano4::grid::SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
  GridVertex                vertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joinTriggered,
  const TreeIdSet&          joining,
  bool                      noParallelTopologyChanges
) {
  std::bitset<TWO_POWER_D> result;

  if (noParallelTopologyChanges) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      bool oneLocal  = false;
      bool oneRemote = false;

      for (int i = 0; i < TWO_POWER_D and not(oneLocal and oneRemote); ++i) {
        const int r = vertices[v].backupOfAdjacentRanks[i];
        if (r < 0 and r != RankOfPeriodicBoundaryCondition) {
          continue;
        }

        oneLocal |= r == treeNumber;
        oneRemote |= r != treeNumber;
      }

      result.set(v, oneLocal and oneRemote);
    }

    return result;
  }

  const bool checkSplit     = not splitTriggered.empty();
  const bool checkSplitting = not splitting.empty();
  int        cachedRanks[2 * TWO_POWER_D];
  bool       cachedIsLocal[2 * TWO_POWER_D];
  int        cachedCount = 0;

  auto isLocalRank = [&](int r) {
    for (int i = 0; i < cachedCount; ++i) {
      if (cachedRanks[i] == r) {
        return cachedIsLocal[i];
      }
    }

    const bool local = r == treeNumber or (checkSplit and splitTriggered.count(r) > 0)
                       or (checkSplitting and splitting.count(r) > 0);

    if (cachedCount < 2 * TWO_POWER_D) {
      cachedRanks[cachedCount]   = r;
      cachedIsLocal[cachedCount] = local;
      cachedCount++;
    }
    return local;
  };

  for (int v = 0; v < TWO_POWER_D; ++v) {
    const auto& adj = vertices[v].backupOfAdjacentRanks;

    bool oneLocal  = false;
    bool oneRemote = false;

    for (int i = 0; i < TWO_POWER_D and not(oneLocal and oneRemote); ++i) {
      const int r = adj[i];

      const bool valid = (r >= 0) or (r == RankOfPeriodicBoundaryCondition);
      if (not valid) {
        continue;
      }

      const bool local = isLocalRank(r);
      oneLocal |= local;
      oneRemote |= not local;
    }

    result.set(v, oneLocal and oneRemote);
  }

  return result;
}

std::bitset<TWO_TIMES_D> peano4::grid::SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
  GridVertex                vertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joinTriggered,
  const TreeIdSet&          joining,
  bool                      calledByLeaveCell,
  bool                      noParallelTopologyChanges
) {
  std::bitset<TWO_TIMES_D> result;
  const auto&              T = getFaceAdjTables();

  if (noParallelTopologyChanges) {
    for (int j = 0; j < TWO_TIMES_D; j++) {
      const int* verts     = T.faceVerts[j];
      bool       oneLocal  = false;
      bool       oneRemote = false;

      for (int k = 0; k < TWO_POWER_D / 2 and not(oneLocal and oneRemote); ++k) {
        const auto adjacentRanks = readFaceAdjacentRanks(vertices, j, verts[k], calledByLeaveCell);
        const int  r0            = adjacentRanks.first;
        const int  r1            = adjacentRanks.second;

        if (r0 >= 0 or r0 == RankOfPeriodicBoundaryCondition) {
          oneLocal |= r0 == treeNumber;
          oneRemote |= r0 != treeNumber;
        }
        if (not(oneLocal and oneRemote) and (r1 >= 0 or r1 == RankOfPeriodicBoundaryCondition)) {
          oneLocal |= r1 == treeNumber;
          oneRemote |= r1 != treeNumber;
        }
      }

      result.set(j, oneLocal and oneRemote);
    }

    return result;
  }

  const bool checkSplit     = not splitTriggered.empty();
  const bool checkSplitting = not splitting.empty();
  int        cachedRanks[4 * TWO_POWER_D];
  bool       cachedIsLocal[4 * TWO_POWER_D];
  int        cachedCount = 0;

  auto isLocalRank = [&](int r) {
    for (int i = 0; i < cachedCount; ++i) {
      if (cachedRanks[i] == r) {
        return cachedIsLocal[i];
      }
    }

    const bool local = r == treeNumber or (checkSplit and splitTriggered.count(r) > 0)
                       or (checkSplitting and splitting.count(r) > 0);

    if (cachedCount < 4 * TWO_POWER_D) {
      cachedRanks[cachedCount]   = r;
      cachedIsLocal[cachedCount] = local;
      cachedCount++;
    }
    return local;
  };

  for (int j = 0; j < TWO_TIMES_D; j++) {
    const int* verts     = T.faceVerts[j];
    bool       oneLocal  = false;
    bool       oneRemote = false;

    for (int k = 0; k < TWO_POWER_D / 2 and not(oneLocal and oneRemote); ++k) {
      const auto adjacentRanks = readFaceAdjacentRanks(vertices, j, verts[k], calledByLeaveCell);
      const int  r0            = adjacentRanks.first;
      const int  r1            = adjacentRanks.second;

      auto accumulate = [&](int r) {
        const bool valid = (r >= 0) or (r == SpacetreeHelper::RankOfPeriodicBoundaryCondition);
        if (not valid) {
          return;
        }
        const bool local = isLocalRank(r);
        oneLocal |= local;
        oneRemote |= not local;
      };

      accumulate(r0);
      if (not(oneLocal and oneRemote)) {
        accumulate(r1);
      }
    }

    result.set(j, oneLocal and oneRemote);
  }

  return result;
}

bool peano4::grid::SpacetreeHelper::areAllVerticesNonHanging(GridVertex vertices[TWO_POWER_D]) {
  bool result = true;
  dfor2(k) result &= (vertices[kScalar].getState() != GridVertex::State::HangingVertex);
  enddforx return result;
}

bool peano4::grid::SpacetreeHelper::isSpacetreeNodeRefined(GridVertex vertices[TWO_POWER_D]) {
  bool result = false;
  dfor2(k) result |= GridVertex::willVertexBeRefined(vertices[kScalar]);
  result |= GridVertex::hasVertexBeenRefined(vertices[kScalar]);
  enddforx return result;
}

int peano4::grid::SpacetreeHelper::getTreeOwningSpacetreeNode(
  GridVertex                vertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joinTriggered,
  const TreeIdSet&          joining
) {
  constexpr int NotSetYet = -1;
  int           id        = NotSetYet;

  int weakId = NotSetYet;
  dfor2(
    k
  ) if (vertices[kScalar].getState() != GridVertex::State::HangingVertex
        and vertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1] != -1) {
    weakId = vertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1];
  }
  if (vertices[kScalar].getState() != GridVertex::State::HangingVertex
      and isVertexAdjacentToLocalSpacetree(
        vertices[kScalar],
        treeNumber,
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        true,
        false,
        true
      )) {
    assertion(
      id == NotSetYet or vertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1] == id,
      id,
      kScalar,
      vertices[kScalar].toString(),
      vertices[0].toString(),
      vertices[1].toString(),
      vertices[2].toString(),
      vertices[3].toString(),
      treeNumber
    );
    id = vertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1];
  }
  enddforx assertion(
    id != NotSetYet
      or not isSpacetreeNodeLocal(
        vertices,
        treeNumber,
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        false,
        false,
        false
      ),
    id
  );

  if (id == NotSetYet) {
    id = weakId;
  }

  return id;
}

bool peano4::grid::SpacetreeHelper::isSpacetreeNodeSplitCandidate(
  GridVertex                coarseGridVertices[TWO_POWER_D],
  GridVertex                fineGridVertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const TreeIdSet&          splitting,
  const TreeIdSet&          joinTriggered,
  const TreeIdSet&          joining
) {
  bool isOneVertexAdjacentToPeriodicBC = false;
  bool isOneVertexParentOfForkedTree   = false;

  dfor2(k) const auto& adjacentRanks = fineGridVertices[kScalar].adjacentRanks;
  isOneVertexAdjacentToPeriodicBC
    |= std::find(std::begin(adjacentRanks), std::end(adjacentRanks), RankOfPeriodicBoundaryCondition)
       != std::end(adjacentRanks);
  isOneVertexParentOfForkedTree |= fineGridVertices[kScalar].getFlag(
    GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep
  );
  enddforx

    bool result
    = isSpacetreeNodeLocal(
        fineGridVertices,
        treeNumber,
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        false,
        false,
        false
      )
      and isSpacetreeNodeLocal(
        coarseGridVertices,
        treeNumber,
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        false,
        false,
        false
      )
      and areAllVerticesNonHanging(fineGridVertices) and not isOneVertexAdjacentToPeriodicBC
      and (not isOneVertexParentOfForkedTree);

  return result;
}
