// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  enum class FaceKind : std::uint8_t;
  enum class Adjacency : std::uint8_t;
  struct LevelJump;
  struct ExchangedFace;
  struct Neighbour;
  class CellConnectivity;
  class Spacetree;

  TARGET_DECLARE
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = peano4::power(SubdivisionFactor, DIMENSIONS - 1);
  constexpr int NumberOfFaceKinds = 5;
  TARGET_DECLARE_END
} // namespace peano4

/**
 * What lies on the two sides of a face: Cells on both sides, a cell and the domain boundary, a cell
 * and a coarser cell, a cell and finer cells, or the levels strictly between the coarse and the fine
 * level of a level jump of more than one level.
 */
enum class peano4::FaceKind : std::uint8_t { Interior, Boundary, Hanging, Coarse, Intermediate };

/**
 * Tells whether a cell, a face or a level jump reads data that a neighbour rank sends. A remote
 * one waits for the halo exchange.
 */
enum class peano4::Adjacency : std::uint8_t { Local, Remote };

/**
 * One level jump of the cell connectivity: A coarse face and the fine faces one level below it. The
 * coarse cell lies on side coarseSide of the coarse face. The cell of a hanging fine face lies on the
 * other side.
 */
struct peano4::LevelJump final {
  int coarseFace{-1};
  int coarseSide{0};
  int level{0};
  int fineFaces[peano4::SubfacesPerFace]{};
};

/**
 * One face that this rank and a neighbour rank both store. The member side is the side of the cell
 * this rank owns.
 */
struct peano4::ExchangedFace final {
  int face{-1};
  int side{0};
};

/**
 * One neighbour rank and its range of exchanged faces. Both ranks sort these faces the same way.
 */
struct peano4::Neighbour final {
  int rank{-1};
  int firstFace{0};
  int numberOfFaces{0};
};

/**
 * Stores the cells one rank owns, numbered by slots, with their faces, their level jumps and the
 * faces the rank exchanges with neighbour ranks. A regrid keeps the slot of every surviving cell and
 * renumbers the faces. The lists are sorted by adjacency and then by level or kind and a caller
 * loops over one span per group.
 */
class peano4::CellConnectivity final {
public:
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfSlots() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getNumberOfFaces() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getMinimumLevel() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getMaximumLevel() const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::Span<const int> getCells() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(Adjacency adjacency) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(Adjacency adjacency, int level) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getFaces(Adjacency adjacency, FaceKind kind) const noexcept;

  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getFaces(
    Adjacency adjacency,
    FaceKind  firstKind,
    FaceKind  lastKind
  ) const noexcept;

  [[nodiscard]] INLINE_METHOD tarch::Span<const LevelJump> getLevelJumps(Adjacency adjacency) const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::Span<const LevelJump> getLevelJumps(Adjacency adjacency, int level) const noexcept;

  [[nodiscard]] INLINE_METHOD tarch::Span<const LevelJump> getLevelJumps(
    Adjacency adjacency,
    int       firstLevel,
    int       lastLevel
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
  [[nodiscard]] INLINE_METHOD int getCellCurveIndex(int cell) const noexcept;

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
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getCellCentreBesideFace(
    int face,
    int side
  ) const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getWidthOfLevel(int level, int axis) const noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double>
  getDomainOffset() const noexcept;

  [[nodiscard]] INLINE_METHOD const CellConnectivity& onDevice() const noexcept;

private:
  friend class peano4::Spacetree;

  [[nodiscard]] INLINE_METHOD int getRangeIndexOfLevel(Adjacency adjacency, int level) const noexcept;
  [[nodiscard]] INLINE_METHOD int getRangeIndexOfKind(Adjacency adjacency, FaceKind kind) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getPositionOfLatticeCoordinate(
    double latticeCoordinate,
    int    level,
    int    axis
  ) const noexcept;

  int _numberOfSlots{0};
  int _numberOfFaces{0};
  int _numberOfCells{0};
  int _numberOfLevelJumps{0};
  int _numberOfNeighbours{0};
  int _minimumLevel{0};
  int _maximumLevel{0};

  const int* _cells{nullptr};
  const int* _cellRanges{nullptr};
  const int* _cellLevel{nullptr};
  const int* _cellLattice{nullptr};
  const int* _facesOfCell{nullptr};
  const int* _cellCurveIndex{nullptr};

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
  double        _domainCentre[DIMENSIONS]{};

  const CellConnectivity* _deviceTwin{nullptr};
};

#include "peano4/CellConnectivity.cpph"
