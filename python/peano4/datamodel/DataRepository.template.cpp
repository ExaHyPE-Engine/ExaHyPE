// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

{{DATA_CONTAINER_INSTANTIATION}}

std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalMemoryUsageInBytes = 0;

void {{FULL_QUALIFIED_CLASSNAME}}::init() {
#ifdef WITH_MPI
  {%- for container in DATA_CONTAINERS %}
    {{ container.fq }}::initDatatype();
  {%- endfor %}
#endif

  // TODO: Call init here for the maps
}

void {{FULL_QUALIFIED_CLASSNAME}}::shutdown() {
  {%- for container in DATA_CONTAINERS %}
    _{{ container.name }}Stack.clear();
  {%- endfor %}

#ifdef WITH_MPI
  {%- for container in DATA_CONTAINERS %}
    {{ container.fq }}::shutdownDatatype();
  {%- endfor %}
#endif
}

void {{FULL_QUALIFIED_CLASSNAME}}::updateMemoryUsage() {
  std::size_t localMemoryUsageInBytes = 0;

  {%- for container in DATA_CONTAINERS %}
    localMemoryUsageInBytes += _{{container.name}}Stack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
  {%- endfor %}

  _localMemoryUsageInBytes = localMemoryUsageInBytes;

#ifdef WITH_MPI
  tarch::mpi::Rank::getInstance().allReduce(
    &_localMemoryUsageInBytes,
    &_globalMemoryUsageInBytes,
    1, MPI_UINT64_T,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );

#if PEANO_DEBUG >= 1
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1) {
    assertion2(_globalMemoryUsageInBytes == _localMemoryUsageInBytes, _globalMemoryUsageInBytes, _localMemoryUsageInBytes);
  }
#endif
#else
  _globalMemoryUsageInBytes = _localMemoryUsageInBytes;
#endif
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getLocalMemoryUsageInBytes() {
  return _localMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getGlobalMemoryUsageInBytes() {
  return _globalMemoryUsageInBytes;
}
