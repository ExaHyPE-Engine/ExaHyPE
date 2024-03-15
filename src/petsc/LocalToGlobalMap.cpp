#include "LocalToGlobalMap.h"

tarch::logging::Log  petsc::LocalToGlobalMap::_log( "petsc::LocalToGlobalMap" );

petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) : _treeNumber{treeNumber}, _totalNumberOfIndices{0},
  _numberOfVertices{0},
  _numberOfCells{0},
  _numberOfFaces{0}
{}

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


void petsc::LocalToGlobalMap::merge( const LocalToGlobalMap& otherMap, Type type = Type::Vertex ) {
  logTraceInWith2Arguments("merge", otherMap.getTotalNumberOfIndices(), (int)type);

  //increase our count of the number of indices globally
  _totalNumberOfIndices += otherMap.getTotalNumberOfIndices();

  //increase our count of indices for the type that we see
  increaseCountOfFacetType(type, otherMap.getTotalNumberOfIndices());

  int otherTreeNumber = otherMap.getTreeNumber();
  int otherIndices    = otherMap.getTotalNumberOfIndices();

  //add these new indices to the count
  _indicesPerSubtree.insert( std::make_pair( otherTreeNumber, otherIndices ) );

  logTraceOutWith4Arguments("merge", _totalNumberOfIndices, _numberOfVertices, _numberOfCells, _numberOfFaces);
}

int petsc::LocalToGlobalMap::getIndicesPerSubtree( int treeNumber ) const {
  return _indicesPerSubtree.at( treeNumber );
}

int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& index, Type type ) {
  int localTreeNumber = index.first;
  int localIndex      = index.second;
  
  //to be returned:
  int globalIndex(0);

  //let's count the indices that were used up by other subtrees:
  for (int i=0; i<localTreeNumber; i++){
    globalIndex += getIndicesPerSubtree( i );
  }

  logTraceInWith4Arguments("getGlobalIndex", index.first, index.second, (int)type, globalIndex);
  
  // add in offsets based on the types of indices we store
  if ( type > Type::Vertex )
    globalIndex += _numberOfVertices;
  if ( type > Type::Cell )
    globalIndex += _numberOfCells;
  if ( type > Type::Face )
    globalIndex += _numberOfFaces;

  /*
  By this point, our globalIndex should be set to the starting
  index of the type of facet we store. Eg if there are 
  10 vertices, 10 cells and 10 faces, and we are looking for a
  face, our globalIndex should be set to 20, since we just 
  skipped over all 10 vertices and all 10 faces.

  Now, we just need to add on the extra offset and return
  */
  globalIndex += localIndex;
  
  logTraceOutWith1Argument("getGlobalIndex", globalIndex);
  return globalIndex;
}

// this occurs after finishing traversal
void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {
  logTraceInWith3Arguments("computeLocalToGlobalMap", _numberOfVertices, _numberOfCells, _numberOfFaces);
  logTraceOut("computeLocalToGlobalMap");
}

void petsc::LocalToGlobalMap::increaseCountOfFacetType(Type type, int count)
{
  logTraceInWith3Arguments("increaseCountOfFacetType", _numberOfVertices, _numberOfCells, _numberOfFaces);
  switch (type)
  {
    case Type::Vertex :
    {
      _numberOfVertices += count;
    }
    break;

    case Type::Cell :
    {
      _numberOfCells += count;
    }
    break;

    case Type::Face :
    {
      _numberOfFaces += count;
    }
    break;
  }
  logTraceOutWith3Arguments("increaseCountOfFacetType", _numberOfVertices, _numberOfCells, _numberOfFaces);
}

int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  return _totalNumberOfIndices;
}

int petsc::LocalToGlobalMap::getTotalNumberOfCells() const {
  return _numberOfCells;
}