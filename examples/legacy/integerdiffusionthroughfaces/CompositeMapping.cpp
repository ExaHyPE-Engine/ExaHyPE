#include "CompositeMapping.h"


examples::integerdiffusionthroughfaces::CompositeMapping::~CompositeMapping() {
  for (auto& p: _mappings) {
	delete p;
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::append( Mapping* mapping ) {
  assertion( mapping!=nullptr );
  _mappings.push_back(mapping);
}


void examples::integerdiffusionthroughfaces::CompositeMapping::beginTraversal() {
  for (auto& p: _mappings) {
    p->beginTraversal();
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::endTraversal() {
  for (auto& p: _mappings) {
    p->endTraversal();
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::createPersistentFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->createPersistentFace(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::createHangingFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->createHangingFace(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::destroyPersistentFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->destroyPersistentFace(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::destroyHangingFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->destroyHangingFace(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::touchFaceFirstTime(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->touchFaceFirstTime(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::touchFaceLastTime(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  for (auto& p: _mappings) {
	p->touchFaceLastTime(x,h,normal,data);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::createCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  for (auto& p: _mappings) {
	p->createCell(center,h,data,faces,coarseData,coarseFaces);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::destroyCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  for (auto& p: _mappings) {
    p->destroyCell(center,h,data,faces,coarseData,coarseFaces);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  for (auto& p: _mappings) {
	p->touchCellFirstTime(center,h,data,faces,coarseData,coarseFaces);
  }
}


void examples::integerdiffusionthroughfaces::CompositeMapping::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  for (auto& p: _mappings) {
	p->touchCellLastTime(center,h,data,faces,coarseData,coarseFaces);
  }
}

