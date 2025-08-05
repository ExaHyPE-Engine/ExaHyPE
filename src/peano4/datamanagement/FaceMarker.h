// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

#include <bitset>

namespace peano4 {
  namespace datamanagement {
    struct FaceMarker;
  } // namespace datamanagement
} // namespace peano4

std::ostream& operator<<(std::ostream& out, const peano4::datamanagement::FaceMarker& marker);

/**
 * Provide information about selected face
 *
 * This routine is passed into any observer/action set routine which
 * refers to a face, so routines like touchFaceFirstTime(). It provides
 * you with information about the particular face as well as their location
 * within the mesh.
 *
 * @see VertexMarker for an analogous description of the vertex handling.
 * @see FaceEnumerator for a discussion of the ordering of the faces.
 */
struct peano4::datamanagement::FaceMarker {
private:
  /**
   * Centre of the underlying cell
   */
  tarch::la::Vector<DIMENSIONS, double> _cellCentre;

  /**
   * Size of the underlying cell
   */
  tarch::la::Vector<DIMENSIONS, double> _h;

  bool _cellIsLocal;

  int _select;

  /**
   * @see hasBeenRefined()
   */
  std::bitset<2 * DIMENSIONS> _hasBeenRefined;

  /**
   * @see hasBeenRefined()
   */
  std::bitset<2 * DIMENSIONS> _willBeRefined;

  std::bitset<2 * DIMENSIONS> _isLocal;

  /**
   * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
   */
  tarch::la::Vector<DIMENSIONS, int> _relativePositionOfCellWithinFatherCell;

public:
  /**
   * The derivation of _isLocal and _isRefined is very similar to
   * peano4::grid::Spacetree::getFaceType().
   */
  FaceMarker(const peano4::grid::GridTraversalEvent& event, int select = 0);

  /**
   * Selects a face within a cell, i.e. now the marker knows to which
   * face it corresponds. After that, the routine returns a this
   * reference.
   *
   * @param face Number from 0 to 2d-1
   */
  inline FaceMarker& select(int face) {
    _select = face;
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
  inline int getSelectedFaceNumber() const { return _select; }

  /**
   * Center of a particular face with respective reference cell.
   */
  inline tarch::la::Vector<DIMENSIONS, double> x(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(_cellCentre);
    int                                   normal = i % DIMENSIONS;
    result(normal) += i >= DIMENSIONS ? _h(normal) / 2.0 : -_h(normal) / 2.0;
    return result;
  }

  /**
   * A marker is always tied to one particular face. You can still
   * get the data of all faces of a cell (see x(int)), but this
   * particular function gives you the x coordinate with the centre
   * of the currently selected face.
   *
   * @see select(int)
   */
  inline tarch::la::Vector<DIMENSIONS, double> x() const { return x(_select); }

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
  inline tarch::la::Vector<DIMENSIONS, double> normal(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    int                                   index = i % DIMENSIONS;
    result(index) += i < DIMENSIONS ? -_h(index) : _h(index);
    return result;
  }
  inline tarch::la::Vector<DIMENSIONS, double> normal() const { return normal(_select); }

  /**
   * This operation gives you the outer normal of a cell. Different to
   * normal(int), the normal however is oriented along the domain
   * boundaries, i.e. it always points outside from the local domain.
   */
  inline tarch::la::Vector<DIMENSIONS, double> outerNormal(int i) const {
    tarch::la::Vector<DIMENSIONS, double> result(0.0);
    int                                   index        = i % DIMENSIONS;
    bool                                  negativeSign = i < DIMENSIONS;
    if (not _cellIsLocal) {
      negativeSign = not negativeSign;
    }
    result(index) += negativeSign ? -_h(index) : _h(index);
    return result;
  }
  inline tarch::la::Vector<DIMENSIONS, double> outerNormal() const { return outerNormal(_select); }

  /**
   * Size of the underlying cell
   */
  inline tarch::la::Vector<DIMENSIONS, double> h() const { return _h; }

  std::string toString() const;

  /**
   * Has a face been refined
   *
   * We do not really refine faces. We refine cells. Cells in turn are
   * refined if and only if one of their adjacent vertices holds a refine
   * flag. Consequently, if one vertex adjacent to a face carries the
   * refine flag, the face will be refined as well. In this case, it is
   * not hanging.
   */
  inline bool hasBeenRefined() const { return hasBeenRefined(_select); }
  inline bool hasBeenRefined(int i) const { return _hasBeenRefined[i]; }

  inline bool willBeRefined() const { return willBeRefined(_select); }
  inline bool willBeRefined(int i) const { return _willBeRefined[i]; }

  inline bool isLocal() const { return isLocal(_select); }
  inline bool isLocal(int i) const { return _isLocal[i]; }

  /**
   * Return relative position within father cell. The result is from
   * (0,1,2,3) x (0,1,2) x (0,1,2) or (0,1,2) x (0,1,2,3) x (0,1,2) or
   * (0,1,2) x (0,1,2) x (0,1,2,3).
   */
  tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell() const;
  tarch::la::Vector<DIMENSIONS, int> getRelativePositionWithinFatherCell(int i) const;

  /**
   * Return relative position of a subface within its father face. The
   * result is from (0,1,2)^(DIMENSIONS-1)
   */
  tarch::la::Vector<DIMENSIONS - 1, int> getRelativePositionWithinFatherFace() const;

  /**
   * The term patch here refers to a 3x3 or 3x3x3 refinement. The routine
   * determines whether the face is on the boundary of this patch of not.
   */
  bool isInteriorFaceWithinPatch() const;
};
