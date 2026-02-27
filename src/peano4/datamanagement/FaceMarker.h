// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::datamanagement {
  struct FaceMarker;
} // namespace peano4::datamanagement

/**
 * Provide information about selected face
 *
 * This routine is passed into any observer/action set routine which
 * refers to a face, so routines like touchFaceFirstTime(). It provides
 * you with information about the particular face as well as their location
 * within the mesh.
 *
 * @see FaceEnumerator for a discussion of the ordering of the faces.
 */
struct peano4::datamanagement::FaceMarker {
  tarch::la::Vector<DIMENSIONS, double> h{};
  tarch::la::Vector<DIMENSIONS, double> cellCentre{};

  int selectedFace{-1};

  bool cellIsLocal;

  /**
   * Packed per-face state:
   * bit i corresponds to face i in [0, 2*DIMENSIONS)
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
   * Selects a face within a cell, i.e. now the marker knows to which
   * face it corresponds. After that, the routine returns a this
   * reference.
   *
   * @param face Number from 0 to 2d-1
   */
  INLINE_METHOD FaceMarker& select(int face) {
    selectedFace = face;
    return *this;
  }

  /**
   * Return the number of the face.
   *
   * If you use the modulo DIMENSIONS, then you implicitly know along which
   * coordinate axis the corresponding face normal points.
   *
   * @see FaceEnumerator for a discussion of the face ordering.
   *
   * @return Number from 0 to 2d-1
   */
  INLINE_METHOD int getSelectedFaceNumber() const { return selectedFace; }

  /**
   * Center of a particular face with respective reference cell.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(cellCentre);
    int                                   normal = i % DIMENSIONS;
    result(normal) += i >= DIMENSIONS ? h(normal) / 2.0 : -h(normal) / 2.0;
    return result;
  }

  /**
   * A marker is always tied to one particular face. You can still
   * get the data of all faces of a cell (see getX(int)), but this
   * particular function gives you the x coordinate with the centre
   * of the currently selected face.
   *
   * @see select(int)
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const { return getX(selectedFace); }

  /**
   * This operation gives you the normal of a certain face.
   *
   * The orientation of the normal depends on the context in which
   * the face marker is used: Every operation within Peano is
   * always triggered from a cell point of view. The normal corresponds
   * to this view. If you run through a mesh twice, and if you twice
   * hit the same face, the normal thus might be mirrored as a
   * face marker might be constructed from the other adjacent cell.
   *
   * We do enumerate the faces as follows: The first face is the one
   * whose normal is parallel to first coordinate axis. The second
   * face is the one whose normal is parallel to the second axis. The
   * d+1th face is the one parallel to the very first face. The very
   * first face however is closer to the origin in the unit cube.
   *
   * @image html FaceEnumerator.png
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    int                                   index = i % DIMENSIONS;
    result(index) += i < DIMENSIONS ? -h(index) : h(index);
    return result;
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> normal() const { return normal(selectedFace); }

  /**
   * This operation gives you the outer normal of a cell. Different to
   * normal(int), the normal however is oriented along the domain
   * boundaries, i.e. it always points outside from the local domain.
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    int                                   index        = i % DIMENSIONS;
    bool                                  negativeSign = i < DIMENSIONS;
    if (not cellIsLocal) {
      negativeSign = not negativeSign;
    }
    result(index) += negativeSign ? -h(index) : h(index);
    return result;
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> outerNormal() const { return outerNormal(selectedFace); }

  /**
   * Size of the underlying cell
   */
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getH() const { return h; }

  /**
   * Has a face been refined
   *
   * We do not really refine faces. We refine cells. Cells in turn are
   * refined if and only if one of their adjacent vertices holds a refine
   * flag. Consequently, if one vertex adjacent to a face carries the
   * refine flag, the face will be refined as well. In this case, it is
   * not hanging.
   */
  INLINE_METHOD bool hasBeenRefined() const { return hasBeenRefined(selectedFace); }
  INLINE_METHOD bool hasBeenRefined(int i) const { return (hasBeenRefinedMask >> i) & 1u; }

  INLINE_METHOD bool willBeRefined() const { return willBeRefined(selectedFace); }
  INLINE_METHOD bool willBeRefined(int i) const { return (willBeRefinedMask >> i) & 1u; }

  INLINE_METHOD bool isLocal() const { return isLocal(selectedFace); }
  INLINE_METHOD bool isLocal(int i) const { return (isLocalMask >> i) & 1u; }

  INLINE_METHOD bool isHanging() const { return isHanging(selectedFace); }
  INLINE_METHOD bool isHanging(int i) const { return (isHangingMask >> i) & 1u; }

  /**
   * Return relative position within father cell. The result is from
   * (0,1,2,3) x (0,1,2) x (0,1,2) or (0,1,2) x (0,1,2,3) x (0,1,2) or
   * (0,1,2) x (0,1,2) x (0,1,2,3).
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const {
    assertion1(selectedFace >= 0, toString());
    return getRelativePositionWithinFatherCell(selectedFace);
  }
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int i) const {
    assertion2(i >= 0, i, toString());
    tarch::la::Vector<DIMENSIONS, int> result = relativePositionWithinFatherCell;
    const int                          normal = i % DIMENSIONS;
    result(normal) += i >= DIMENSIONS ? 1 : 0;
    return result;
  }

  /**
   * Return relative position of a subface within its father face. The
   * result is from (0,1,2)^(DIMENSIONS-1)
   *
   * Should return the position in all directions but the normal direction
   * of the face. I.e. if I am at the left, front, bottom position, then
   * relative to the right face I am at the front and bottom.
   * Therefore in 2D, if the normal is 0 it should return pos[1], and if
   * the normal is 1 it should return pos[0].
   * In 3D, if the normal is 0 it should return (pos[1], pos[2]), if the
   * normal is 1 it should return (pos[0], pos[2]), it the normal is 2 it
   * should return (pos[0], pos[1]).
   */
  INLINE_METHOD tarch::la::Vector<DIMENSIONS - 1, int> getRelativePositionWithinFatherFace() const {
    const int                              normal = getSelectedFaceNumber() % DIMENSIONS;
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
   * The term patch here refers to a 3x3 or 3x3x3 refinement. The routine
   * determines whether the face is on the boundary of this patch of not.
   */
  INLINE_METHOD bool isInteriorFaceWithinPatch() const {
    int normal = getSelectedFaceNumber() % DIMENSIONS;
    return getRelativePositionWithinFatherCell()(normal) == 1 or getRelativePositionWithinFatherCell()(normal) == 2;
  }

  std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::FaceMarker& marker);
