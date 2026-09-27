// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  enum class FaceKind : std::uint8_t;
  enum class Dependence : std::uint8_t;
  struct LevelJump;
  struct ExchangedFace;
  struct Neighbour;
  class Partition;
  class Spacetree;

  TARGET_DECLARE
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = peano4::power(SubdivisionFactor, DIMENSIONS - 1);
  constexpr int NumberOfFaceKinds = 5;
  TARGET_DECLARE_END
} // namespace peano4

/**
 * Names what lies on the two sides of a face: A cell on both sides, a cell and the domain wall,
 * a cell and a coarser cell across the coarse face of its level jump, a cell and finer cells
 * across the fine faces of its level jump, or no cell at all inside a level jump of more than
 * one level.
 */
enum class peano4::FaceKind : std::uint8_t { Interior, Boundary, Hanging, Coarse, Intermediate };

/**
 * Names whether a cell, a face or a level jump reads data a neighbour rank sends: A local one
 * reads local data only, a remote one waits for the halo exchange.
 */
enum class peano4::Dependence : std::uint8_t { Local, Remote };

/**
 * One level jump of the partition: A coarse face and the fine faces one level below it.
 * coarseSide names the side of the coarse face whose cell is coarse; the fine faces hold their
 * own cells on the other side.
 */
struct peano4::LevelJump final {
  int coarseFace{-1};
  int coarseSide{0};
  int level{0};
  int fineFaces[peano4::SubfacesPerFace]{};
};

/** One face this rank and a neighbour rank both hold; side is the side whose cell this rank owns. */
struct peano4::ExchangedFace final {
  int face{-1};
  int side{0};
};

/** One neighbour rank and its run of exchanged faces, in the canonical order both ranks derive. */
struct peano4::Neighbour final {
  int rank{-1};
  int firstFace{0};
  int numberOfFaces{0};
};

/**
 * The cells of the spacetree one rank owns, numbered by slots, with their faces, the level
 * jumps between them and the faces a neighbour rank holds too. A cell keeps its slot across
 * every regrid that keeps the cell; the faces are numbered afresh by every regrid. Every list is
 * ordered by dependence and then by level or kind, so every group a caller loops over is one span.
 */
class peano4::Partition final {
public:
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfSlots() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfFaces() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getMinimumLevel() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getMaximumLevel() const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::Span<const int> getCells() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(Dependence dependence) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(Dependence dependence, int level) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getFaces(Dependence dependence, FaceKind kind) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getFaces(
    Dependence dependence,
    FaceKind   firstKind,
    FaceKind   lastKind
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const LevelJump> getLevelJumps(Dependence dependence) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const LevelJump> getLevelJumps(
    Dependence dependence,
    int        level
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const Neighbour> getNeighbours() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const ExchangedFace> getExchangedFaces(
    const Neighbour& neighbour
  ) const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getCellLevel(int cell) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> getCellLattice(
    int cell
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getCellCentre(
    int cell
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getCellCentre(int cell, int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getCellWidth(
    int cell
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getCellWidth(int cell, int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getCellFace(int cell, int faceNumber) const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD FaceKind getFaceKind(int face) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getFaceNormal(int face) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getFaceLevel(int face) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> getFaceLattice(
    int face
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getFaceCentre(
    int face
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getFaceCentre(int face, int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getFaceWidth(
    int face
  ) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getFaceWidth(int face, int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getFaceCell(int face, int side) const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getWidthOfLevel(int level, int axis) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double>
  getDomainOffset() const noexcept;

  [[nodiscard]] INLINE_METHOD const Partition& onDevice() const noexcept;

private:
  friend class peano4::Spacetree;

  [[nodiscard]] INLINE_METHOD int getRangeIndexOfLevel(Dependence dependence, int level) const noexcept;
  [[nodiscard]] INLINE_METHOD int getRangeIndexOfKind(Dependence dependence, FaceKind kind) const noexcept;

  int _numberOfSlots{0};
  int _numberOfFaces{0};
  int _numberOfCells{0};
  int _numberOfLevelJumps{0};
  int _numberOfNeighbours{0};
  int _minimumLevel{0};
  int _maximumLevel{0};

  /**
   * The cells in the order of the list; the ranges hold one offset per dependence and level and
   * one end. Every range array lies in host memory, in the device twin too.
   */
  const int* _cells{nullptr};
  const int* _cellRanges{nullptr};
  const int* _cellLevel{nullptr};
  const int* _cellLattice{nullptr};
  const int* _facesOfCell{nullptr};

  /** The faces in the order of the list; the ranges hold one offset per dependence and kind and one end. */
  const int*      _faces{nullptr};
  const int*      _faceRanges{nullptr};
  const int*      _faceLevel{nullptr};
  const int*      _faceNormal{nullptr};
  const int*      _faceLattice{nullptr};
  const int*      _cellsOfFace{nullptr};
  const FaceKind* _faceKind{nullptr};

  const LevelJump* _levelJumps{nullptr};
  const int*       _levelJumpRanges{nullptr};

  const Neighbour*     _neighbours{nullptr};
  const ExchangedFace* _exchangedFaces{nullptr};

  const double* _widthOfLevel{nullptr};
  double        _domainOffset[DIMENSIONS]{};

  /** The partition with the same arrays in device memory; null where this partition is that one. */
  const Partition* _deviceTwin{nullptr};
};

#include "peano4/Partition.cpph"
