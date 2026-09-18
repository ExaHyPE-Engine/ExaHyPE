// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/sfc/ActiveCurve.h"
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
  ~VertexMarker() noexcept                         = default;

  explicit VertexMarker(const peano4::grid::GridTraversalEvent& event, int selectedVertex = 0);

  INLINE_METHOD VertexMarker&     select(int vertexNumber);
  [[nodiscard]] INLINE_METHOD int getSelectedVertexNumber() const;

  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getH() const;

  [[nodiscard]] std::string toString() const;

  [[nodiscard]] INLINE_METHOD bool isParentOfSubtree(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool isParentOfSubtree() const;
  [[nodiscard]] INLINE_METHOD bool hasBeenRefined() const;
  [[nodiscard]] INLINE_METHOD bool hasBeenRefined(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool willBeRefined() const;
  [[nodiscard]] INLINE_METHOD bool willBeRefined(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool isLocal() const;
  [[nodiscard]] INLINE_METHOD bool isLocal(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool isHanging() const;
  [[nodiscard]] INLINE_METHOD bool isHanging(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool isAdjacentToParallelDomainBoundary() const;
  [[nodiscard]] INLINE_METHOD bool isAdjacentToParallelDomainBoundary(int vertexNumber) const;
  [[nodiscard]] INLINE_METHOD bool isParentCellLocal() const;
  [[nodiscard]] INLINE_METHOD bool isParentVertexLocal(int vertexNumber) const;

  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(
    int vertexNumber
  ) const;
  [[nodiscard]] INLINE_METHOD bool coincidesWithCoarseGridVertex() const;

  [[nodiscard]] INLINE_METHOD bool isContainedInAdjacentCells(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;
  [[nodiscard]] INLINE_METHOD bool isContainedInAdjacentCellsByDimension(
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          dimension,
    double                                       scaleAdjacentCells = 1.0,
    double                                       tolerance          = tarch::la::NUMERICAL_ZERO_DIFFERENCE
  ) const;

  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativeAdjacentCell(
    const tarch::la::Vector<DIMENSIONS, double>& position
  ) const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingCellCentre() const;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getInvokingParentCellsCentre() const;
  [[nodiscard]] std::bitset<peano4::CellsPerVertex>                 areAdjacentCellsLocal() const;

private:
  INLINE_METHOD static void validateVertexNumber(int vertexNumber);

  tarch::la::Vector<DIMENSIONS, double> _cellCentre{};
  tarch::la::Vector<DIMENSIONS, double> _h{};
  int                                   _select{0};

  std::bitset<peano4::VerticesPerCell>       _hasBeenRefined{};
  std::bitset<peano4::VerticesPerCell>       _willBeRefined{};
  std::bitset<peano4::VerticesPerCell>       _isLocal{};
  std::bitset<peano4::VerticesPerCell>       _isHanging{};
  std::bitset<peano4::VerticesPerCell>       _isParentVertexLocal{};
  std::bitset<peano4::CellsPerNeighbourhood> _isAdjacentCellLocal{};
  std::bitset<peano4::VerticesPerCell>       _isParentOfSubtree{};
  bool                                       _isParentCellLocal{false};
  std::bitset<peano4::VerticesPerCell>       _isAdjacentToParallelDomainBoundary{};
  tarch::la::Vector<DIMENSIONS, int>         _relativePositionWithinFatherCell{};
};

#include "peano4/dataview/VertexMarker.cpph"
