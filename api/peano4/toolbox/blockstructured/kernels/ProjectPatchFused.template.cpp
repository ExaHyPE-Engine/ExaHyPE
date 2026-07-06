// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchFused.h"

#include "peano4/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/ParallelFor.h"

{% from 'CopyPatchOntoFaces.macro.h' import CopyPatchOntoFaces with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  projectPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices,
  const int                          numberOfPatches,
  [[maybe_unused]] const int         targetDevice
) {
  traceEnter();
  traceEnter("Patch projection");
  ::tarch::accelerator::setCurrentDevice(targetDevice);

  ::tarch::parallelFor(
    ::tarch::TeamRange(numberOfPatches),
    [=] GPU_CALLABLE_LAMBDA (int patchIndex, ::tarch::accelerator::TeamHandle team) {
      const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];

      ::tarch::parallelFor(
        team,
        ::tarch::RangeMD<{{DIMENSIONS}}>(
          {{DOFS_PER_AXIS}}, {{DOFS_PER_AXIS}}{% if DIMENSIONS==3 %}, {{DOFS_PER_AXIS}}{% endif %}
        ),
        [=](int x, int y{% if DIMENSIONS==3 %}, int z{% endif %}) {
          {{ CopyPatchOntoFaces() | indent(10) }}
        }
      );
    }
  );
  traceLeave("Patch projection");
  traceLeave();
}
