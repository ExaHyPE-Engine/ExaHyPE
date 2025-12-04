// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeHelper.h"

#include "peano4/utils/Loop.h"

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
  const auto& T      = getFaceAdjTables();
  const int*  verts  = T.faceVerts[faceNumber];
  const int   normal = faceNumber % DIMENSIONS;

  int counter = 0;

  if (calledByReceivingProcess) {
    // Use backupOfAdjacentRanks for all contributing vertices
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int         vIdx  = verts[k];
      const GridVertex& v     = fineGridVertices[vIdx];
      const auto        state = v.getState();
      // Receiving side ignores New
      const bool  invalid = (state == GridVertex::State::HangingVertex) or (state == GridVertex::State::New);
      const auto& adj     = v.backupOfAdjacentRanks;
      const int   i0      = T.adjIdx0[normal][vIdx];
      const int   i1      = T.adjIdx1[normal][vIdx];
      out[counter++]      = invalid ? -1 : adj[i0];
      out[counter++]      = invalid ? -1 : adj[i1];
    }
  } else {
    // Use (current) adjacentRanks for all contributing vertices
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int         vIdx  = verts[k];
      const GridVertex& v     = fineGridVertices[vIdx];
      const auto        state = v.getState();
      // Sending side ignores Delete
      const bool  invalid = (state == GridVertex::State::HangingVertex) or (state == GridVertex::State::Delete);
      const auto& adj     = v.adjacentRanks;
      const int   i0      = T.adjIdx0[normal][vIdx];
      const int   i1      = T.adjIdx1[normal][vIdx];
      out[counter++]      = invalid ? -1 : adj[i0];
      out[counter++]      = invalid ? -1 : adj[i1];
    }
  }
}

std::bitset<TWO_POWER_D> peano4::grid::SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
  GridVertex                vertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining
) {
  std::bitset<TWO_POWER_D> result;

  auto isLocal = [&](int r) { return r == treeNumber or splitTriggered.count(r) > 0 or splitting.count(r) > 0; };

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

      const bool local = isLocal(r);
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
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining,
  bool                      calledByLeaveCell
) {
  std::bitset<TWO_TIMES_D> result;

  auto isLocal = [&](int r) { return r == treeNumber or splitTriggered.count(r) > 0 or splitting.count(r) > 0; };

  for (int j = 0; j < TWO_TIMES_D; j++) {
    int adjacency[TWO_POWER_D];
    getAdjacentRanksOfFace(vertices, j, calledByLeaveCell, adjacency);

    bool oneLocal  = false;
    bool oneRemote = false;

    for (int i = 0; i < TWO_POWER_D and not(oneLocal and oneRemote); i++) {
      const int  r     = adjacency[i];
      const bool valid = (r >= 0) or (r == SpacetreeHelper::RankOfPeriodicBoundaryCondition);
      if (not valid) {
        continue;
      }

      const bool local = isLocal(r);
      oneLocal |= local;
      oneRemote |= not local;
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

bool peano4::grid::SpacetreeHelper::isSpacetreeNodeLocal(
  GridVertex                vertices[TWO_POWER_D],
  const int                 treeNumber,
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining,
  bool                      splittingIsConsideredLocal,
  bool                      joiningIsConsideredLocal
) {
  dfor2(k) const int rank      = vertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1];
  const bool         isHanging = vertices[kScalar].getState() == GridVertex::State::HangingVertex;

  if (not isHanging and rank != treeNumber and splitTriggered.find(rank) == splitTriggered.end()
      and (not splittingIsConsideredLocal or splitting.find(rank) == splitting.end())
      and (not joiningIsConsideredLocal or joining.find(rank) == joining.end())) {
    return false;
  }
  enddforx

    return true;
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
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining
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
    assertion8(
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
  enddforx assertion1(
    id != NotSetYet
      or not isSpacetreeNodeLocal(vertices, treeNumber, splitTriggered, splitting, joinTriggered, joining, false, false),
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
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining
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
    = isSpacetreeNodeLocal(fineGridVertices, treeNumber, splitTriggered, splitting, joinTriggered, joining, false, false)
      and isSpacetreeNodeLocal(
        coarseGridVertices,
        treeNumber,
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        false,
        false
      )
      and areAllVerticesNonHanging(fineGridVertices) and not isOneVertexAdjacentToPeriodicBC
      and (not isOneVertexParentOfForkedTree);

  return result;
}
