// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

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

  static INLINE_METHOD bool bitTest8(std::uint8_t m, int i) noexcept { return (m >> i) & 1u; }
  static INLINE_METHOD void bitSet8(std::uint8_t& m, int i, bool v) noexcept {
    const std::uint8_t b = std::uint8_t(1u << i);
    m                    = v ? (m | b) : (m & ~b);
  }
  static INLINE_METHOD bool bitTest32(std::uint32_t m, int i) noexcept { return (m >> i) & 1u; }
  static INLINE_METHOD void bitSet32(std::uint32_t& m, int i, bool v) noexcept {
    const std::uint32_t b = (std::uint32_t(1u) << i);
    m                     = v ? (m | b) : (m & ~b);
  }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept { return x; }
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const noexcept { return h; }
  INLINE_METHOD void   setX(const tarch::la::Vector<DIMENSIONS, double>& v) noexcept { x = v; }
  INLINE_METHOD void   setH(const tarch::la::Vector<DIMENSIONS, double>& v) noexcept { h = v; }
  INLINE_METHOD double getX(int d) const noexcept { return x[d]; }
  INLINE_METHOD double getH(int d) const noexcept { return h[d]; }
  INLINE_METHOD void   setX(int d, double v) noexcept { x[d] = v; }
  INLINE_METHOD void   setH(int d, double v) noexcept { h[d] = v; }

  INLINE_METHOD std::uint8_t getHasBeenRefined() const noexcept { return hasBeenRefined; }
  INLINE_METHOD bool         getHasBeenRefined(int v) const noexcept { return bitTest8(hasBeenRefined, v); }
  INLINE_METHOD void         setHasBeenRefined(std::uint8_t m) noexcept { hasBeenRefined = m; }
  INLINE_METHOD void         setHasBeenRefined(int v, bool on) noexcept { bitSet8(hasBeenRefined, v, on); }

  INLINE_METHOD std::uint8_t getWillBeRefined() const noexcept { return willBeRefined; }
  INLINE_METHOD bool         getWillBeRefined(int v) const noexcept { return bitTest8(willBeRefined, v); }
  INLINE_METHOD void         setWillBeRefined(std::uint8_t m) noexcept { willBeRefined = m; }
  INLINE_METHOD void         setWillBeRefined(int v, bool on) noexcept { bitSet8(willBeRefined, v, on); }

  INLINE_METHOD std::uint8_t getIsVertexLocal() const noexcept { return isVertexLocal; }
  INLINE_METHOD bool         getIsVertexLocal(int v) const noexcept { return bitTest8(isVertexLocal, v); }
  INLINE_METHOD void         setIsVertexLocal(std::uint8_t m) noexcept { isVertexLocal = m; }
  INLINE_METHOD void         setIsVertexLocal(int v, bool on) noexcept { bitSet8(isVertexLocal, v, on); }

  INLINE_METHOD std::uint8_t getIsVertexHanging() const noexcept { return isVertexHanging; }
  INLINE_METHOD bool         getIsVertexHanging(int v) const noexcept { return bitTest8(isVertexHanging, v); }
  INLINE_METHOD void         setIsVertexHanging(std::uint8_t m) noexcept { isVertexHanging = m; }
  INLINE_METHOD void         setIsVertexHanging(int v, bool on) noexcept { bitSet8(isVertexHanging, v, on); }

  INLINE_METHOD std::uint8_t getIsParentVertexLocal() const noexcept { return isParentVertexLocal; }
  INLINE_METHOD bool         getIsParentVertexLocal(int v) const noexcept { return bitTest8(isParentVertexLocal, v); }
  INLINE_METHOD void         setIsParentVertexLocal(std::uint8_t m) noexcept { isParentVertexLocal = m; }
  INLINE_METHOD void         setIsParentVertexLocal(int v, bool on) noexcept { bitSet8(isParentVertexLocal, v, on); }

  INLINE_METHOD std::uint8_t getIsVertexParentOfSubtree() const noexcept { return isVertexParentOfSubtree; }
  INLINE_METHOD bool getIsVertexParentOfSubtree(int v) const noexcept { return bitTest8(isVertexParentOfSubtree, v); }
  INLINE_METHOD void setIsVertexParentOfSubtree(std::uint8_t m) noexcept { isVertexParentOfSubtree = m; }
  INLINE_METHOD void setIsVertexParentOfSubtree(int v, bool on) noexcept { bitSet8(isVertexParentOfSubtree, v, on); }

  INLINE_METHOD std::uint8_t getIsVertexAdjacentToParallelDomainBoundary() const noexcept {
    return isVertexAdjacentToParallelDomainBoundary;
  }
  INLINE_METHOD bool getIsVertexAdjacentToParallelDomainBoundary(int v) const noexcept {
    return bitTest8(isVertexAdjacentToParallelDomainBoundary, v);
  }
  INLINE_METHOD void setIsVertexAdjacentToParallelDomainBoundary(std::uint8_t m) noexcept {
    isVertexAdjacentToParallelDomainBoundary = m;
  }
  INLINE_METHOD void setIsVertexAdjacentToParallelDomainBoundary(int v, bool on) noexcept {
    bitSet8(isVertexAdjacentToParallelDomainBoundary, v, on);
  }

  INLINE_METHOD std::uint8_t getIsFaceLocal() const noexcept { return isFaceLocal; }
  INLINE_METHOD bool         getIsFaceLocal(int f) const noexcept { return bitTest8(isFaceLocal, f); }
  INLINE_METHOD void         setIsFaceLocal(std::uint8_t m) noexcept { isFaceLocal = m; }
  INLINE_METHOD void         setIsFaceLocal(int f, bool on) noexcept { bitSet8(isFaceLocal, f, on); }

  INLINE_METHOD std::uint8_t getIsFaceHanging() const noexcept { return isFaceHanging; }
  INLINE_METHOD bool         getIsFaceHanging(int f) const noexcept { return bitTest8(isFaceHanging, f); }
  INLINE_METHOD void         setIsFaceHanging(std::uint8_t m) noexcept { isFaceHanging = m; }
  INLINE_METHOD void         setIsFaceHanging(int f, bool on) noexcept { bitSet8(isFaceHanging, f, on); }

  INLINE_METHOD std::uint8_t getIsFaceAdjacentToParallelDomainBoundary() const noexcept {
    return isFaceAdjacentToParallelDomainBoundary;
  }
  INLINE_METHOD bool getIsFaceAdjacentToParallelDomainBoundary(int f) const noexcept {
    return bitTest8(isFaceAdjacentToParallelDomainBoundary, f);
  }
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(int f, bool on) noexcept {
    bitSet8(isFaceAdjacentToParallelDomainBoundary, f, on);
  }
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(std::uint8_t m) noexcept {
    isFaceAdjacentToParallelDomainBoundary = m;
  }

  INLINE_METHOD bool getIsCellLocal() const noexcept { return isCellLocal != 0; }
  INLINE_METHOD void setIsCellLocal(bool on) noexcept { isCellLocal = on ? 1u : 0u; }

  INLINE_METHOD bool getIsParentCellLocal() const noexcept { return isParentCellLocal != 0; }
  INLINE_METHOD void setIsParentCellLocal(bool on) noexcept { isParentCellLocal = on ? 1u : 0u; }

  INLINE_METHOD std::uint32_t getIsAdjacentCellLocal() const noexcept { return isAdjacentCellLocal; }
  INLINE_METHOD bool          getIsAdjacentCellLocal(int a) const noexcept { return bitTest32(isAdjacentCellLocal, a); }
  INLINE_METHOD void          setIsAdjacentCellLocal(std::uint32_t m) noexcept { isAdjacentCellLocal = m; }
  INLINE_METHOD void          setIsAdjacentCellLocal(int a, bool on) noexcept { bitSet32(isAdjacentCellLocal, a, on); }

  INLINE_METHOD const std::array<int, TWO_POWER_D>& getVertexDataFrom() const noexcept { return vertexDataFrom; }
  INLINE_METHOD int  getVertexDataFrom(int v) const noexcept { return vertexDataFrom[v]; }
  INLINE_METHOD void setVertexDataFrom(const std::array<int, TWO_POWER_D>& v) noexcept { vertexDataFrom = v; }
  INLINE_METHOD void setVertexDataFrom(int v, int id) noexcept { vertexDataFrom[v] = id; }

  INLINE_METHOD const std::array<int, TWO_POWER_D>& getVertexDataTo() const noexcept { return vertexDataTo; }
  INLINE_METHOD int                                 getVertexDataTo(int v) const { return vertexDataTo[v]; }
  INLINE_METHOD void setVertexDataTo(const std::array<int, TWO_POWER_D>& v) noexcept { vertexDataTo = v; }
  INLINE_METHOD void setVertexDataTo(int v, int id) noexcept { vertexDataTo[v] = id; }

  INLINE_METHOD const std::array<int, TWO_TIMES_D>& getFaceDataFrom() const noexcept { return faceDataFrom; }
  INLINE_METHOD int                                 getFaceDataFrom(int f) const { return faceDataFrom[f]; }
  INLINE_METHOD void setFaceDataFrom(const std::array<int, TWO_TIMES_D>& f) noexcept { faceDataFrom = f; }
  INLINE_METHOD void setFaceDataFrom(int f, int id) noexcept { faceDataFrom[f] = id; }

  INLINE_METHOD const std::array<int, TWO_TIMES_D>& getFaceDataTo() const noexcept { return faceDataTo; }
  INLINE_METHOD int                                 getFaceDataTo(int f) const { return faceDataTo[f]; }
  INLINE_METHOD void setFaceDataTo(const std::array<int, TWO_TIMES_D>& f) noexcept { faceDataTo = f; }
  INLINE_METHOD void setFaceDataTo(int f, int id) noexcept { faceDataTo[f] = id; }

  INLINE_METHOD int  getCellData() const noexcept { return cellData; }
  INLINE_METHOD void setCellData(int c) noexcept { cellData = c; }

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionToFather() const noexcept {
    return relativePositionToFather;
  }
  INLINE_METHOD int  getRelativePositionToFather(int d) const noexcept { return relativePositionToFather[d]; }
  INLINE_METHOD void setRelativePositionToFather(const tarch::la::Vector<DIMENSIONS, int>& v) noexcept {
    relativePositionToFather = v;
  }
  INLINE_METHOD void setRelativePositionToFather(int d, int v) noexcept { relativePositionToFather[d] = v; }

  datamanagement::CellMarker getCellMarker() const;
  datamanagement::FaceMarker getFaceMarker() const;

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridTraversalEvent& event);
