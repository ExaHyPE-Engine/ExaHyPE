#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <limits>

#include "PatchData.h"

#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"


visualisation::data::PatchData::PatchData(int dimensions_, double* offset_, double* size_, int dofsPerAxis, int unknownsPerDoF, int originTree_):
  dimensions(dimensions_),
  originTree(originTree_){
  for (int d=0; d<dimensions; d++) {
    offset[d]    = offset_[d];
    size[d]      = size_[d];
  }

  int points = tarch::la::aPowI(dimensions,dofsPerAxis) * unknownsPerDoF;

  data = new double[points];
}


bool visualisation::data::PatchData::samePatch( const PatchData& otherPatch ) {
  bool equal = true;

  for (int d=0; d<dimensions; d++) {
    equal &= tarch::la::equals( size[d], otherPatch.size[d] );
    equal &= tarch::la::equals( offset[d], otherPatch.offset[d] );
  }

  return equal;
}


bool visualisation::data::PatchData::overlaps( const PatchData& otherPatch ) {
  bool overlaps = true;

  for (int d=0; d<dimensions; d++) {
	overlaps &= tarch::la::smaller( offset[d], otherPatch.offset[d] + otherPatch.size[d] );
	overlaps &= tarch::la::greater( offset[d]+size[d], otherPatch.offset[d] );
  }

  return overlaps;
}


void visualisation::data::PatchData::copyData( const PatchData& otherData, int dofsPerAxis, int unknownsPerDoF ) {
  int points = tarch::la::aPowI(dimensions,dofsPerAxis) * unknownsPerDoF;
  for (int i=0; i<points; i++) {
    data[i] = otherData.data[i];
  }
}


void visualisation::data::PatchData::free() {
  if ( data!=nullptr ) {
    delete[] data;
    data = nullptr;
  }
}
