#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler2D::celldata::euler2DCellLabel >>   examples::exahype2::euler2D::repositories::DataRepository::_CellDataeuler2DCellLabelStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler2D::celldata::euler2DQ >>   examples::exahype2::euler2D::repositories::DataRepository::_CellDataeuler2DQStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler2D::facedata::euler2DFaceLabel >>   examples::exahype2::euler2D::repositories::DataRepository::_FaceDataeuler2DFaceLabelStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler2D::facedata::euler2DQEstimates >>   examples::exahype2::euler2D::repositories::DataRepository::_FaceDataeuler2DQEstimatesStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::exahype2::euler2D::facedata::euler2DQFluxEstimates >>   examples::exahype2::euler2D::repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack;



void examples::exahype2::euler2D::repositories::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::exahype2::euler2D::celldata::euler2DCellLabel::initDatatype();
examples::exahype2::euler2D::celldata::euler2DQ::initDatatype();
examples::exahype2::euler2D::facedata::euler2DFaceLabel::initDatatype();
examples::exahype2::euler2D::facedata::euler2DQEstimates::initDatatype();
examples::exahype2::euler2D::facedata::euler2DQFluxEstimates::initDatatype();

  #endif
}


void examples::exahype2::euler2D::repositories::DataRepository::shutdownDatatypes() {

  _CellDataeuler2DCellLabelStack.clear();
_CellDataeuler2DQStack.clear();
_FaceDataeuler2DFaceLabelStack.clear();
_FaceDataeuler2DQEstimatesStack.clear();
_FaceDataeuler2DQFluxEstimatesStack.clear();


  #ifdef Parallel
  examples::exahype2::euler2D::celldata::euler2DCellLabel::shutdownDatatype();
examples::exahype2::euler2D::celldata::euler2DQ::shutdownDatatype();
examples::exahype2::euler2D::facedata::euler2DFaceLabel::shutdownDatatype();
examples::exahype2::euler2D::facedata::euler2DQEstimates::shutdownDatatype();
examples::exahype2::euler2D::facedata::euler2DQFluxEstimates::shutdownDatatype();

  #endif
}
