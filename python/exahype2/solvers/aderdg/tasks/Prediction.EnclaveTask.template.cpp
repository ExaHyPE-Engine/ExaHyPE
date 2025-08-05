// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"

#include "kernels/{{SOLVER_NAME}}/FusedSpaceTimePredictorVolumeIntegral.h"
{% if USE_POINT_SOURCE %}
#include "kernels/{{SOLVER_NAME}}/PointSources.h"
#include "kernels/{{SOLVER_NAME}}/FusedSpaceTimePredictorVolumeIntegralWithPointSources.h"
{% endif %}

#include "exahype2/CellData.h"

#include "peano4/parallel/parallel.h"

#include <algorithm>
#include <cstring>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const peano4::datamanagement::CellMarker&       marker,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QOld,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS const   faces[2][TWO_TIMES_D],
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QNew
) {
  auto lQhbnd = faces[0];
  auto lFhbnd = faces[1];
  {{SPACE_TIME_PREDICTOR_CALL}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const peano4::datamanagement::CellMarker&       marker,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QOld,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS const   faces[2][TWO_TIMES_D],
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QNew
):
  ::exahype2::EnclaveTask<{{SOLUTION_STORAGE_PRECISION}}, 2>(
    _enclaveTaskTypeId,
    marker,
    timeStamp,
    timeStepSize,
    QOld,
    faces,
    QNew,
    [&]() -> void {
      applyKernelToCell(
        _marker,
        _t,
        _dt,
        _QIn,
        _faces,
        _QOut
      );
    }
  )
{
  setPriority({{ENCLAVE_TASK_PRIORITY}});
}

{% if CAN_FUSE %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(const std::list<Task*>& otherTasks) {
  const int numberOfCells = otherTasks.size() + 1;

  logInfo("fuse()", "Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << numberOfCells << " cells of type " << getTaskType() << " into "
    << {{NUMBER_OF_ENCLAVE_TASKS}} << " asynchronous enclave tasks on target device " << TARGET_DEVICE
  );

  ::exahype2::CellData<{{SOLUTION_STORAGE_PRECISION}}, {{SOLUTION_STORAGE_PRECISION}}, 2> cellData(
    numberOfCells,
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  int cellIndex = 0;
  for (auto& otherTask: otherTasks) {
    cellData.QIn[cellIndex]               = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QIn;
    for (int f = 0; f < TWO_TIMES_D; ++f) {
      cellData.faces[0][cellIndex][f]     = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[0][f];
      cellData.faces[1][cellIndex][f]     = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[1][f];
    }
    cellData.QOut[cellIndex]              = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QOut;
    cellData.cellSize[cellIndex]          = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.h();
    cellData.cellCentre[cellIndex]        = static_cast<Prediction{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.x();
    cellData.maxEigenvalue[cellIndex]     = 0.0;
    delete otherTask;
    cellIndex++;
  }
  cellData.t                              = _t;
  cellData.dt                             = _dt;
  cellData.QIn[cellIndex]                 = _QIn;
  for (int f = 0; f < TWO_TIMES_D; ++f) {
    cellData.faces[0][cellIndex][f]       = _faces[0][f];
    cellData.faces[1][cellIndex][f]       = _faces[1][f];
  }
  cellData.QOut[cellIndex]                = _QOut;
  cellData.cellCentre[cellIndex]          = _marker.x();
  cellData.cellSize[cellIndex]            = _marker.h();
  cellData.maxEigenvalue[cellIndex]       = 0.0;

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    auto* QOld = cellData.QIn[cellIndex];
    auto* QNew = cellData.QOut[cellIndex];
    auto lQhbnd = cellData.faces[0][cellIndex];
    auto lFhbnd = cellData.faces[1][cellIndex];

{% if IS_LINEAR %}
{% if USE_POINT_SOURCE %}
    const auto pointSources = kernels::{{SOLVER_NAME}}::pointSources(
      repositories::{{SOLVER_INSTANCE}},
      cellData.cellCentre[cellIndex],
      cellData.cellSize[cellIndex]
    );
    if (!pointSources.empty()) {
      // Linear, with point sources
      const auto _ = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegralWithPointSources<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd,
        lFhbnd,
        QNew,
        cellData.cellCentre[cellIndex],
        cellData.cellSize[cellIndex],
        cellData.t,
        cellData.dt,
        pointSources
      );
    } else {
      // Linear, without point sources
      const auto _ = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd,
        lFhbnd,
        QNew,
        cellData.cellCentre[cellIndex],
        cellData.cellSize[cellIndex],
        cellData.t,
        cellData.dt
      );
    }
{% else %}
    // Linear, without point sources
    const auto _ = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lQhbnd,
      lFhbnd,
      QNew,
      cellData.cellCentre[cellIndex],
      cellData.cellSize[cellIndex],
      cellData.t,
      cellData.dt
    );
{% endif %}
{% else %}
    // Nonlinear, without point sources
    const auto numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lQhbnd,
      lFhbnd,
      QNew,
      cellData.cellCentre[cellIndex],
      cellData.cellSize[cellIndex],
      cellData.t,
      cellData.dt
    );
{% endif %}
  }
}

{% endif %}
