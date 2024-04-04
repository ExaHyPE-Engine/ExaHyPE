// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include <utility>
#include <map>

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"

namespace petsc {
  class LocalToGlobalMap;
}


class petsc::LocalToGlobalMap {
  public:

    enum class Type: int{
      Vertex=0, Cell=1, Face=2
    };

    static constexpr int RankGlobalTreeNumber = -1;

    /**
     * Is used by enumeration such that boundary Dirichlet points, for example,
     * have a properly initialised number, even though they do not carry an
     * unknown.
     */
    static constexpr int NoDegreeOfFreedom = -1;

    /**
     * Creates an empty map for tree treeNumber
     *
     * The code creates one global map per rank. This one has the tree number
     * -1 tied to it. Afterwards, the code runs through its local subdomains.
     * For this, each observer/sweep creates an instance of this object, builds
     * up its local map. After that, they merge() this local map into the
     * global one.
     *
     * @param treeNumber Number of tree where this map is constructed or
     *   RankGlobalTreeNumber if it is the global map on this rank
     */
    LocalToGlobalMap(int treeNumber = RankGlobalTreeNumber);

    void setTreeNumber( int treeNumber );
    
    int getTreeNumber() const;

    /**
     * Reserve some indices for unknowns and return the first one
     */
    int reserveIndex(int cardinality);

    /**
     * Merge two maps
     *
     * This operation accepts a second map, and merges otherMap into the
     * local map. Usually, the method is called for map -1, i.e. the map
     * that is held for the rank as a whole and is given the map of a
     * subdomain.
     *
     * We add an argument of "Type", an enum that denotes which kind of 
     * L2GM we are merging in. So, in the global map, we keep a count 
     * of how many vertices, cells and faces we saw. 
     *
     * This will allow us to later enumerate the facets one after 
     * another.
     */
    void merge( const LocalToGlobalMap& otherMap, Type type );

    /**
     * Maps the local index onto a global index
     *
     * Pass in the combination of tree number and local index and return the
     * global index according to the map.
     */
    int getGlobalIndex( const std::pair<int,int>& index, Type type );

    /**
     * Compute global to local map
     *
     * This operation is called once after all the indices are handed out.
     *
     */
    void computeLocalToGlobalMap();

    /*
    Increase count of indices we reserve for a particular facet type 
    (currently support vertices, faces, cells). Pass in the type and
    the integer
    */
    void increaseCountOfFacetType(Type type, int count);

    int getTotalNumberOfIndices() const;

    int getTotalNumberOfCells() const;
    
    /**
     * should only be called from main tree, ie with treenumber = -1
     * @param Number of tree where this map is constructed.
     * @returns the number of vertices within the tree labelled treeNumber
    */
    int getIndicesPerSubtree( int treeNumber ) const;

  
  private:
    int _treeNumber;

    /*
    these three should sum to the same
    as _totalNumberOfIndices
    */
    int _numberOfVertices;
    int _numberOfCells;
    int _numberOfFaces;
    
    // let's make this total number of indices in this tree specifically.
    // later, we use this to make sure the merge was successful
    int _totalNumberOfIndices;

    // std::vector<petsc::LocalToGlobalMap> _subTrees;
    int _numberOfSubtrees;

    /**
     * keeps track of the number of indices per subtree.
     * used for computing local to global map.
    */
    std::map<int,int> _indicesPerSubtree;

    static tarch::logging::Log  _log;

};

 
