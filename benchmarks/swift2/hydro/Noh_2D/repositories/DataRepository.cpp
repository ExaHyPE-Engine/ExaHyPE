#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics >>   benchmarks::swift2::noh::repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorOverContainerOfPointers< benchmarks::swift2::noh::vertexdata::hydroPartSet >>   benchmarks::swift2::noh::repositories::DataRepository::_VertexDatahydroPartSetStack;



void benchmarks::swift2::noh::repositories::DataRepository::initDatatypes() {
  #ifdef Parallel
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::initDatatype();
benchmarks::swift2::noh::vertexdata::hydroPartSet::initDatatype();

  #endif
}


void benchmarks::swift2::noh::repositories::DataRepository::shutdownDatatypes() {

  _CellDatahydroPartSetCellStatisticsStack.clear();
_VertexDatahydroPartSetStack.clear();


  #ifdef Parallel
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::shutdownDatatype();
benchmarks::swift2::noh::vertexdata::hydroPartSet::shutdownDatatype();

  #endif
}
