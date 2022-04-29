#include "DataRepository.h"


peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorStack< examples::particles::celldata::ParticleSetCellStatistics >>   examples::particles::repositories::DataRepository::_CellDataParticleSetCellStatisticsStack;
peano4::maps::HierarchicalStackMap< peano4::stacks::STDVectorOverVectorOfPointersStack< examples::particles::vertexdata::ParticleSet >>   examples::particles::repositories::DataRepository::_VertexDataParticleSetStack;



void examples::particles::repositories::DataRepository::initDatatypes() {
  #ifdef Parallel
  examples::particles::celldata::ParticleSetCellStatistics::initDatatype();
examples::particles::vertexdata::ParticleSet::initDatatype();

  #endif
}


void examples::particles::repositories::DataRepository::shutdownDatatypes() {

  _CellDataParticleSetCellStatisticsStack.clear();
_VertexDataParticleSetStack.clear();


  #ifdef Parallel
  examples::particles::celldata::ParticleSetCellStatistics::shutdownDatatype();
examples::particles::vertexdata::ParticleSet::shutdownDatatype();

  #endif
}
