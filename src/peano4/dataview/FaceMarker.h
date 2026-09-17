// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::dataview {
  struct FaceMarker;

  std::ostream& operator<<(std::ostream& out, const FaceMarker& marker);
} // namespace peano4::dataview

/**
 * Describes the faces of the cell a traversal event visits. Every observer
 * routine that refers to a face, such as touchFaceFirstTime(), receives one.
 *
 * @see peano4::dataview::FaceEnumerator for a discussion of the ordering of
 *   the faces.
 */
struct peano4::dataview::FaceMarker final {
  tarch::la::Vector<DIMENSIONS, double> h{};
  tarch::la::Vector<DIMENSIONS, double> cellCentre{};

  int selectedFace{-1};

  bool cellIsLocal{false};

  /**
   * Packed per-face state, where bit i belongs to face i in [0, 2*DIMENSIONS).
   */
  std::uint8_t hasBeenRefinedMask{0};
  std::uint8_t willBeRefinedMask{0};
  std::uint8_t isLocalMask{0};
  std::uint8_t isHangingMask{0};

  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell{};

  INLINE_METHOD FaceMarker& select(int faceNumber);
  INLINE_METHOD int         getSelectedFaceNumber() const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX(int faceNumber) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal(int faceNumber) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal() const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal(int faceNumber) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal() const;

  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const;

  INLINE_METHOD bool hasBeenRefined() const;
  INLINE_METHOD bool hasBeenRefined(int faceNumber) const;
  INLINE_METHOD bool willBeRefined() const;
  INLINE_METHOD bool willBeRefined(int faceNumber) const;
  INLINE_METHOD bool isLocal() const;
  INLINE_METHOD bool isLocal(int faceNumber) const;
  INLINE_METHOD bool isHanging() const;
  INLINE_METHOD bool isHanging(int faceNumber) const;

  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int faceNumber) const;
  INLINE_METHOD tarch::la::Vector<DIMENSIONS - 1, int> getRelativePositionWithinFatherFace() const;
  INLINE_METHOD bool                                   isInteriorFaceWithinPatch() const;

  std::string toString() const;

private:
  INLINE_METHOD static bool isSet(std::uint8_t mask, int faceNumber);
};

#include "peano4/dataview/FaceMarker.cpph"
