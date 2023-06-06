#include "LocalToGlobalMap.h"


petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) {

}


void petsc::LocalToGlobalMap::setTreeNumber( int treeNumber ) {

}


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
