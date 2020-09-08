#include "CompositeMapping.h"
#include "CellData.h"

#include "peano4/utils/Loop.h"


#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"




examples::delta::CompositeMapping::CompositeMapping() {
}


examples::delta::CompositeMapping::~CompositeMapping() {
  for (auto& p: _mappings) {
	delete p;
  }
}


void examples::delta::CompositeMapping::append( Mapping* mapping ) {
  assertion( mapping!=nullptr );
  _mappings.push_back(mapping);
}


void examples::delta::CompositeMapping::beginTraversal() {
  for (auto& p: _mappings) {
    p->beginTraversal();
  }
}


void examples::delta::CompositeMapping::endTraversal() {
  for (auto& p: _mappings) {
    p->endTraversal();
  }
}


void examples::delta::CompositeMapping::createCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  for (auto& p: _mappings) {
    p->createCell(center,h,data,coarseData);
  }
}


void examples::delta::CompositeMapping::destroyCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  for (auto& p: _mappings) {
    p->destroyCell(center,h,data,coarseData);
  }
}


void examples::delta::CompositeMapping::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  for (auto& p: _mappings) {
    p->touchCellFirstTime(center,h,data,coarseData);
  }
}


void examples::delta::CompositeMapping::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  CellData&                                    coarseData
)  {
  for (auto& p: _mappings) {
    p->touchCellLastTime(center,h,data,coarseData);
  }
}

