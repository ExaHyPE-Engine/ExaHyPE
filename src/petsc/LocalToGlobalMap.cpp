#include "LocalToGlobalMap.h"
#include <iostream>

petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) : _treeNumber{treeNumber}, _totalNumberOfIndices{0} {
// petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) : _treeNumber{treeNumber}, _numberOfSubtrees{0}, _totalNumberOfIndices{0} {
  std::cout << "\ntreenumber = " << treeNumber << "\n\n";
}


void petsc::LocalToGlobalMap::setTreeNumber( int treeNumber ) {
  std::cout << "treeNumber was " << _treeNumber << "\n";
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

}


int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& index ) {
  return 15;
}


void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {

}


int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  return _totalNumberOfIndices;
}
