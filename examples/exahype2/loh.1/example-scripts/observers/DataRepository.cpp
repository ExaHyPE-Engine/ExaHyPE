#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::loh1::celldata::LOH1Q >>   examples::exahype2::loh1::observers::DataRepository::_CellDataLOH1QStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel >>   examples::exahype2::loh1::observers::DataRepository::_CellDataLOH1CellSemaphoreLabelStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::loh1::facedata::LOH1Q >>   examples::exahype2::loh1::observers::DataRepository::_FaceDataLOH1QStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::loh1::facedata::LOH1QNew >>   examples::exahype2::loh1::observers::DataRepository::_FaceDataLOH1QNewStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::loh1::facedata::Label >>   examples::exahype2::loh1::observers::DataRepository::_FaceDataLabelStack;



void examples::exahype2::loh1::observers::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::exahype2::loh1::celldata::LOH1Q::initDatatype();
examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel::initDatatype();
examples::exahype2::loh1::facedata::LOH1Q::initDatatype();
examples::exahype2::loh1::facedata::LOH1QNew::initDatatype();
examples::exahype2::loh1::facedata::Label::initDatatype();

  #endif
}


void examples::exahype2::loh1::observers::DataRepository::shutdownDatatypes() {

  _CellDataLOH1QStack.clear();
_CellDataLOH1CellSemaphoreLabelStack.clear();
_FaceDataLOH1QStack.clear();
_FaceDataLOH1QNewStack.clear();
_FaceDataLabelStack.clear();


  #ifdef Parallel
  examples::exahype2::loh1::celldata::LOH1Q::shutdownDatatype();
examples::exahype2::loh1::celldata::LOH1CellSemaphoreLabel::shutdownDatatype();
examples::exahype2::loh1::facedata::LOH1Q::shutdownDatatype();
examples::exahype2::loh1::facedata::LOH1QNew::shutdownDatatype();
examples::exahype2::loh1::facedata::Label::shutdownDatatype();

  #endif
}
