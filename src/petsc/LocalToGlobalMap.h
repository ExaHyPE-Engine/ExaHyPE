// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include <utility>


namespace petsc {
  class LocalToGlobalMap;
}


class petsc::LocalToGlobalMap {
  public:
    /**
     * Creates an empty map for tree treeNumber
     *
     * The code creates one global map per rank. This one has the tree number
     * -1 tied to it. Afterwards, the code runs through its local subdomains.
     * For this, each observer/sweep creates an instance of this object, builds
     * up its local map. After that, they merge() this local map into the
     * global one.
     *
     * @param treeNumber Number of tree where this map is constructed or -1 if
     *   it is the global map on this rank
     */
    LocalToGlobalMap(int treeNumber);

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
     */
    void merge( const LocalToGlobalMap& otherMap );

    /**
     * Maps the local index onto a global index
     *
     * Pass in the combination of tree number and local index and return the
     * global index according to the map.
     */
    int getGlobalIndex( const std::pair<int,int>& index );

    /**
     * Compute global to local map
     *
     * This operation is called once after all the indices are handed out.
     *
     */
    void computeLocalToGlobalMap();

    int getTotalNumberOfIndices();
};

