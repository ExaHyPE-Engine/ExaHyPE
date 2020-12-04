#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::swe::celldata::SWEQ >>   examples::exahype2::swe::observers::DataRepository::_CellDataSWEQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::swe::facedata::SWEQ >>   examples::exahype2::swe::observers::DataRepository::_FaceDataSWEQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::swe::facedata::SWEQNew >>   examples::exahype2::swe::observers::DataRepository::_FaceDataSWEQNewStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::swe::facedata::Label >>   examples::exahype2::swe::observers::DataRepository::_FaceDataLabelStack;



void examples::exahype2::swe::observers::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::exahype2::swe::celldata::SWEQ::initDatatype();
examples::exahype2::swe::facedata::SWEQ::initDatatype();
examples::exahype2::swe::facedata::SWEQNew::initDatatype();
examples::exahype2::swe::facedata::Label::initDatatype();

  #endif
}


void examples::exahype2::swe::observers::DataRepository::shutdownDatatypes() {

  _CellDataSWEQStack.clear();
_FaceDataSWEQStack.clear();
_FaceDataSWEQNewStack.clear();
_FaceDataLabelStack.clear();


  #ifdef Parallel
  examples::exahype2::swe::celldata::SWEQ::shutdownDatatype();
examples::exahype2::swe::facedata::SWEQ::shutdownDatatype();
examples::exahype2::swe::facedata::SWEQNew::shutdownDatatype();
examples::exahype2::swe::facedata::Label::shutdownDatatype();

  #endif
}
