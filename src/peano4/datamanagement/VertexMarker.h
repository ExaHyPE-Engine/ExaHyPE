// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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

    std::bitset<TwoPowerD>  _hasBeenRefined;
    std::bitset<TwoPowerD>  _willBeRefined;
    std::bitset<TwoPowerD>  _isLocal;
    std::bitset<TwoPowerD>  _isHanging;

    bool                    _isParentCellLocal;
    //std::bitset<TwoPowerD>  _isAdjacentToParallelDomainBoundary;

    /**
     * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
     */
    tarch::la::Vector<Dimensions,int>  _relativePositionOfCellWithinFatherCell;
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
    bool hasBeenRefined() const;
    bool hasBeenRefined(int i) const;

    bool willBeRefined() const;
    bool willBeRefined(int i) const;

    /**
     * @see isLocal(int) for currently selected/represented vertex
     */
    bool isLocal() const;

    /**
     * Is a vertex local, i.e. not adjacent to any boundary or remote
     */
    bool isLocal(int i) const;

    bool isHanging() const;

    bool isHanging(int i) const;

    /**
     * Whenever we touch a vertex the first or the last time, we touch
     * it from a cell: If we touch a vertex the first time, the traversal
     * is just about to enter a cell and sees that one of its vertices is
     * not loaded yet. When we issue a touchVertexLastTime() event, the
     * traversal is just about to leave a cell.
     *
     * So there's a cell state associated with any vertex event, and,
     * therefore, we know if the corresponding cell is local and whether
     * its parent cell is local.
     */
    bool isParentCellLocal() const;

    //bool isAdjacentToParallelDomainBoundary() const;
    //bool isAdjacentToParallelDomainBoundary(int i) const;

    /**
     * Return relative position within father cell. The result is from
     * (0,1,2,3)^d and denotes the vertex position within a 3x3 or 3x3x3
     * respectively mesh.
     *
     * @see coincidesWithCoarseGridVertex()
     */
    tarch::la::Vector<Dimensions,int>  getRelativePositionWithinFatherCell() const;
    tarch::la::Vector<Dimensions,int>  getRelativePositionWithinFatherCell(int i) const;

    /**
     * The vertex (0,0,0) coincides with a coarse grid vertex, the (3,0,0) does
     * so too, and so forth.
     *
     * ## Frequent use case
     *
     * If you want to program some simply injection, it helps to identify vertices
     * which coincide spatially with vertices on the next coarser level. A classic
     * are codes like
     *
     *       if ( marker.coincidesWithCoarseGridVertex() ) {
     *         tarch::la::Vector<Dimensions,int> parent = marker.getRelativePositionWithinFatherCell() / 3;
     *         coarseGridVerticesX( peano4::utils::dLinearised(parent,2) ).set...
     *       }
     *
     * within touchVertexLastTime. This snippet looks if two vertices coincide
     * spatially. If so, it computes the parent's relative position within the
     * parent cell which is simply a division by 3. We then linearise this vector
     * index and thus have access to teh parent.
     */
    bool coincidesWithCoarseGridVertex() const;


    /**
     * Will return true if x is contained within the adjacent cells. You can use
     * shrink those cells via the additional parameter if you want.
     *
     * The geometric operation is defined over the closed interval, i.e. we
     * look into the adjacent cubes to the vertex and their faces still count as
     * "inside". If x is exactly on the face of a cube adjacent to the vertex,
     * we will get back a true. Indeed, the cubes are even slightly bigger due
     * to machine precision.
     */
    bool isContainedInAdjacentCells( const tarch::la::Vector<Dimensions,double>& x, double scaleAdjacentCells = 1.0 ) const;

    /**
     * Each vertex is surrounded by @f$ 2^d @f$ cells. In reality,
     * it might be fewer of them, as we might have to deal with a hanging vertex,
     * but logically we can always assume that there were @f$ 2^d @f$.
     * Let's assume that they are grouped lexicographically around the vertex,
     * i.e. we have
     * <pre>

2 | 3
--+--
0 | 1

       </pre>
     *
     * cells centred around a central vertex denoted by the plus in this fancy
     * ASCII art. If x falls into the cell 1, this routine would return (1,0).
     * If it falls into cell 3, it would return (1,1).
     *
     */
    tarch::la::Vector<Dimensions,int> getRelativeAdjacentCell( const tarch::la::Vector<Dimensions,double>& x ) const;

};



