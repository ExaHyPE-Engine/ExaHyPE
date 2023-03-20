#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStackOverSmartPointers< benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStackOverSmartPointers< benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStackOverSmartPointers< benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStackOverSmartPointers< benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStackOverSmartPointers< benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel >>   benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack;



void benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::initDatatypes() {
  #ifdef Parallel
  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::initDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel::initDatatype();

  #endif
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::DataRepository::shutdownDatatypes() {

  _CellDataSelfSimilarInfallFD4CellLabelStack.clear();
_CellDataSelfSimilarInfallFD4QStack.clear();
_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.clear();
_FaceDataSelfSimilarInfallFD4QOldStack.clear();
_FaceDataSelfSimilarInfallFD4QNewStack.clear();
_FaceDataSelfSimilarInfallFD4QUpdateStack.clear();
_FaceDataSelfSimilarInfallFD4FaceLabelStack.clear();


  #ifdef Parallel
  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::shutdownDatatype();
benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel::shutdownDatatype();

  #endif
}
