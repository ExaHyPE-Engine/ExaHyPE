// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "peano4/grid/Spacetree.h"

#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

{% for container in VERTEX_DATA_CONTAINERS -%}
::peano4::stacks::TreeToStacks<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}Stack;
::peano4::stacks::TreeToStacks<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStack;
{% endfor -%}
{% for container in FACE_DATA_CONTAINERS -%}
::peano4::stacks::TreeToStacks<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}Stack;
::peano4::stacks::TreeToStacks<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStack;
{% endfor -%}
{% for container in CELL_DATA_CONTAINERS -%}
::peano4::stacks::TreeToStacks<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}Stack;
{% endfor %}

std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localVertexDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalVertexDataMemoryUsageInBytes = 0;

void {{FULL_QUALIFIED_CLASSNAME}}::init() {
#ifdef WITH_MPI
{%- for container in VERTEX_DATA_CONTAINERS %}
  {{ container.fq }}::initDatatype();
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  {{ container.fq }}::initDatatype();
{%- endfor %}
{%- for container in CELL_DATA_CONTAINERS %}
  {{ container.fq }}::initDatatype();
{%- endfor %}
#endif

  const int numberOfMaxGlobalSpacetrees = loadBalancer.getMaxPermittedTreesPerRank() * tarch::mpi::Rank::getInstance().getNumberOfRanks();
  ::peano4::grid::Spacetree::_vertexStack.resize(numberOfMaxGlobalSpacetrees); // TODO: Resize it somewhere else or add static resize function to the spacetree class

{%- for container in VERTEX_DATA_CONTAINERS %}
  {{ container.name }}Stack.resize(numberOfMaxGlobalSpacetrees);
  {{ container.name }}HangingStack.resize(numberOfMaxGlobalSpacetrees);
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  {{ container.name }}Stack.resize(numberOfMaxGlobalSpacetrees);
  {{ container.name }}HangingStack.resize(numberOfMaxGlobalSpacetrees);
{%- endfor %}
{%- for container in CELL_DATA_CONTAINERS %}
  {{ container.name }}Stack.resize(numberOfMaxGlobalSpacetrees);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::shutdown() {
{%- for container in VERTEX_DATA_CONTAINERS %}
  {{ container.name }}Stack.clear();
  {{ container.name }}HangingStack.clear();
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  {{ container.name }}Stack.clear();
  {{ container.name }}HangingStack.clear();
{%- endfor %}
{%- for container in CELL_DATA_CONTAINERS %}
  {{ container.name }}Stack.clear();
{%- endfor %}

#ifdef WITH_MPI
{%- for container in VERTEX_DATA_CONTAINERS %}
  {{ container.fq }}::shutdownDatatype();
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  {{ container.fq }}::shutdownDatatype();
{%- endfor %}
{%- for container in CELL_DATA_CONTAINERS %}
  {{ container.fq }}::shutdownDatatype();
{%- endfor %}
#endif
}

void {{FULL_QUALIFIED_CLASSNAME}}::updateMemoryUsage() {
  _localCellDataMemoryUsageInBytes = 0;
  _localFaceDataMemoryUsageInBytes = 0;
  _localVertexDataMemoryUsageInBytes = 0;

{%- for container in CELL_DATA_CONTAINERS %}
  _localCellDataMemoryUsageInBytes += {{container.name}}Stack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  _localFaceDataMemoryUsageInBytes += {{container.name}}Stack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
  _localFaceDataMemoryUsageInBytes += {{container.name}}HangingStack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
{%- endfor %}
{%- for container in VERTEX_DATA_CONTAINERS %}
  _localVertexDataMemoryUsageInBytes += {{container.name}}Stack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
  _localVertexDataMemoryUsageInBytes += {{container.name}}HangingStack.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
{%- endfor %}

#ifdef WITH_MPI
  tarch::mpi::Rank::getInstance().allReduce(
    &_localCellDataMemoryUsageInBytes,
    &_globalCellDataMemoryUsageInBytes,
    1, MPI_UINT64_T,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &_localFaceDataMemoryUsageInBytes,
    &_globalFaceDataMemoryUsageInBytes,
    1, MPI_UINT64_T,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
  tarch::mpi::Rank::getInstance().allReduce(
    &_localVertexDataMemoryUsageInBytes,
    &_globalVertexDataMemoryUsageInBytes,
    1, MPI_UINT64_T,
    MPI_SUM,
    [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
  );
#else
  _globalCellDataMemoryUsageInBytes   = _localCellDataMemoryUsageInBytes;
  _globalFaceDataMemoryUsageInBytes   = _localFaceDataMemoryUsageInBytes;
  _globalVertexDataMemoryUsageInBytes = _localVertexDataMemoryUsageInBytes;
#endif
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getLocalCellDataMemoryUsageInBytes() {
  return _localCellDataMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getGlobalCellDataMemoryUsageInBytes() {
  return _globalCellDataMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getLocalFaceDataMemoryUsageInBytes() {
  return _localFaceDataMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getGlobalFaceDataMemoryUsageInBytes() {
  return _globalFaceDataMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getLocalVertexDataMemoryUsageInBytes() {
  return _localVertexDataMemoryUsageInBytes;
}

std::size_t {{FULL_QUALIFIED_CLASSNAME}}::getGlobalVertexDataMemoryUsageInBytes() {
  return _globalVertexDataMemoryUsageInBytes;
}
