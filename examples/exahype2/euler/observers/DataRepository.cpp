#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler::celldata::EulerQ >>   examples::exahype2::euler::observers::DataRepository::_CellDataEulerQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler::celldata::EulerCellSemaphoreLabel >>   examples::exahype2::euler::observers::DataRepository::_CellDataEulerCellSemaphoreLabelStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler::facedata::EulerQ >>   examples::exahype2::euler::observers::DataRepository::_FaceDataEulerQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler::facedata::EulerQNew >>   examples::exahype2::euler::observers::DataRepository::_FaceDataEulerQNewStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler::facedata::Label >>   examples::exahype2::euler::observers::DataRepository::_FaceDataLabelStack;



void examples::exahype2::euler::observers::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::exahype2::euler::celldata::EulerQ::initDatatype();
examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::initDatatype();
examples::exahype2::euler::facedata::EulerQ::initDatatype();
examples::exahype2::euler::facedata::EulerQNew::initDatatype();
examples::exahype2::euler::facedata::Label::initDatatype();

  #endif
}


void examples::exahype2::euler::observers::DataRepository::shutdownDatatypes() {

  _CellDataEulerQStack.clear();
_CellDataEulerCellSemaphoreLabelStack.clear();
_FaceDataEulerQStack.clear();
_FaceDataEulerQNewStack.clear();
_FaceDataLabelStack.clear();


  #ifdef Parallel
  examples::exahype2::euler::celldata::EulerQ::shutdownDatatype();
examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::shutdownDatatype();
examples::exahype2::euler::facedata::EulerQ::shutdownDatatype();
examples::exahype2::euler::facedata::EulerQNew::shutdownDatatype();
examples::exahype2::euler::facedata::Label::shutdownDatatype();

  #endif
}
