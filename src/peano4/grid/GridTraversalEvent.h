// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridTraversalEvent;
} // namespace peano4::grid

struct peano4::grid::GridTraversalEvent final {
  int level{0};

  tarch::la::Vector<DIMENSIONS, double> x{};
  tarch::la::Vector<DIMENSIONS, double> h{};

  tarch::la::Vector<DIMENSIONS, int> relativePositionToFather{};

  std::uint8_t hasBeenRefined{0};
  std::uint8_t willBeRefined{0};

  std::uint8_t isVertexLocal{0};
  std::uint8_t isVertexHanging{0};
  std::uint8_t isParentVertexLocal{0};
  std::uint8_t isVertexParentOfSubtree{0};
  std::uint8_t isVertexAdjacentToParallelDomainBoundary{0};

  std::uint8_t isFaceLocal{0};
  std::uint8_t isFaceHanging{0};
  std::uint8_t isFaceAdjacentToParallelDomainBoundary{0};

  std::uint8_t  isCellLocal{0};
  std::uint8_t  isParentCellLocal{0};
  std::uint32_t isAdjacentCellLocal{0};

  int                          cellData{-1};
  std::array<int, TWO_POWER_D> vertexDataFrom{};
  std::array<int, TWO_POWER_D> vertexDataTo{};
  std::array<int, TWO_TIMES_D> faceDataFrom{};
  std::array<int, TWO_TIMES_D> faceDataTo{};

  /**
   * Stores pool slots for the cell, its vertices, and its faces.
   *
   * The arrays use cell-local entity order. Pool traversal uses the data fields
   * above for classification sentinels and cell-local positions.
   */
  int                          cellPoolIndex{-1};
  std::array<int, TWO_POWER_D> vertexPoolIndices{};
  std::array<int, TWO_TIMES_D> facePoolIndices{};

