// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_DATAMANAGEMENT_FACE_MARKER_H_
#define _PEANO4_DATAMANAGEMENT_FACE_MARKER_H_


#include "peano4/utils/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "tarch/la/Vector.h"


#include <bitset>


namespace peano4 {
  namespace datamanagement {
    struct FaceMarker;
  }
}


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::FaceMarker& marker );


struct peano4::datamanagement::FaceMarker {
  private:
    /**
     * Centre of the underlying cell
     */
    tarch::la::Vector<Dimensions,double>  _cellCentre;

    /**
     * Size of the underlying cell
     */
    tarch::la::Vector<Dimensions,double>  _h;

    std::bitset<2*Dimensions>             _isRefined;
    std::bitset<2*Dimensions>             _isLocal;

    bool                                  _cellIsLocal;

    int _select;
  public:
    /**
     * The derivation of _isLocal and _isRefined is very similar to
     * peano4::grid::Spacetree::getFaceType().
     */
    FaceMarker(const peano4::grid::GridTraversalEvent& event, int select=0);

    /**
     * Selects a face witin a cell, i.e. now the marker knows to which
     * face it corresponds. After that, the routine returns a this
     * reference.
     */
    FaceMarker& select(int face);

    int getSelectedFaceNumber() const;

    tarch::la::Vector<Dimensions,double> x(int i) const;

    /**
     * A marker is always tied to one particular face. You can still
     * get the data of all faces of a cell (see x(int)), but this
     * particular function gives you the x coordinate with the centre
     * of the currently selected face.
     *
     * @see select(int)
     */
    tarch::la::Vector<Dimensions,double> x() const;

    /**
     * We do enumerate the faces as follows: The first face is the one
     * whose normal is parallel to first coordinate axis. The second
     * face is the one whose normal is parallel to the second axis. The
     * d+1th face is the one parallel to the very first face. The very
     * first face however is closer to the origin in the unit cube.
     */
    tarch::la::Vector<Dimensions,double> normal(int i) const;
    tarch::la::Vector<Dimensions,double> normal() const;

    /**
     * This operation gives you the outer normal of a cell. Different to
     * normal(int), the normal however is oriented along the domain
     * boundaries, i.e. it always points outside from the local domain.
     */
    tarch::la::Vector<Dimensions,double> outerNormal(int i) const;
    tarch::la::Vector<Dimensions,double> outerNormal() const;

    tarch::la::Vector<Dimensions,double>  h() const;

    std::string toString() const;

    bool isRefined() const;
    bool isRefined(int i) const;

    bool isLocal() const;
    bool isLocal(int i) const;
};


#endif
