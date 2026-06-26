// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
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

#ifdef PEANO_WITH_MPI
namespace {
  // State for the non-blocking, one-sweep-lagged data-memory reduction (see updateMemoryUsage).
  MPI_Request   dataMemoryReductionRequest  = MPI_REQUEST_NULL;
  bool          dataMemoryReductionInFlight = false;
  std::uint64_t dataMemoryReductionSend[3]  = {0, 0, 0};
  std::uint64_t dataMemoryReductionRecv[3]  = {0, 0, 0};
} // namespace
#endif

void {{FULL_QUALIFIED_CLASSNAME}}::init() {
#ifdef PEANO_WITH_MPI
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
#ifdef PEANO_WITH_MPI
  // Drain a still-in-flight lagged reduction so no request outlives MPI_Finalize.
  int mpiIsInitialised = 0;
  MPI_Initialized(&mpiIsInitialised);
  if (mpiIsInitialised) {
    int mpiIsFinalised = 0;
    MPI_Finalized(&mpiIsFinalised);
    if (not mpiIsFinalised and dataMemoryReductionInFlight and dataMemoryReductionRequest != MPI_REQUEST_NULL) {
      tarch::mpi::wait(&dataMemoryReductionRequest);
      dataMemoryReductionRequest  = MPI_REQUEST_NULL;
      dataMemoryReductionInFlight = false;
    }
  }
#endif
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

#ifdef PEANO_WITH_MPI
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

#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    auto progress = []() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };

    // Publish the reduction launched on the previous sweep (its result is one sweep stale). On the
    // first sweep nothing is in flight yet, so fall back to the local view.
    if (dataMemoryReductionInFlight) {
      tarch::mpi::wait(&dataMemoryReductionRequest, progress);
      dataMemoryReductionRequest  = MPI_REQUEST_NULL;
      dataMemoryReductionInFlight = false;
      _globalCellDataMemoryUsageInBytes   = dataMemoryReductionRecv[0];
      _globalFaceDataMemoryUsageInBytes   = dataMemoryReductionRecv[1];
      _globalVertexDataMemoryUsageInBytes = dataMemoryReductionRecv[2];
    } else {
      _globalCellDataMemoryUsageInBytes   = _localCellDataMemoryUsageInBytes;
      _globalFaceDataMemoryUsageInBytes   = _localFaceDataMemoryUsageInBytes;
      _globalVertexDataMemoryUsageInBytes = _localVertexDataMemoryUsageInBytes;
    }

    // Launch this sweep's reduction without blocking; the next sweep completes it. A blocking
    // collective here is a per-sweep barrier that deadlocks against the inter-rank tree
    // redistribution during grid construction at scale, so overlap it with the next traversal.
    dataMemoryReductionSend[0] = _localCellDataMemoryUsageInBytes;
    dataMemoryReductionSend[1] = _localFaceDataMemoryUsageInBytes;
    dataMemoryReductionSend[2] = _localVertexDataMemoryUsageInBytes;
    tarch::mpi::iallreduce(
      dataMemoryReductionSend,
      dataMemoryReductionRecv,
      3, MPI_UINT64_T,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      &dataMemoryReductionRequest
    );
    dataMemoryReductionInFlight = true;
  } else {
    _globalCellDataMemoryUsageInBytes   = _localCellDataMemoryUsageInBytes;
    _globalFaceDataMemoryUsageInBytes   = _localFaceDataMemoryUsageInBytes;
    _globalVertexDataMemoryUsageInBytes = _localVertexDataMemoryUsageInBytes;
  }
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