  static INLINE_METHOD bool bitTest8(std::uint8_t mask, int bitNumber) noexcept { return (mask >> bitNumber) & 1u; }
  static INLINE_METHOD void bitSet8(std::uint8_t& mask, int bitNumber, bool value) noexcept {
    const std::uint8_t bit = std::uint8_t(1u << bitNumber);
    mask                   = value ? (mask | bit) : (mask & ~bit);
  }
  static INLINE_METHOD bool bitTest32(std::uint32_t mask, int bitNumber) noexcept { return (mask >> bitNumber) & 1u; }
  static INLINE_METHOD void bitSet32(std::uint32_t& mask, int bitNumber, bool value) noexcept {
    const std::uint32_t bit = (std::uint32_t(1u) << bitNumber);
    mask                    = value ? (mask | bit) : (mask & ~bit);
  }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept { return x; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const noexcept { return h; }
  INLINE_METHOD void   setX(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept { x = value; }
  INLINE_METHOD void   setH(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept { h = value; }
  INLINE_METHOD double getX(int axis) const noexcept { return x[axis]; }
  INLINE_METHOD double getH(int axis) const noexcept { return h[axis]; }
  INLINE_METHOD void   setX(int axis, double value) noexcept { x[axis] = value; }
  INLINE_METHOD void   setH(int axis, double value) noexcept { h[axis] = value; }

  INLINE_METHOD std::uint8_t getHasBeenRefined() const noexcept { return hasBeenRefined; }
  INLINE_METHOD bool         getHasBeenRefined(int vertexNumber) const noexcept {
    return bitTest8(hasBeenRefined, vertexNumber);
  }
  INLINE_METHOD void setHasBeenRefined(std::uint8_t mask) noexcept { hasBeenRefined = mask; }
  INLINE_METHOD void setHasBeenRefined(int vertexNumber, bool value) noexcept {
    bitSet8(hasBeenRefined, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getWillBeRefined() const noexcept { return willBeRefined; }
  INLINE_METHOD bool getWillBeRefined(int vertexNumber) const noexcept { return bitTest8(willBeRefined, vertexNumber); }
  INLINE_METHOD void setWillBeRefined(std::uint8_t mask) noexcept { willBeRefined = mask; }
  INLINE_METHOD void setWillBeRefined(int vertexNumber, bool value) noexcept {
    bitSet8(willBeRefined, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsVertexLocal() const noexcept { return isVertexLocal; }
  INLINE_METHOD bool getIsVertexLocal(int vertexNumber) const noexcept { return bitTest8(isVertexLocal, vertexNumber); }
  INLINE_METHOD void setIsVertexLocal(std::uint8_t mask) noexcept { isVertexLocal = mask; }
  INLINE_METHOD void setIsVertexLocal(int vertexNumber, bool value) noexcept {
    bitSet8(isVertexLocal, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsVertexHanging() const noexcept { return isVertexHanging; }
  INLINE_METHOD bool         getIsVertexHanging(int vertexNumber) const noexcept {
    return bitTest8(isVertexHanging, vertexNumber);
  }
  INLINE_METHOD void setIsVertexHanging(std::uint8_t mask) noexcept { isVertexHanging = mask; }
  INLINE_METHOD void setIsVertexHanging(int vertexNumber, bool value) noexcept {
    bitSet8(isVertexHanging, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsParentVertexLocal() const noexcept { return isParentVertexLocal; }
  INLINE_METHOD bool         getIsParentVertexLocal(int vertexNumber) const noexcept {
    return bitTest8(isParentVertexLocal, vertexNumber);
  }
  INLINE_METHOD void setIsParentVertexLocal(std::uint8_t mask) noexcept { isParentVertexLocal = mask; }
  INLINE_METHOD void setIsParentVertexLocal(int vertexNumber, bool value) noexcept {
    bitSet8(isParentVertexLocal, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsVertexParentOfSubtree() const noexcept { return isVertexParentOfSubtree; }
  INLINE_METHOD bool         getIsVertexParentOfSubtree(int vertexNumber) const noexcept {
    return bitTest8(isVertexParentOfSubtree, vertexNumber);
  }
  INLINE_METHOD void setIsVertexParentOfSubtree(std::uint8_t mask) noexcept { isVertexParentOfSubtree = mask; }
  INLINE_METHOD void setIsVertexParentOfSubtree(int vertexNumber, bool value) noexcept {
    bitSet8(isVertexParentOfSubtree, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsVertexAdjacentToParallelDomainBoundary() const noexcept {
    return isVertexAdjacentToParallelDomainBoundary;
  }
  INLINE_METHOD bool getIsVertexAdjacentToParallelDomainBoundary(int vertexNumber) const noexcept {
    return bitTest8(isVertexAdjacentToParallelDomainBoundary, vertexNumber);
  }
  INLINE_METHOD void setIsVertexAdjacentToParallelDomainBoundary(std::uint8_t mask) noexcept {
    isVertexAdjacentToParallelDomainBoundary = mask;
  }
  INLINE_METHOD void setIsVertexAdjacentToParallelDomainBoundary(int vertexNumber, bool value) noexcept {
    bitSet8(isVertexAdjacentToParallelDomainBoundary, vertexNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsFaceLocal() const noexcept { return isFaceLocal; }
  INLINE_METHOD bool         getIsFaceLocal(int faceNumber) const noexcept { return bitTest8(isFaceLocal, faceNumber); }
  INLINE_METHOD void         setIsFaceLocal(std::uint8_t mask) noexcept { isFaceLocal = mask; }
  INLINE_METHOD void setIsFaceLocal(int faceNumber, bool value) noexcept { bitSet8(isFaceLocal, faceNumber, value); }

  INLINE_METHOD std::uint8_t getIsFaceHanging() const noexcept { return isFaceHanging; }
  INLINE_METHOD bool getIsFaceHanging(int faceNumber) const noexcept { return bitTest8(isFaceHanging, faceNumber); }
  INLINE_METHOD void setIsFaceHanging(std::uint8_t mask) noexcept { isFaceHanging = mask; }
  INLINE_METHOD void setIsFaceHanging(int faceNumber, bool value) noexcept {
    bitSet8(isFaceHanging, faceNumber, value);
  }

  INLINE_METHOD std::uint8_t getIsFaceAdjacentToParallelDomainBoundary() const noexcept {
    return isFaceAdjacentToParallelDomainBoundary;
  }
  INLINE_METHOD bool getIsFaceAdjacentToParallelDomainBoundary(int faceNumber) const noexcept {
    return bitTest8(isFaceAdjacentToParallelDomainBoundary, faceNumber);
  }
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(int faceNumber, bool value) noexcept {
    bitSet8(isFaceAdjacentToParallelDomainBoundary, faceNumber, value);
  }
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(std::uint8_t mask) noexcept {
    isFaceAdjacentToParallelDomainBoundary = mask;
  }

  INLINE_METHOD bool getIsCellLocal() const noexcept { return isCellLocal != 0; }
  INLINE_METHOD void setIsCellLocal(bool value) noexcept { isCellLocal = value ? 1u : 0u; }

  INLINE_METHOD bool getIsParentCellLocal() const noexcept { return isParentCellLocal != 0; }
  INLINE_METHOD void setIsParentCellLocal(bool value) noexcept { isParentCellLocal = value ? 1u : 0u; }

  INLINE_METHOD std::uint32_t getIsAdjacentCellLocal() const noexcept { return isAdjacentCellLocal; }
  INLINE_METHOD bool          getIsAdjacentCellLocal(int cellNumber) const noexcept {
    return bitTest32(isAdjacentCellLocal, cellNumber);
  }
  INLINE_METHOD void setIsAdjacentCellLocal(std::uint32_t mask) noexcept { isAdjacentCellLocal = mask; }
  INLINE_METHOD void setIsAdjacentCellLocal(int cellNumber, bool value) noexcept {
    bitSet32(isAdjacentCellLocal, cellNumber, value);
  }

  INLINE_METHOD const std::array<int, TWO_POWER_D>& getVertexDataFrom() const noexcept { return vertexDataFrom; }
  INLINE_METHOD int  getVertexDataFrom(int vertexNumber) const noexcept { return vertexDataFrom[vertexNumber]; }
  INLINE_METHOD void setVertexDataFrom(const std::array<int, TWO_POWER_D>& indices) noexcept {
    vertexDataFrom = indices;
  }
  INLINE_METHOD void setVertexDataFrom(int vertexNumber, int index) noexcept { vertexDataFrom[vertexNumber] = index; }

  INLINE_METHOD const std::array<int, TWO_POWER_D>& getVertexDataTo() const noexcept { return vertexDataTo; }
  INLINE_METHOD int  getVertexDataTo(int vertexNumber) const { return vertexDataTo[vertexNumber]; }
  INLINE_METHOD void setVertexDataTo(const std::array<int, TWO_POWER_D>& indices) noexcept { vertexDataTo = indices; }
  INLINE_METHOD void setVertexDataTo(int vertexNumber, int index) noexcept { vertexDataTo[vertexNumber] = index; }

  INLINE_METHOD const std::array<int, TWO_TIMES_D>& getFaceDataFrom() const noexcept { return faceDataFrom; }
  INLINE_METHOD int  getFaceDataFrom(int faceNumber) const { return faceDataFrom[faceNumber]; }
  INLINE_METHOD void setFaceDataFrom(const std::array<int, TWO_TIMES_D>& indices) noexcept { faceDataFrom = indices; }
  INLINE_METHOD void setFaceDataFrom(int faceNumber, int index) noexcept { faceDataFrom[faceNumber] = index; }

  INLINE_METHOD const std::array<int, TWO_TIMES_D>& getFaceDataTo() const noexcept { return faceDataTo; }
  INLINE_METHOD int  getFaceDataTo(int faceNumber) const { return faceDataTo[faceNumber]; }
  INLINE_METHOD void setFaceDataTo(const std::array<int, TWO_TIMES_D>& indices) noexcept { faceDataTo = indices; }
  INLINE_METHOD void setFaceDataTo(int faceNumber, int index) noexcept { faceDataTo[faceNumber] = index; }

  INLINE_METHOD int  getCellData() const noexcept { return cellData; }
  INLINE_METHOD void setCellData(int index) noexcept { cellData = index; }

  INLINE_METHOD int  getCellPoolIndex() const noexcept { return cellPoolIndex; }
  INLINE_METHOD void setCellPoolIndex(int poolIndex) noexcept { cellPoolIndex = poolIndex; }

  INLINE_METHOD const std::array<int, TWO_POWER_D>& getVertexPoolIndices() const noexcept { return vertexPoolIndices; }
  INLINE_METHOD const std::array<int, TWO_TIMES_D>& getFacePoolIndices() const noexcept { return facePoolIndices; }

  INLINE_METHOD int  getVertexPoolIndex(int position) const noexcept { return vertexPoolIndices[position]; }
  INLINE_METHOD void setVertexPoolIndex(int position, int poolIndex) noexcept {
    vertexPoolIndices[position] = poolIndex;
  }

  INLINE_METHOD int  getFacePoolIndex(int position) const noexcept { return facePoolIndices[position]; }
  INLINE_METHOD void setFacePoolIndex(int position, int poolIndex) noexcept { facePoolIndices[position] = poolIndex; }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionToFather() const noexcept {
    return relativePositionToFather;
  }
  INLINE_METHOD int  getRelativePositionToFather(int axis) const noexcept { return relativePositionToFather[axis]; }
  INLINE_METHOD void setRelativePositionToFather(const tarch::la::Vector<DIMENSIONS, int>& position) noexcept {
    relativePositionToFather = position;
  }
  INLINE_METHOD void setRelativePositionToFather(int axis, int position) noexcept {
    relativePositionToFather[axis] = position;
  }

  peano4::dataview::CellMarker getCellMarker() const;
  peano4::dataview::FaceMarker getFaceMarker() const;

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridTraversalEvent& event);
