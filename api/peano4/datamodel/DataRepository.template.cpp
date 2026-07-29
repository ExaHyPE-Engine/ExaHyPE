// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

{% for container in VERTEX_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer({{container.container_id}});
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStackContainer({{container.hanging_container_id}});
{% endfor -%}

{% for container in FACE_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer({{container.container_id}});
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}HangingStackContainer({{container.hanging_container_id}});
{% endfor -%}

{% for container in CELL_DATA_CONTAINERS -%}
::peano4::grid::StackContainer<{{container.stack}}> {{FULL_QUALIFIED_CLASSNAME}}::{{container.name}}StackContainer({{container.container_id}});
{% endfor %}

std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalCellDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalFaceDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_localVertexDataMemoryUsageInBytes = 0;
std::uint64_t {{FULL_QUALIFIED_CLASSNAME}}::_globalVertexDataMemoryUsageInBytes = 0;

namespace {
  constexpr int NumberOfDataMemoryWords = 3;

  /**
   * The sums of the preceding sweep, and whether they are still waiting to be published.
   */
  std::array<std::uint64_t, NumberOfDataMemoryWords> reducedDataMemory{};
  bool                                              hasPendingDataMemory = false;
} // namespace

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

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    // Publish the sums the preceding sweep reduced, so the global figures lag that sweep. The
    // first sweep has nothing to publish yet and falls back to the local view.
    if (hasPendingDataMemory) {
      _globalCellDataMemoryUsageInBytes   = reducedDataMemory[0];
      _globalFaceDataMemoryUsageInBytes   = reducedDataMemory[1];
      _globalVertexDataMemoryUsageInBytes = reducedDataMemory[2];
    } else {
      _globalCellDataMemoryUsageInBytes   = _localCellDataMemoryUsageInBytes;
      _globalFaceDataMemoryUsageInBytes   = _localFaceDataMemoryUsageInBytes;
      _globalVertexDataMemoryUsageInBytes = _localVertexDataMemoryUsageInBytes;
    }

    // Reduce this sweep's figures for the next one to publish. Every rank reaches this once per
    // sweep, right after its traversal has joined.
    reducedDataMemory[0] = _localCellDataMemoryUsageInBytes;
    reducedDataMemory[1] = _localFaceDataMemoryUsageInBytes;
    reducedDataMemory[2] = _localVertexDataMemoryUsageInBytes;
    tarch::mpi::allreduce(
      MPI_IN_PLACE,
      reducedDataMemory.data(),
      NumberOfDataMemoryWords,
      MPI_UINT64_T,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    hasPendingDataMemory = true;
  } else {
    _globalCellDataMemoryUsageInBytes   = _localCellDataMemoryUsageInBytes;
    _globalFaceDataMemoryUsageInBytes   = _localFaceDataMemoryUsageInBytes;
    _globalVertexDataMemoryUsageInBytes = _localVertexDataMemoryUsageInBytes;
  }
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
