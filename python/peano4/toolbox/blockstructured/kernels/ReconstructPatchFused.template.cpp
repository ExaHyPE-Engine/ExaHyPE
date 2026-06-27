// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatchFused.h"

#include "peano4/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/ParallelFor.h"

{% from 'CopyPatchAndFillHalo.macro.h' import CopyPatchAndFillHalo with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  reconstructPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.QOutFlattened,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  const {{PRECISION}}* NOALIAS const  QInFlattened,
  const {{PRECISION}}* NOALIAS const  facePool,
  const std::uint64_t*                faceIndices,
  {{PRECISION}}* NOALIAS const        QOutFlattened,
  const int                           numberOfPatches,
  [[maybe_unused]] const int          targetDevice
) {
  ::tarch::accelerator::setCurrentDevice(targetDevice);

  ::tarch::parallelFor(
    ::tarch::TeamRange(numberOfPatches),
    [=] GPU_CALLABLE_LAMBDA (int patchIndex, ::tarch::accelerator::TeamHandle team) {
      const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
      auto* const QOut      = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

      ::tarch::parallelFor(
        team,
        ::tarch::RangeMD<{{DIMENSIONS}}>(
          {{DOFS_PER_AXIS}}, {{DOFS_PER_AXIS}}{% if DIMENSIONS==3 %}, {{DOFS_PER_AXIS}}{% endif %}
        ),
        [=](int x, int y{% if DIMENSIONS==3 %}, int z{% endif %}) {
          {{ CopyPatchAndFillHalo() | indent(10) }}
        }
      );
    }
  );
}
