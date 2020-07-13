// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_DATAMANAGEMENT_VERTEX_MARKER_H_
#define _PEANO4_DATAMANAGEMENT_VERTEX_MARKER_H_


#include "peano4/utils/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "tarch/la/Vector.h"


namespace peano4 {
  namespace datamanagement {
    struct VertexMarker;
  }
}


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::VertexMarker& marker );


struct peano4::datamanagement::VertexMarker {
  private:
	/**
	 * Centre of the underlying cell
	 */
    tarch::la::Vector<Dimensions,double>  _cellCentre;

	/**
	 * Size of the underlying cell
	 */
    tarch::la::Vector<Dimensions,double>  _h;

    int _select;

    std::bitset<TwoPowerD>  _isRefined;
    std::bitset<TwoPowerD>  _isLocal;
  public:
    VertexMarker(const peano4::grid::GridTraversalEvent& event, int select = 0);

    /**
     * Picks a vertex within a cell. After that, the routine returns.
     */
    VertexMarker& select(int number);

    int getSelectedVertexNumber() const;

    /**
     * We do enumerate the vertices in a lexicographic way, i.e. we start with the
     * bottom left vertex. Then we run along the first Cartesian axis, then the
     * second, and so forth. This yields a z-pattern for the enumeration.
     *
     * @return Position of ith vertex
     */
    tarch::la::Vector<Dimensions,double> x(int i) const;
    tarch::la::Vector<Dimensions,double> x() const;

    tarch::la::Vector<Dimensions,double>  h() const;

    std::string toString() const;

    /**
     * A vertex is refined iff it is surrounded by @f$ 2^d @f$ cells which are
     * refined.
     */
    bool isRefined() const;
    bool isRefined(int i) const;

    bool isLocal() const;
    bool isLocal(int i) const;
};



#endif
