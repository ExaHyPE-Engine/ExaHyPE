// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace peano4::datamanagement {
  struct FaceMarker;
} // namespace peano4::datamanagement

/**
 * Describes the faces of the cell a traversal event visits. Every observer
 * routine that refers to a face, such as touchFaceFirstTime(), receives one.
 *
 * @see FaceEnumerator for a discussion of the ordering of the faces.
 */
struct peano4::datamanagement::FaceMarker {
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

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<DIMENSIONS, int> relativePositionWithinFatherCell{};

  /**
   * Selects the face the marker refers to and returns the marker.
   *
   * @param face Number from 0 to 2d-1
   */
  INLINE_METHOD FaceMarker& select(int face) {
    selectedFace = face;
    return *this;
  }

  /**
   * Returns the number of the selected face; its value modulo DIMENSIONS is
   * the axis of the face normal.
   *
   * @see FaceEnumerator for a discussion of the face ordering.
   *
   * @return Number from 0 to 2d-1
   */
  INLINE_METHOD int getSelectedFaceNumber() const { return selectedFace; }

  /**
   * Returns the centre of face faceNumber of the cell.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX(int faceNumber) const {
    tarch::la::Vector<DIMENSIONS, double> result(cellCentre);
    const auto                            normal = faceNumber % DIMENSIONS;
    result(normal) += faceNumber >= DIMENSIONS ? h(normal) / 2.0 : -h(normal) / 2.0;
    return result;
  }

  /**
   * Returns the centre of the selected face; getX(int) returns the centre of
   * any face of the cell.
   *
   * @see select(int)
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const { return getX(selectedFace); }

  /**
   * Returns the normal of face faceNumber, scaled with the cell width and
   * oriented from the cell's point of view, so a marker built from the other
   * adjacent cell returns the mirrored normal. Face faceNumber has its normal
   * along axis faceNumber modulo DIMENSIONS, and the faces below DIMENSIONS
   * lie closer to the origin of the unit cube.
   *
   * @image html FaceEnumerator.png
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal(int faceNumber) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    const auto                            index = faceNumber % DIMENSIONS;
    result(index) += faceNumber < DIMENSIONS ? -h(index) : h(index);
    return result;
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal() const { return normal(selectedFace); }

  /**
   * Returns the normal of face faceNumber oriented away from the local
   * domain: For a remote cell the sign of normal(int) flips.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal(int faceNumber) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    const auto                            index        = faceNumber % DIMENSIONS;
    bool                                  negativeSign = faceNumber < DIMENSIONS;
    if (not cellIsLocal) {
      negativeSign = not negativeSign;
    }
    result(index) += negativeSign ? -h(index) : h(index);
    return result;
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal() const { return outerNormal(selectedFace); }

  /**
   * Returns the width of the cell.
   */
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const { return h; }

  /**
   * Returns whether face faceNumber holds a refined vertex. A refined vertex
   * refines every adjacent cell, so such a face lies between two refined
   * cells and is not hanging.
   */
  INLINE_METHOD bool hasBeenRefined() const { return hasBeenRefined(selectedFace); }
  INLINE_METHOD bool hasBeenRefined(int faceNumber) const { return ((hasBeenRefinedMask >> faceNumber) & 1U) != 0; }

  INLINE_METHOD bool willBeRefined() const { return willBeRefined(selectedFace); }
  INLINE_METHOD bool willBeRefined(int faceNumber) const { return ((willBeRefinedMask >> faceNumber) & 1U) != 0; }

  INLINE_METHOD bool isLocal() const { return isLocal(selectedFace); }
  INLINE_METHOD bool isLocal(int faceNumber) const { return ((isLocalMask >> faceNumber) & 1U) != 0; }

  INLINE_METHOD bool isHanging() const { return isHanging(selectedFace); }
  INLINE_METHOD bool isHanging(int faceNumber) const { return ((isHangingMask >> faceNumber) & 1U) != 0; }

  /**
   * Returns the relative position of face faceNumber within the father cell:
   * The cell's position with the entry along the face normal raised by one
   * for a face on the positive side, so the result is from
   * (0,1,2,3) x (0,1,2) x (0,1,2), (0,1,2) x (0,1,2,3) x (0,1,2) or
   * (0,1,2) x (0,1,2) x (0,1,2,3).
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const {
    assertion(selectedFace >= 0, toString());
    return getRelativePositionWithinFatherCell(selectedFace);
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int faceNumber) const {
    assertion(faceNumber >= 0, faceNumber, toString());
    tarch::la::Vector<DIMENSIONS, int> result = relativePositionWithinFatherCell;
    const auto                         normal = faceNumber % DIMENSIONS;
    result(normal) += faceNumber >= DIMENSIONS ? 1 : 0;
    return result;
  }

  /**
   * Returns the relative position of a subface within its father face, an
   * entry of (0,1,2)^(DIMENSIONS-1): The entries of the father-cell position
   * along every axis except the face normal. In 2D a normal of 0 returns
   * pos[1] and a normal of 1 returns pos[0]; in 3D a normal of 0 returns
   * (pos[1], pos[2]), a normal of 1 returns (pos[0], pos[2]) and a normal of
   * 2 returns (pos[0], pos[1]).
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS - 1, int> getRelativePositionWithinFatherFace() const {
    const auto                             normal = getSelectedFaceNumber() % DIMENSIONS;
    tarch::la::Vector<DIMENSIONS - 1, int> result = {
      normal == 0 ? relativePositionWithinFatherCell[1] : relativePositionWithinFatherCell[0]
#if DIMENSIONS == 3
      ,
      normal == 2 ? relativePositionWithinFatherCell[1] : relativePositionWithinFatherCell[2]
#endif
    };

    return result;
  }

  /**
   * Returns whether the selected face lies inside the 3x3 or 3x3x3 patch of
   * the father cell: On subcell 1 along the normal, on the positive side of
   * subcell 0 or on the negative side of subcell 2.
   */
  INLINE_METHOD bool isInteriorFaceWithinPatch() const {
    assertion(getSelectedFaceNumber() >= 0, toString());
    const auto normal             = getSelectedFaceNumber() % DIMENSIONS;
    const bool faceOnNegativeSide = getSelectedFaceNumber() < DIMENSIONS;
    return (
      (relativePositionWithinFatherCell[normal] == 1)
      or (relativePositionWithinFatherCell[normal] == (faceOnNegativeSide ? 2 : 0))
    );
  }

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::FaceMarker& marker);
