// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "GridVertex.h"
#include "SpacetreeState.h"
#include "SplitInstruction.h"
#include "TreeIdSet.h"

#include <array>
#include <bitset>
#include <vector>

namespace peano4::grid {
  class SpacetreeHelper;
} // namespace peano4::grid

class peano4::grid::SpacetreeHelper {
private:
  // Maps (vertex k in {0..2^D-1}, adjacency entry j in {0..2^D-1})
  // to the linearised index of the corresponding entry in the 3^D patch.
  struct Patch3x3IndexTable {
    int idx[TWO_POWER_D][TWO_POWER_D];
  };

  struct FaceAdjTables {
    int          faceVerts[TWO_TIMES_D][TWO_POWER_D / 2]; // Which vertices lie on each face
    std::uint8_t adjIdx0[DIMENSIONS][TWO_POWER_D];        // idx0 for (normal, vertex)
    std::uint8_t adjIdx1[DIMENSIONS][TWO_POWER_D];        // idx1 for (normal, vertex)
  };

  static const Patch3x3IndexTable _patch3x3Table;
  static const FaceAdjTables      _faceAdjTable;

public:
  /**
   * Periodic boundary conditions are technically realised as domain
   * decomposition, i.e. the vertices along the periodic boundaries carry
   * rank numbers which host RankOfPeriodicBoundaryCondition. So this is
   * the rank number dedicated to periodic BCs. As we treat the BCs as
   * particular neighbour ranks, all data along the periodic BCs is
   * automatically piped onto specialised streams. This holds for both
   * user and grid data.
   */
  static constexpr int RankOfPeriodicBoundaryCondition               = -2;
  static constexpr int NumberOfStationarySweepsToWaitAtLeastTillJoin = 2;

  struct NeighbourTrees {
    std::array<int, TWO_POWER_D> ids;
    int                          count = 0;

    INLINE_METHOD int size() const { return count; }
    INLINE_METHOD int operator[](int i) const { return ids[i]; }

    INLINE_METHOD operator std::vector<int>() const { return std::vector<int>(ids.begin(), ids.begin() + count); }
  };

  static const FaceAdjTables&      getFaceAdjTables();
  static const Patch3x3IndexTable& getPatch3x3IndexTable();

  static void getAdjacentRanksOfFace(
    GridVertex vertex[TWO_POWER_D],
    int        faceNumber,
    bool       useBackedUpAdjacencyInformation,
    int        out[TWO_POWER_D]
  );

  static bool doesRankIdentifyHorizontalDataExchange(
    int                       rank,
    int                       treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joining,
    bool                      calledByReceivingProcess,
    SpacetreeState            treeState
  );

  static int getNeighbourTreeAlongFace(
    GridVertex                fineGridVertices[TWO_POWER_D],
    int                       faceNumber,
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joining,
    bool                      calledByReceivingProcess,
    SpacetreeState            treeState,
    bool                      noParallelTopologyChanges
  );

  static NeighbourTrees getNeighbourTreesAlongVertex(
    const GridVertex&         vertex,
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joining,
    bool                      calledByReceivingProcess,
    SpacetreeState            treeState,
    bool                      noParallelTopologyChanges
  );

  INLINE_METHOD static bool isVertexAdjacentToLocalSpacetree(
    const GridVertex&         vertex,
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      splittingIsConsideredLocal,
    bool                      joiningIsConsideredLocal,
    bool                      isHangingVertexAdjacentToLocalTree
  ) {
    const bool checkSplit     = not splitTriggered.empty();
    const bool checkSplitting = splittingIsConsideredLocal and not splitting.empty();
    const bool checkJoining   = joiningIsConsideredLocal and not joining.empty();

    bool result = false;

    for (auto i = 0; i < TWO_POWER_D; i++) {
      const auto adjacentRank = vertex.adjacentRanks[i];

      assertion(splitTriggered.count(adjacentRank) <= 1);
      assertion(splitting.count(adjacentRank) <= 1);

      const bool inSplit     = checkSplit and splitTriggered.count(adjacentRank);
      const bool inSplitting = checkSplitting and splitting.count(adjacentRank);
      const bool inJoining   = checkJoining and joining.count(adjacentRank);
      if (adjacentRank == treeNumber or inSplit or inSplitting or inJoining) {
        result = true;
        break;
      }
    }

    return result
           and (not isHangingVertexAdjacentToLocalTree ? vertex.getState() != GridVertex::State::HangingVertex : true);
  }

  INLINE_METHOD static std::bitset<TWO_POWER_D> areVerticesLocal(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      noParallelTopologyChanges
  ) {
    if (noParallelTopologyChanges) {
      std::bitset<TWO_POWER_D> bitset;
      for (int i = 0; i < TWO_POWER_D; ++i) {
        bool isLocal = false;
        for (int j = 0; j < TWO_POWER_D; ++j) {
          // Keep locality conservative until the current and backup snapshots converge again.
          if (vertices[i].adjacentRanks[j] == treeNumber or vertices[i].backupOfAdjacentRanks[j] == treeNumber) {
            isLocal = true;
            break;
          }
        }
        bitset.set(i, isLocal);
      }
      return bitset;
    }

    std::bitset<TWO_POWER_D> bitset;
    for (auto i = 0; i < TWO_POWER_D; i++) {
      bitset.set(
        i,
        isVertexAdjacentToLocalSpacetree(
          vertices[i],
          treeNumber,
          splitTriggered,
          splitting,
          joinTriggered,
          joining,
          true,
          true,
          true
        )
      );
    }
    return bitset;
  }

