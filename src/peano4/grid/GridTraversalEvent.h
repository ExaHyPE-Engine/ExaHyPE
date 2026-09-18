// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
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

  int                                      cellData{-1};
  std::array<int, peano4::VerticesPerCell> vertexDataFrom{};
  std::array<int, peano4::VerticesPerCell> vertexDataTo{};
  std::array<int, peano4::FacesPerCell>    faceDataFrom{};
  std::array<int, peano4::FacesPerCell>    faceDataTo{};

  /**
   * Stores pool slots for the cell, its vertices and its faces.
   *
   * The arrays run in cell-local entity order.
   */
  int                                      cellPoolIndex{-1};
  std::array<int, peano4::VerticesPerCell> vertexPoolIndices{};
  std::array<int, peano4::FacesPerCell>    facePoolIndices{};

  static INLINE_METHOD bool bitTest8(std::uint8_t mask, int bitNumber) noexcept;
  static INLINE_METHOD void bitSet8(std::uint8_t& mask, int bitNumber, bool value) noexcept;
  static INLINE_METHOD bool bitTest32(std::uint32_t mask, int bitNumber) noexcept;
  static INLINE_METHOD void bitSet32(std::uint32_t& mask, int bitNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept;
  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const noexcept;
  INLINE_METHOD void                 setX(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept;
  INLINE_METHOD void                 setH(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept;
  [[nodiscard]] INLINE_METHOD double getX(int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD double getH(int axis) const noexcept;
  INLINE_METHOD void                 setX(int axis, double value) noexcept;
  INLINE_METHOD void                 setH(int axis, double value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getHasBeenRefined() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getHasBeenRefined(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setHasBeenRefined(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setHasBeenRefined(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getWillBeRefined() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getWillBeRefined(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setWillBeRefined(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setWillBeRefined(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsVertexLocal() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsVertexLocal(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setIsVertexLocal(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsVertexLocal(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsVertexHanging() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsVertexHanging(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setIsVertexHanging(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsVertexHanging(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsParentVertexLocal() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsParentVertexLocal(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setIsParentVertexLocal(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsParentVertexLocal(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsVertexParentOfSubtree() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsVertexParentOfSubtree(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setIsVertexParentOfSubtree(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsVertexParentOfSubtree(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsVertexAdjacentToParallelDomainBoundary() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsVertexAdjacentToParallelDomainBoundary(int vertexNumber) const noexcept;
  INLINE_METHOD void                       setIsVertexAdjacentToParallelDomainBoundary(std::uint8_t mask) noexcept;
  INLINE_METHOD void setIsVertexAdjacentToParallelDomainBoundary(int vertexNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsFaceLocal() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsFaceLocal(int faceNumber) const noexcept;
  INLINE_METHOD void                       setIsFaceLocal(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsFaceLocal(int faceNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsFaceHanging() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsFaceHanging(int faceNumber) const noexcept;
  INLINE_METHOD void                       setIsFaceHanging(std::uint8_t mask) noexcept;
  INLINE_METHOD void                       setIsFaceHanging(int faceNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint8_t getIsFaceAdjacentToParallelDomainBoundary() const noexcept;
  [[nodiscard]] INLINE_METHOD bool         getIsFaceAdjacentToParallelDomainBoundary(int faceNumber) const noexcept;
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(int faceNumber, bool value) noexcept;
  INLINE_METHOD void setIsFaceAdjacentToParallelDomainBoundary(std::uint8_t mask) noexcept;

  [[nodiscard]] INLINE_METHOD bool getIsCellLocal() const noexcept;
  INLINE_METHOD void               setIsCellLocal(bool value) noexcept;

  [[nodiscard]] INLINE_METHOD bool getIsParentCellLocal() const noexcept;
  INLINE_METHOD void               setIsParentCellLocal(bool value) noexcept;

  [[nodiscard]] INLINE_METHOD std::uint32_t getIsAdjacentCellLocal() const noexcept;
  [[nodiscard]] INLINE_METHOD bool          getIsAdjacentCellLocal(int cellNumber) const noexcept;
  INLINE_METHOD void                        setIsAdjacentCellLocal(std::uint32_t mask) noexcept;
  INLINE_METHOD void                        setIsAdjacentCellLocal(int cellNumber, bool value) noexcept;

  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::VerticesPerCell>& getVertexDataFrom() const noexcept;
  [[nodiscard]] INLINE_METHOD int getVertexDataFrom(int vertexNumber) const noexcept;
  INLINE_METHOD void              setVertexDataFrom(const std::array<int, peano4::VerticesPerCell>& indices) noexcept;
  INLINE_METHOD void              setVertexDataFrom(int vertexNumber, int index) noexcept;

  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::VerticesPerCell>& getVertexDataTo() const noexcept;
  [[nodiscard]] INLINE_METHOD int getVertexDataTo(int vertexNumber) const noexcept;
  INLINE_METHOD void              setVertexDataTo(const std::array<int, peano4::VerticesPerCell>& indices) noexcept;
  INLINE_METHOD void              setVertexDataTo(int vertexNumber, int index) noexcept;

  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::FacesPerCell>& getFaceDataFrom() const noexcept;
  [[nodiscard]] INLINE_METHOD int getFaceDataFrom(int faceNumber) const noexcept;
  INLINE_METHOD void              setFaceDataFrom(const std::array<int, peano4::FacesPerCell>& indices) noexcept;
  INLINE_METHOD void              setFaceDataFrom(int faceNumber, int index) noexcept;

  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::FacesPerCell>& getFaceDataTo() const noexcept;
  [[nodiscard]] INLINE_METHOD int                                          getFaceDataTo(int faceNumber) const noexcept;
  INLINE_METHOD void setFaceDataTo(const std::array<int, peano4::FacesPerCell>& indices) noexcept;
  INLINE_METHOD void setFaceDataTo(int faceNumber, int index) noexcept;

  [[nodiscard]] INLINE_METHOD int getCellData() const noexcept;
  INLINE_METHOD void              setCellData(int index) noexcept;

  [[nodiscard]] INLINE_METHOD int getCellPoolIndex() const noexcept;
  INLINE_METHOD void              setCellPoolIndex(int poolIndex) noexcept;

  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::VerticesPerCell>& getVertexPoolIndices() const noexcept;
  [[nodiscard]] INLINE_METHOD const std::array<int, peano4::FacesPerCell>& getFacePoolIndices() const noexcept;

  [[nodiscard]] INLINE_METHOD int getVertexPoolIndex(int position) const noexcept;
  INLINE_METHOD void              setVertexPoolIndex(int position, int poolIndex) noexcept;

  [[nodiscard]] INLINE_METHOD int getFacePoolIndex(int position) const noexcept;
  INLINE_METHOD void              setFacePoolIndex(int position, int poolIndex) noexcept;

  [[nodiscard]] INLINE_METHOD const tarch::la::Vector<DIMENSIONS, int>& getRelativePositionToFather() const noexcept;
  [[nodiscard]] INLINE_METHOD int getRelativePositionToFather(int axis) const noexcept;
  INLINE_METHOD void setRelativePositionToFather(const tarch::la::Vector<DIMENSIONS, int>& position) noexcept;
  INLINE_METHOD void setRelativePositionToFather(int axis, int position) noexcept;

  [[nodiscard]] peano4::dataview::CellMarker getCellMarker() const;
  [[nodiscard]] peano4::dataview::FaceMarker getFaceMarker() const;

  [[nodiscard]] std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridTraversalEvent& event);

#include "peano4/grid/GridTraversalEvent.cpph"
