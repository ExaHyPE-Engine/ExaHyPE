#include "LocalToGlobalMap.h"

petsc::LocalToGlobalMap::LocalToGlobalMap(int) {}

void petsc::LocalToGlobalMap::setTreeNumber( int ) {}

int petsc::LocalToGlobalMap::reserveIndex(int) {
  return 0;
}

void petsc::LocalToGlobalMap::merge( const LocalToGlobalMap& ) {}

int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& ) {
  return 15;
}

void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {}

int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  return 23;
}