  INLINE_METHOD static std::bitset<TWO_TIMES_D> areFacesLocal(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      noParallelTopologyChanges
  ) {
    if (noParallelTopologyChanges) {
      std::bitset<TWO_TIMES_D> result;
      const auto&              T = SpacetreeHelper::getFaceAdjTables();

      for (int faceNumber = 0; faceNumber < TWO_TIMES_D; ++faceNumber) {
        const int  normal  = faceNumber % DIMENSIONS;
        const bool upper   = faceNumber >= DIMENSIONS;
        bool       isLocal = false;

        for (int i = 0; i < TWO_POWER_D / 2; ++i) {
          const int         vIdx = T.faceVerts[faceNumber][i];
          const GridVertex& v    = vertices[vIdx];
          const int         i0   = T.adjIdx0[normal][vIdx];
          const int         i1   = T.adjIdx1[normal][vIdx];

          if (v.getState() != GridVertex::State::HangingVertex or upper) {
            if (v.adjacentRanks[i0] == treeNumber) {
              isLocal = true;
              break;
            }
          }
          if (v.getState() != GridVertex::State::HangingVertex or not upper) {
            if (v.adjacentRanks[i1] == treeNumber) {
              isLocal = true;
              break;
            }
          }
        }

        result.set(faceNumber, isLocal);
      }

      return result;
    }

    std::bitset<TWO_TIMES_D> result;

    const auto& T = SpacetreeHelper::getFaceAdjTables();

    auto isLocalRank = [&](int r) { return r == treeNumber or splitTriggered.count(r) > 0 or splitting.count(r) > 0; };

    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      const int  normal = faceNumber % DIMENSIONS;
      const bool upper  = faceNumber >= DIMENSIONS;

      bool isLocal = false;

      for (int i = 0; i < TWO_POWER_D / 2; i++) {
        const int         vIdx = T.faceVerts[faceNumber][i];
        const GridVertex& v    = vertices[vIdx];

        const int i0 = T.adjIdx0[normal][vIdx]; // Entry with normal-bit = 0
        const int i1 = T.adjIdx1[normal][vIdx]; // Entry with normal-bit = 1

        if (v.getState() != GridVertex::State::HangingVertex or upper) {
          if (isLocalRank(v.adjacentRanks[i0])) {
            isLocal = true;
            break;
          }
        }

        if (v.getState() != GridVertex::State::HangingVertex or not upper) {
          if (isLocalRank(v.adjacentRanks[i1])) {
            isLocal = true;
            break;
          }
        }
      }

      result.set(faceNumber, isLocal);
    }

    return result;
  }

  static std::bitset<TWO_POWER_D> areVerticesAdjacentToParallelDomainBoundary(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      noParallelTopologyChanges
  );

  static std::bitset<TWO_TIMES_D> areFacesAdjacentToParallelDomainBoundary(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      calledByLeaveCell,
    bool                      noParallelTopologyChanges
  );

  static bool areAllVerticesNonHanging(GridVertex vertices[TWO_POWER_D]);

  INLINE_METHOD static bool isSpacetreeNodeLocal(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining,
    bool                      splittingIsConsideredLocal,
    bool                      joiningIsConsideredLocal,
    bool                      noParallelTopologyChanges
  ) {
    if (noParallelTopologyChanges) {
      for (int k = 0; k < TWO_POWER_D; ++k) {
        const int  rank      = vertices[k].adjacentRanks[TWO_POWER_D - k - 1];
        const bool isHanging = vertices[k].getState() == GridVertex::State::HangingVertex;
        if (not isHanging and rank != treeNumber) {
          return false;
        }
      }
      return true;
    }

    for (int k = 0; k < TWO_POWER_D; ++k) {
      const int  rank      = vertices[k].adjacentRanks[TWO_POWER_D - k - 1];
      const bool isHanging = vertices[k].getState() == GridVertex::State::HangingVertex;

      if (not isHanging and rank != treeNumber and splitTriggered.count(rank) == 0
          and (not splittingIsConsideredLocal or splitting.count(rank) == 0)
          and (not joiningIsConsideredLocal or joining.count(rank) == 0)) {
        return false;
      }
    }

    return true;
  }

  static bool isSpacetreeNodeRefined(GridVertex vertices[TWO_POWER_D]);

  static bool isSpacetreeNodeSplitCandidate(
    GridVertex                coarseGridVertices[TWO_POWER_D],
    GridVertex                fineGridVertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining
  );

  static int getTreeOwningSpacetreeNode(
    GridVertex                vertices[TWO_POWER_D],
    const int                 treeNumber,
    const SplitSpecification& splitTriggered,
    const TreeIdSet&          splitting,
    const TreeIdSet&          joinTriggered,
    const TreeIdSet&          joining
  );
};
