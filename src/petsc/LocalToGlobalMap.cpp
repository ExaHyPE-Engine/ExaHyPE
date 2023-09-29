#include "LocalToGlobalMap.h"

petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) : _treeNumber{treeNumber}, _totalNumberOfIndices{0} {
}

void petsc::LocalToGlobalMap::setTreeNumber( int treeNumber ) {
  _treeNumber = treeNumber;
}

int petsc::LocalToGlobalMap::getTreeNumber() const { return _treeNumber; }

int petsc::LocalToGlobalMap::reserveIndex(int cardinality) {

  // get current number of indices in this tree, so we can return it,
  // as this will be the next available index
  int firstIndex = getTotalNumberOfIndices();

  //increment number of indices in this tree
  _totalNumberOfIndices += cardinality;

  return firstIndex;
}


void petsc::LocalToGlobalMap::merge( const LocalToGlobalMap& otherMap ) {
  //increase our count of the number of indices globally
  _totalNumberOfIndices += otherMap.getTotalNumberOfIndices();

  int otherTreeNumber = otherMap.getTreeNumber();
  int otherIndices = otherMap.getTotalNumberOfIndices();

  //add these new indices to the count
  _indicesPerSubtree.insert( std::make_pair( otherTreeNumber, otherIndices ) );
}

int petsc::LocalToGlobalMap::getIndicesPerSubtree( int treeNumber ) const {
  return _indicesPerSubtree.at( treeNumber );
}

// used in MapSolutionOntoMesh2petsc_actionsets_ProjectPETScSolutionOnVerticesBackOntoMesh0.cpp
/**
* TODO - change it so this routine is completed during computeLocalToGlobalMap stage, so
* we can just read off a value here.
*/
int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& index ) {

  int localTreeNumber = index.first;
  int localIndex = index.second;
  
  //to be returned:
  int globalIndex(0);

  //let's count the indices that were used up by other subtrees:
  for (int i=0; i<localTreeNumber; i++){
    globalIndex += getIndicesPerSubtree( i );
  }

  //add on the offset in the local subtree, and then return
  globalIndex += localIndex;
  return globalIndex;
}

// this occurs after finishing traversal
void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {
}


int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  assertion(_totalNumberOfIndices<=4);
  return _totalNumberOfIndices;
}
