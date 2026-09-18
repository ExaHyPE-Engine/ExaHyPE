// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestHelpers.h"

#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/particles/particles.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"

namespace {
#if DIMENSIONS == 2
  constexpr double        MeshSize         = 1.0 / peano4::Power<peano4::sfc::ActiveCurve::SubdivisionFactor, 2>;
  constexpr std::uint32_t AllAdjacentCells = (1U << peano4::CellsPerNeighbourhood) - 1U;

  /**
   * Returns the marker of one vertex as seen from the cell around cellCentre,
   * with the given masks of local vertices and local adjacent cells.
   */
  peano4::dataview::VertexMarker vertexMarker(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const int                                    selectedVertex,
    const std::uint8_t                           localVertices,
    const std::uint32_t                          localAdjacentCells
  ) {
    peano4::grid::GridTraversalEvent event;
    event.setX(cellCentre);
    event.setH(tarch::la::Vector<DIMENSIONS, double>(MeshSize));
    event.setIsVertexLocal(localVertices);
    event.setIsAdjacentCellLocal(localAdjacentCells);
    return peano4::dataview::VertexMarker(event, selectedVertex);
  }
#endif
} // namespace

peano4::particles::tests::TestHelpers::TestHelpers() :
  TestCase("peano4::particles::tests::TestHelpers") {}

void peano4::particles::tests::TestHelpers::run() { testMethod(testParticleAssignedToVertexWillBeLocal); }

/**
 * Two ranks meet along the cell column right of x = 5 * MeshSize. A particle in
 * the cell below and left of the vertex (5 * MeshSize, 5 * MeshSize) has to count
 * as local on the rank that owns that cell and as remote on the other rank, from
 * the viewpoint of every cell around the vertex and for the vertex below as well.
 */
void peano4::particles::tests::TestHelpers::testParticleAssignedToVertexWillBeLocal() {
#if DIMENSIONS == 2
  struct View {
    tarch::la::Vector<DIMENSIONS, double> cellCentre;
    int                                   selectedVertex;
    std::uint8_t                          localVertices;
    std::uint32_t                         localAdjacentCells;
  };
  const View views[] = {
    {{MeshSize * 4.5, MeshSize * 5.5}, 1, 0b0010, 0b111000100},
    {{MeshSize * 5.5, MeshSize * 5.5}, 0, 0b0011, 0b111000010},
    {{MeshSize * 4.5, MeshSize * 4.5}, 3, 0b1011, 0b000100111},
    {{MeshSize * 5.5, MeshSize * 4.5}, 2, 0b1111, 0b000010011},
    {{MeshSize * 4.5, MeshSize * 4.5}, 1, 0b1011, 0b000100111},
    {{MeshSize * 5.5, MeshSize * 4.5}, 0, 0b1111, 0b000010011},
  };
  // Both particles sit in the cell below and left of the vertex, close to its right face.
  const tarch::la::Vector<DIMENSIONS, double> particles[] = {
    {4.905135 * MeshSize, 4.5 * MeshSize},
    {4.994964 * MeshSize, 4.5 * MeshSize}
  };

  for (const auto& view : views) {
    const auto remoteRank =
      vertexMarker(view.cellCentre, view.selectedVertex, view.localVertices, view.localAdjacentCells);
    const auto owningRank =
      vertexMarker(view.cellCentre, view.selectedVertex, 0b1111, ~view.localAdjacentCells & AllAdjacentCells);
    for (const auto& particle : particles) {
      validateWithParams2(
        not peano4::particles::particleAssignedToVertexWillBeLocal(particle, remoteRank),
        particle,
        remoteRank.toString()
      );
      validateWithParams2(
        peano4::particles::particleAssignedToVertexWillBeLocal(particle, owningRank),
        particle,
        owningRank.toString()
      );
    }
  }
#endif
}
