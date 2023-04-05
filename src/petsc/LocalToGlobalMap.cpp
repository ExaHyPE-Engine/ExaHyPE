#include "LocalToGlobalMap.h"
#include <iostream>

petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) : _treeNumber(treeNumber) {
  std::cout << "\ntreenumber = " << treeNumber << "\n\n";
}


void petsc::LocalToGlobalMap::setTreeNumber( int treeNumber ) {
  std::cout << "treeNumber was " << _treeNumber << "\n";
  _treeNumber = treeNumber;

}

int petsc::LocalToGlobalMap::getTreeNumber() const { return _treeNumber; }

int petsc::LocalToGlobalMap::reserveIndex(int cardinality) {

}


void petsc::LocalToGlobalMap::merge( const LocalToGlobalMap& otherMap ) {

}


int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& index ) {
  return 15;
}


void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {

}


int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  return 23;
}
