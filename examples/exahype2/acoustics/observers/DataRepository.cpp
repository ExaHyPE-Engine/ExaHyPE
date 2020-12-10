#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::acoustics::celldata::AcousticsQ >>   examples::exahype2::acoustics::observers::DataRepository::_CellDataAcousticsQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::acoustics::facedata::AcousticsQ >>   examples::exahype2::acoustics::observers::DataRepository::_FaceDataAcousticsQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::acoustics::facedata::AcousticsQNew >>   examples::exahype2::acoustics::observers::DataRepository::_FaceDataAcousticsQNewStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::acoustics::facedata::Label >>   examples::exahype2::acoustics::observers::DataRepository::_FaceDataLabelStack;



void examples::exahype2::acoustics::observers::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::exahype2::acoustics::celldata::AcousticsQ::initDatatype();
examples::exahype2::acoustics::facedata::AcousticsQ::initDatatype();
examples::exahype2::acoustics::facedata::AcousticsQNew::initDatatype();
examples::exahype2::acoustics::facedata::Label::initDatatype();

  #endif
}


void examples::exahype2::acoustics::observers::DataRepository::shutdownDatatypes() {

  _CellDataAcousticsQStack.clear();
_FaceDataAcousticsQStack.clear();
_FaceDataAcousticsQNewStack.clear();
_FaceDataLabelStack.clear();


  #ifdef Parallel
  examples::exahype2::acoustics::celldata::AcousticsQ::shutdownDatatype();
examples::exahype2::acoustics::facedata::AcousticsQ::shutdownDatatype();
examples::exahype2::acoustics::facedata::AcousticsQNew::shutdownDatatype();
examples::exahype2::acoustics::facedata::Label::shutdownDatatype();

  #endif
}
