// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

{% for container in VERTEX_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer;
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStackContainer;
{% endfor -%}

{% for container in FACE_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer;
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStackContainer;
{% endfor -%}

{% for container in CELL_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer;
{% endfor %}

std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localVertexDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalVertexDataMemoryUsageInBytes = 0;
std::size_t   {{FULL_QUALIFIED_CLASSNAME}}::_numberOfMaxGlobalSpacetrees = 0;

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
}

void {{FULL_QUALIFIED_CLASSNAME}}::shutdown() {
{%- for container in VERTEX_DATA_CONTAINERS %}
  {{ container.name }}StackContainer.clear();
  {{ container.name }}HangingStackContainer.clear();
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  {{ container.name }}StackContainer.clear();
  {{ container.name }}HangingStackContainer.clear();
{%- endfor %}
{%- for container in CELL_DATA_CONTAINERS %}
  {{ container.name }}StackContainer.clear();
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
  _localCellDataMemoryUsageInBytes += {{container.name}}StackContainer.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
{%- endfor %}
{%- for container in FACE_DATA_CONTAINERS %}
  _localFaceDataMemoryUsageInBytes += {{container.name}}StackContainer.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
  _localFaceDataMemoryUsageInBytes += {{container.name}}HangingStackContainer.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
{%- endfor %}
{%- for container in VERTEX_DATA_CONTAINERS %}
  _localVertexDataMemoryUsageInBytes += {{container.name}}StackContainer.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
  _localVertexDataMemoryUsageInBytes += {{container.name}}HangingStackContainer.size() * {{container.fq}}::Cardinality * sizeof({{container.fq}}::DoFType);
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

void {{FULL_QUALIFIED_CLASSNAME}}::setNumberOfMaxGlobalSpacetrees(std::size_t numberOfMaxGlobalSpacetrees) {
  _numberOfMaxGlobalSpacetrees = numberOfMaxGlobalSpacetrees;
}
