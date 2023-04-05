#include "LocalToGlobalMap.h"

/*

  let's start with a simple sequential indexing system 
  and then later we can worry about crossover!

*/


petsc::LocalToGlobalMap::LocalToGlobalMap(int treeNumber) {
  // std::cout << "\ntreenumber = " << treeNumber << "\n\n";
  _treeNumber = treeNumber;
}


void petsc::LocalToGlobalMap::setTreeNumber( int treeNumber ) {
  std::cout << "localtoglobalmap, treenumber was: " << _treeNumber << "\n";
  _treeNumber = treeNumber;
  std::cout << "setting tree number to " << treeNumber << "\n\n";
}

int petsc::LocalToGlobalMap::getTreeNumber() const {return _treeNumber;}

// int petsc::LocalToGlobalMap::getIndicesPerSubtree( int treeNumber ) const {
//   std::cout << "here\n";
//   return _indicesPerSubtree.at(treeNumber); 
// }

//we need to reserve some indices. best way to do that is to keep track 
//of the total number of indices and return the first avail requested
// index, ie the old value of _totalNumberOfIndices
int petsc::LocalToGlobalMap::reserveIndex(int cardinality) {
  
  std::cout << "calling reserveIndex inside spacetree " << this->getTreeNumber() << "\n";
  int firstIndex = _totalNumberOfIndices; // maybe + 1?
  _totalNumberOfIndices += cardinality;

  return firstIndex;

}


void petsc::LocalToGlobalMap::merge( const LocalToGlobalMap& otherMap ) {

  int treeNumber = otherMap.getTreeNumber();
  int numIndices = otherMap.getTotalNumberOfIndices();

  std::cout << "merging. treenumber, numindeices = " << treeNumber << " " << numIndices << "\n\n";

  //accumulate number of indices
  _totalNumberOfIndices += numIndices;

  //add entry for number of indices in this subtree
  // _indicesPerSubtree[treeNumber] = numIndices;

  //track total number of subtrees for global mapping
  _numberOfSubtrees++;
}

/*
  for now, this will just perform some "modular" arthimetic, and we worry about 
  crossover later. 
  QUESTION: how to find number of total indices in the tree?
    something to be done in localToGlobalMap
*/
// used in MapSolutionOntoMesh2petsc_actionsets_ProjectPETScSolutionOnVerticesBackOntoMesh0.cpp
int petsc::LocalToGlobalMap::getGlobalIndex( const std::pair<int,int>& index ) {
  std::cout << "we are calling getGlobalIndex inside the map with tree number = " << this->getTreeNumber() << "\n";
  std::cout << "local tree number, index = " << index.first << ", " << index.second << "\n\n";

  int globalIndex(0);
  
  //accumulate the number of indices "used up" by the previous subtrees.
  //index.first contains the local tree number 
  //assume that the other spacetree indices will be ordered 0, 1, 2, ....
  // for (int i = 0; i < index.first; i++){
  //   globalIndex += getIndicesPerSubtree( i );
  // }

  //now we've calculated the offset, add on the local index
  return 15;
}

// this occurs after finishing traversal
void petsc::LocalToGlobalMap::computeLocalToGlobalMap() {
  // should we do this only if _treeNumber == -1?
  std::cout << "\t checking when computeLocalToGlobalMap is called\n\n";
}


int petsc::LocalToGlobalMap::getTotalNumberOfIndices() const {
  std::cout << "\n\tasking for total number of indices = " << _totalNumberOfIndices << "\n";
  std::cout << "\tspacetree number is " << _treeNumber << "\n\n";
  return _totalNumberOfIndices;
}
