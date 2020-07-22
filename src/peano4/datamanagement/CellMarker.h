// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_DATAMANAGEMENT_CELL_MARKER_H_
#define _PEANO4_DATAMANAGEMENT_CELL_MARKER_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"


namespace peano4 {
  namespace datamanagement {
    struct CellMarker;
  }
}


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::CellMarker& marker );


/**
 *
 * <h2> All in header policy </h2>
 *
 * The all-in-the-header policy here seems to be weird and it is actually not nice.
 * However, we have to be able to serialise the marker to offload it to the GPU.
 * Therefore,
 */
struct peano4::datamanagement::CellMarker {
  private:
    tarch::la::Vector<Dimensions,double>  _centre;

    tarch::la::Vector<Dimensions,double>  _h;

    bool _isRefined;
    bool _isLocal;
    bool _areAllVerticesRefined;
    bool _isOneVertexHanging;
    bool _areAllVerticesInsideDomain;
  public:
    CellMarker(const peano4::grid::GridTraversalEvent& event);

    bool isRefined() const;

    /**
     * The x coordinate of a cell is its centre.
     */
    inline tarch::la::Vector<Dimensions,double>  x() const {
      return _centre;
    }

    inline tarch::la::Vector<Dimensions,double>  h() const {
      return _h;
    }

    std::string toString() const;

    bool isLocal() const;

    /*
    bool areAllVerticesRefined() const;
    bool isOneVertexHanging() const;
    bool isAdjacentToDomainBoundary() const;
     */


    /**
     * A enclave cell in the definition of Charrier, Hazelwood, Weinzierl is a
     * cell that is not a skeleton cell. A skeleton cell is cell which either
     *
     * - is adjacent to a resolution transitions; or
     * - is adjacent to a domain boundary
     */
    bool isEnclaveCell() const;
    bool isSkeletonCell() const;
};


#endif
