// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::dataview {
  struct VertexMarker;

  tarch::la::Vector<DIMENSIONS, double> reconstructXOfParentVertex(
    const VertexMarker&            marker,
    const std::bitset<DIMENSIONS>& parentVertexPositionWithinParentCell
  );

  std::ostream& operator<<(std::ostream& out, const VertexMarker& marker);
} // namespace peano4::dataview

namespace peano4::particles::tests {
  class MultiscaleTransitionsTest;
  class TestHelpers;
} // namespace peano4::particles::tests

/**
 * Describes the geometry and topology of one selected vertex.
 *
 * Vertex events originate from a cell, so the marker also provides the state
 * of all vertices and cells adjacent to that cell.
 */
struct peano4::dataview::VertexMarker final {
public:
  VertexMarker()                                   = default;
  VertexMarker(const VertexMarker&)                = default;
  VertexMarker(VertexMarker&&) noexcept            = default;
  VertexMarker& operator=(const VertexMarker&)     = default;
  VertexMarker& operator=(VertexMarker&&) noexcept = default;

  explicit VertexMarker(const peano4::grid::GridTraversalEvent& event, int selectedVertex = 0);
  ~VertexMarker() noexcept = default;

  INLINE_METHOD VertexMarker& select(int vertexNumber);
  INLINE_METHOD int           getSelectedVertexNumber() const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX(int vertexNumber) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getH() const;

  std::string toString() const;

  INLINE_METHOD bool isParentOfSubtree(int vertexNumber) const;
  INLINE_METHOD bool isParentOfSubtree() const;
  INLINE_METHOD bool hasBeenRefined() const;
  INLINE_METHOD bool hasBeenRefined(int vertexNumber) const;
  INLINE_METHOD bool willBeRefined() const;
  INLINE_METHOD bool willBeRefined(int vertexNumber) const;
  INLINE_METHOD bool isLocal() const;
  INLINE_METHOD bool isLocal(int vertexNumber) const;
  INLINE_METHOD bool isHanging() const;
  INLINE_METHOD bool isHanging(int vertexNumber) const;
  INLINE_METHOD bool isAdjacentToParallelDomainBoundary() const;
  INLINE_METHOD bool isAdjacentToParallelDomainBoundary(int vertexNumber) const;
  INLINE_METHOD bool isParentCellLocal() const;
  INLINE_METHOD bool isParentVertexLocal(int vertexNumber) const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int vertexNumber) const;
  INLINE_METHOD bool                               coincidesWithCoarseGridVertex() const;

  INLINE_METHOD bool isContainedInAdjacentCells(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;
  INLINE_METHOD bool isContainedInAdjacentCellsByDimension(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          dimension,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativeAdjacentCell(
    const tarch::la::Vector<DIMENSIONS, double>& position
  ) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingCellCentre() const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const;
  std::bitset<TWO_POWER_D>                            areAdjacentCellsLocal() const;

private:
  friend class peano4::particles::tests::MultiscaleTransitionsTest;
  friend class peano4::particles::tests::TestHelpers;

  INLINE_METHOD static void validateVertexNumber(int vertexNumber);

  tarch::la::Vector<DIMENSIONS, double> _cellCentre{};
  tarch::la::Vector<DIMENSIONS, double> _h{};
  int                                   _select{0};

  std::bitset<TWO_POWER_D>           _hasBeenRefined{};
  std::bitset<TWO_POWER_D>           _willBeRefined{};
  std::bitset<TWO_POWER_D>           _isLocal{};
  std::bitset<TWO_POWER_D>           _isHanging{};
  std::bitset<TWO_POWER_D>           _isParentVertexLocal{};
  std::bitset<THREE_POWER_D>         _isAdjacentCellLocal{};
  std::bitset<TWO_POWER_D>           _isParentOfSubtree{};
  bool                               _isParentCellLocal{false};
  std::bitset<TWO_POWER_D>           _isAdjacentToParallelDomainBoundary{};
  tarch::la::Vector<DIMENSIONS, int> _relativePositionWithinFatherCell{};
};

#include "peano4/dataview/VertexMarker.cpph"
