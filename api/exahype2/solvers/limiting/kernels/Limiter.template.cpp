// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Limiter.h"

#include "Quadrature.h"

{% if USE_LIBXSMM %}
#include "Gemms.h"
{% endif %}

#include "tarch/accelerator/accelerator.h"

{% import 'Gemms.template.h' as m with context %}{# Get template macros #}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnFVLimiterSpaceWithHalo(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const       lim
) {
  // Compact projection without ghost layer
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int zy = 0; zy < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; zy++) {
    {{ m.gemm('dg2fv_x', 'luh', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpX',
      'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA,
      '0',
      'zy*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA_PADDED) | indent(4) }}{##}
  }

{%if DIMENSIONS==2 %}
  // y
  for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
    {{ m.gemm('dg2fv_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
      {{ m.gemm('dg2fv_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpY',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  for (int yx = 0; yx < {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER}}; yx++) {
    {{ m.gemm('dg2fv_z', 'tmpY', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'yx*'~NUMBER_OF_DATA_PADDED,
      '0',
      'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}

  // Space out for ghost layer (in place)
  for (int z = {{NUMBER_OF_DOF_LIMITER_3D-1}}; z >= 0; z--) {
    for (int y = {{NUMBER_OF_DOF_LIMITER-1}}; y >= 0; y--) {
      // No overlap since we have ghost layers at the beginning
      std::copy_n(
        lim + (z * {{NUMBER_OF_DOF_LIMITER}} + y) * {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA}},
        {{NUMBER_OF_DATA*NUMBER_OF_DOF_LIMITER}},
        lim + (((z + {{GHOST_LAYER_WIDTH_3D}}) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + y + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DOF_LIMITER + 2 * GHOST_LAYER_WIDTH}} + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DATA}}
      );
      // Zero the block that has just been copied, so the ghost layers hold zeros.
      std::memset(
        lim + (z * {{NUMBER_OF_DOF_LIMITER}} + y) * {{NUMBER_OF_DOF_LIMITER * NUMBER_OF_DATA}},
        0,
        sizeof({{LIMITER_SOLVER_STORAGE_PRECISION}}) * {{NUMBER_OF_DATA*NUMBER_OF_DOF_LIMITER}}
      );
    }
  }
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnFVLimiterSpaceWithoutHalo(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const       lim
) {
  // Compact projection without ghost layer
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int zy = 0; zy < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; zy++) {
    {{ m.gemm('dg2fv_x', 'luh', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpX',
      'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA,
      '0',
      'zy*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA_PADDED) | indent(4) }}{##}
  }

{%if DIMENSIONS==2 %}
  // y
  for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
    {{ m.gemm('dg2fv_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
      {{ m.gemm('dg2fv_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpY',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  for (int yx = 0; yx < {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER}}; yx++) {
    {{ m.gemm('dg2fv_z', 'tmpY', 'Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'yx*'~NUMBER_OF_DATA_PADDED,
      '0',
      'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnDGSpaceFromFVWithHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}* const lim,
  {{REGULAR_SOLVER_STORAGE_PRECISION}}* const       luh
) {
  // Ignore and remove ghost layers
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER_3D}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF_LIMITER_3D}}; z++) {
    for (int y = 0; y < {{NUMBER_OF_DOF_LIMITER}}; y++) {
      {{ m.gemm('fv2dg_x', 'lim', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'tmpX',
        '((z+'~GHOST_LAYER_WIDTH_3D~')*'~(NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH)~'+y+'~GHOST_LAYER_WIDTH~')*'~(NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH)*NUMBER_OF_DATA~'+'~GHOST_LAYER_WIDTH*NUMBER_OF_DATA,
        '0',
        '(z*'~NUMBER_OF_DOF_LIMITER~'+y)*'~NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }

{%if DIMENSIONS==2 %}
  // y
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('fv2dg_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'luh',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF_LIMITER}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      {{ m.gemm('fv2dg_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'tmpY',
        '(z*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  for (int yx = 0; yx < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; yx++) {
    {{ m.gemm('fv2dg_z', 'tmpY', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'luh',
    'yx*'~NUMBER_OF_DATA_PADDED,
    '0',
    'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnDGSpaceFromFVWithoutHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}* const lim,
  {{REGULAR_SOLVER_STORAGE_PRECISION}}* const       luh
) {
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER_3D}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF_LIMITER_3D}}; z++) {
    for (int y = 0; y < {{NUMBER_OF_DOF_LIMITER}}; y++) {
      {{ m.gemm('fv2dg_x', 'lim', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'tmpX',
        '(z*'~NUMBER_OF_DOF_LIMITER~'+y)*'~(NUMBER_OF_DOF_LIMITER)*NUMBER_OF_DATA,
        '0',
        '(z*'~NUMBER_OF_DOF_LIMITER~'+y)*'~NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }

{%if DIMENSIONS==2 %}
  // y
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('fv2dg_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'luh',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF_LIMITER}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      // The multiplication overwrites tmpY, so it needs no zero initialisation.
      {{ m.gemm('fv2dg_y', 'tmpX', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'tmpY',
        '(z*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  for (int yx = 0; yx < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; yx++) {
    {{ m.gemm('fv2dg_z', 'tmpY', 'Quadrature<{{COMPUTE_PRECISION}}>::fv2dg', 'luh',
      'yx*'~NUMBER_OF_DATA_PADDED,
      '0',
      'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnFVSpaceWithHaloFromFVWithoutHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}* const fvWithoutHalo,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const       fvWithHalo
) {
  for (int z = {{NUMBER_OF_DOF_LIMITER_3D-1}}; z >= 0; z--) {
    for (int y = {{NUMBER_OF_DOF_LIMITER-1}}; y >= 0; y--) {
      std::copy_n(
        fvWithoutHalo + (z * {{NUMBER_OF_DOF_LIMITER}} + y) * {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA}},
        {{NUMBER_OF_DATA*NUMBER_OF_DOF_LIMITER}},
        fvWithHalo + (((z + {{GHOST_LAYER_WIDTH_3D}}) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + y + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DATA}}
      );
    }
  }
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectOnFVSpaceWithoutHaloFromFVWithHalo(
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const       fvWithoutHalo,
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}* const fvWithHalo
) {
  for (int z = {{NUMBER_OF_DOF_LIMITER_3D-1}}; z >= 0; z--) {
    for (int y = {{NUMBER_OF_DOF_LIMITER-1}}; y >= 0; y--) {
      std::copy_n(
        fvWithHalo + (((z + {{GHOST_LAYER_WIDTH_3D}}) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + y + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + {{GHOST_LAYER_WIDTH}}) * {{NUMBER_OF_DATA}},
        {{NUMBER_OF_DATA*NUMBER_OF_DOF_LIMITER}},
        fvWithoutHalo + (z * {{NUMBER_OF_DOF_LIMITER}} + y) * {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA}}
      );
    }
  }
}

#include "peano4/Loop.h"

// Checks whether the solver classifies every node of the DG cell as physically admissible.
bool {{FULL_QUALIFIED_NAMESPACE}}::isPhysicallyAdmissible(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  const tarch::la::Vector<DIMENSIONS, double>&      cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const double                                      timeStamp
) {
  const tarch::la::Vector<DIMENSIONS, double>& bottomLeftPosition = cellCentre - 0.5 * cellSize;
  bool result = true;

  int linearisedIndex = 0;
  for (auto index = peano4::dStartVector(); peano4::dCompare(index, {{NUMBER_OF_DOF}}); peano4::dIncrement(index, {{NUMBER_OF_DOF}})) {
    tarch::la::Vector<DIMENSIONS, double> relativeNodePosition;
    relativeNodePosition(0) = Quadrature<{{COMPUTE_PRECISION}}>::nodes[index(0)];
    relativeNodePosition(1) = Quadrature<{{COMPUTE_PRECISION}}>::nodes[index(1)];
    #if DIMENSIONS==3
    relativeNodePosition(2) = Quadrature<{{COMPUTE_PRECISION}}>::nodes[index(2)];
    #endif

    result = result and solver.isPhysicallyAdmissible(
      luh + linearisedIndex,
      tarch::la::multiplyComponents(relativeNodePosition, cellSize) + bottomLeftPosition,
      cellSize,
      timeStamp
    );

    if (not result) {
      return false;
    }

    linearisedIndex += {{NUMBER_OF_DATA}};
  }

  return result;
}

bool {{FULL_QUALIFIED_NAMESPACE}}::discreteMaximumPrincipleAndMinAndMaxSearch(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  const {{COMPUTE_PRECISION}}                       relaxationParameter,
  const {{COMPUTE_PRECISION}}                       differenceScaling,
  {{COMPUTE_PRECISION}}*                            boundaryMinPerObservable,
  {{COMPUTE_PRECISION}}*                            boundaryMaxPerObservable
) {
  {{COMPUTE_PRECISION}} localMinPerObservable[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  {{COMPUTE_PRECISION}} localMaxPerObservable[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  // 1: Determine the new cell-local minimum and maximum
  findCellLocalMinAndMax(solver, luh, localMinPerObservable, localMaxPerObservable);

  // 2: Compare to the boundary minimum and maximum
  bool discreteMaximumPrincipleSatisfied=true;
  for (int v = 0; v < {{NUMBER_OF_OBSERVABLES}}; v++) {
    {{COMPUTE_PRECISION}} boundaryMin = boundaryMinPerObservable[v];
    for (int i = 1; i < {{2*DIMENSIONS}}; i++) {
      boundaryMin = std::min(boundaryMin, boundaryMinPerObservable[i * {{NUMBER_OF_OBSERVABLES}} + v]);
    }
    {{COMPUTE_PRECISION}} boundaryMax = boundaryMaxPerObservable[v];
    for (int i = 1; i < {{2*DIMENSIONS}}; i++) {
      boundaryMax = std::max(boundaryMax, boundaryMaxPerObservable[i * {{NUMBER_OF_OBSERVABLES}} + v]);
    }

    const {{COMPUTE_PRECISION}} scaledRelaxationParameter =
      solver.getDiscreteMaximumPrincipleRelaxationParameter(
        relaxationParameter,
        v,
        localMinPerObservable[v],
        localMaxPerObservable[v],
        boundaryMin,
        boundaryMax
      );
    {{COMPUTE_PRECISION}} scaledDifference = (boundaryMax - boundaryMin) * differenceScaling;
    scaledDifference = std::max(scaledDifference, scaledRelaxationParameter);

    if ((localMinPerObservable[v] < (boundaryMin - scaledDifference)) or
        (localMaxPerObservable[v] > (boundaryMax + scaledDifference))) {
      discreteMaximumPrincipleSatisfied = false;
    }

    // Check for NaN and infinity values
    discreteMaximumPrincipleSatisfied = discreteMaximumPrincipleSatisfied and
                                        std::isfinite(localMinPerObservable[v]) and
                                        std::isfinite(localMaxPerObservable[v]);

    // The block for face 0 carries the cell-local minimum and maximum.
    boundaryMinPerObservable[v] = localMinPerObservable[v];
    boundaryMaxPerObservable[v] = localMaxPerObservable[v];

    // The block for face 1 carries the boundary minimum and maximum.
    boundaryMinPerObservable[v + {{NUMBER_OF_OBSERVABLES}}] = boundaryMin;
    boundaryMaxPerObservable[v + {{NUMBER_OF_OBSERVABLES}}] = boundaryMax;
  }

  return discreteMaximumPrincipleSatisfied;
}

void {{FULL_QUALIFIED_NAMESPACE}}::findCellLocalLimiterMinAndMax(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}* const lim,
  {{COMPUTE_PRECISION}}* const                      localMinPerObservable,
  {{COMPUTE_PRECISION}}* const                      localMaxPerObservable
) {
  std::fill_n(localMinPerObservable, {{NUMBER_OF_OBSERVABLES}}, +std::numeric_limits<{{COMPUTE_PRECISION}}>::infinity());
  std::fill_n(localMaxPerObservable, {{NUMBER_OF_OBSERVABLES}}, -std::numeric_limits<{{COMPUTE_PRECISION}}>::infinity());

  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  for (int z = {{GHOST_LAYER_WIDTH_3D}}; z < {{NUMBER_OF_DOF_LIMITER_3D+GHOST_LAYER_WIDTH_3D}}; z++) { // Skip the ghost layers
    for (int y = {{GHOST_LAYER_WIDTH}}; y < {{NUMBER_OF_DOF_LIMITER+GHOST_LAYER_WIDTH}}; y++) {
      for (int x = {{GHOST_LAYER_WIDTH}}; x < {{NUMBER_OF_DOF_LIMITER+GHOST_LAYER_WIDTH}}; x++) {
        solver.mapDiscreteMaximumPrincipleObservables(
          observables,
          lim + (((z * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + y) * {{NUMBER_OF_DOF_LIMITER+2*GHOST_LAYER_WIDTH}} + x) * {{NUMBER_OF_DATA}})
        );

        for (int v = 0; v < {{NUMBER_OF_OBSERVABLES}}; v++) {
          localMinPerObservable[v] = std::min(localMinPerObservable[v], observables[v]);
          localMaxPerObservable[v] = std::max(localMaxPerObservable[v], observables[v]);
        }
      }
    }
  }
}

void {{FULL_QUALIFIED_NAMESPACE}}::findCellLocalMinAndMax(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{COMPUTE_PRECISION}}* const                      localMinPerObservable,
  {{COMPUTE_PRECISION}}* const                      localMaxPerObservable
) {
  std::fill_n(localMinPerObservable, {{NUMBER_OF_OBSERVABLES}}, +std::numeric_limits<{{COMPUTE_PRECISION}}>::infinity());
  std::fill_n(localMaxPerObservable, {{NUMBER_OF_OBSERVABLES}}, -std::numeric_limits<{{COMPUTE_PRECISION}}>::infinity());

  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  for (int zyx = 0; zyx < {{NUMBER_OF_DOF**DIMENSIONS}}; zyx++) {
    solver.mapDiscreteMaximumPrincipleObservables(
      observables,
      luh + (zyx * {{NUMBER_OF_DATA}})
    );

    for (int v = 0; v < {{NUMBER_OF_OBSERVABLES}}; v++) {
      localMinPerObservable[v] = std::min(localMinPerObservable[v], observables[v]);
      localMaxPerObservable[v] = std::max(localMaxPerObservable[v], observables[v]);
    }
  }

  compareWithADERDGSolutionAtGaussLobattoNodes(solver, luh, localMinPerObservable, localMaxPerObservable);
  compareWithADERDGSolutionAtFVSubcellCentres(solver, luh, localMinPerObservable, localMaxPerObservable);
}

// Project onto the Gauss-Lobatto nodes and update the minimum and maximum if required
void {{FULL_QUALIFIED_NAMESPACE}}::compareWithADERDGSolutionAtGaussLobattoNodes(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{COMPUTE_PRECISION}}*                            min,
  {{COMPUTE_PRECISION}}*                            max
) {
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int zy = 0; zy < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; zy++) {
    {{ m.gemm('uh2lob_x', 'luh', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::uh2lob', 'tmpX',
      'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA,
      '0',
      'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA_PADDED) | indent(4) }}{##}
  }

{%if DIMENSIONS==2 %}
  // y
  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  // Constant x slice of projected solution
  {{COMPUTE_PRECISION}}* lob = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DATA_PADDED}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('uh2lob_y_slice', 'tmpX', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::uh2lob', 'lob',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      '0') | indent(4) }}{##}

    for (int p = 0; p < {{NUMBER_OF_DOF}}; p++) {
      solver.mapDiscreteMaximumPrincipleObservables(observables, lob + p * {{NUMBER_OF_DATA_PADDED}});
      for (int n = 0; n < {{NUMBER_OF_OBSERVABLES}}; n++) {
        min[n] = std::min(min[n], observables[n]);
        max[n] = std::max(max[n], observables[n]);
      }
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
  tarch::freeMemory(lob, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      {{ m.gemm('uh2lob_y', 'tmpX', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::uh2lob', 'tmpY',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  // Constant x,y slice of projected solution
  {{COMPUTE_PRECISION}}* lob = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int yx = 0; yx < {{NUMBER_OF_DOF}}; yx++) {
    {{ m.gemm('uh2lob_z_slice', 'tmpY', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::uh2lob', 'lob',
      'yx*'~NUMBER_OF_DATA_PADDED,
      '0',
      '0') | indent(4) }}{##}
    for (int p = 0; p < {{NUMBER_OF_DOF}}; p++) {
      solver.mapDiscreteMaximumPrincipleObservables(observables, lob + p * {{NUMBER_OF_DATA_PADDED}});
      for (int n = 0; n < {{NUMBER_OF_OBSERVABLES}}; n++) {
        min[n] = std::min(min[n], observables[n]);
        max[n] = std::max(max[n], observables[n]);
      }
    }
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
  tarch::freeMemory(lob, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}
}

// Project onto the FV subcell nodes and update the minimum and maximum if required
void {{FULL_QUALIFIED_NAMESPACE}}::compareWithADERDGSolutionAtFVSubcellCentres(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{COMPUTE_PRECISION}}*                            min,
  {{COMPUTE_PRECISION}}*                            max
) {
  // x
  {{COMPUTE_PRECISION}}* tmpX = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int zy = 0; zy < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; zy++) {
    {{ m.gemm('dg2fv_x', 'luh', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpX',
      'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA,
      '0',
      'zy*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DATA_PADDED) | indent(4) }}{##}
  }

{%if DIMENSIONS==2 %}
  // y
  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  {{COMPUTE_PRECISION}}* lim = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
    {{ m.gemm('dg2fv_y_slice', 'tmpX', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'x*'~NUMBER_OF_DATA_PADDED,
      '0',
      '0') | indent(4) }}{##}
    for (int p = 0; p < {{NUMBER_OF_DOF_LIMITER}}; p++) {
      solver.mapDiscreteMaximumPrincipleObservables(observables, lim + p * {{NUMBER_OF_DATA_PADDED}});
      for (int n = 0; n < {{NUMBER_OF_OBSERVABLES}}; n++) {
        min[n] = std::min(min[n], observables[n]);
        max[n] = std::max(max[n], observables[n]);
      }
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
  tarch::freeMemory(lim, tarch::MemoryLocation::Heap);
{% else %}
  // y
  {{COMPUTE_PRECISION}}* tmpY = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF_LIMITER}}; x++) {
      {{ m.gemm('dg2fv_y', 'tmpX', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'tmpY',
        '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED,
        '0',
        '(z*'~NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER~'+x)*'~NUMBER_OF_DATA_PADDED) | indent(6) }}{##}
    }
  }
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);

  // z
  {{COMPUTE_PRECISION}} observables[{{NUMBER_OF_OBSERVABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  // Constant x,y slice of projected solution
  {{COMPUTE_PRECISION}}* lim = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DATA_PADDED}}, tarch::MemoryLocation::Heap);
  for (int yx = 0; yx < {{NUMBER_OF_DOF_LIMITER*NUMBER_OF_DOF_LIMITER}}; yx++) {
    {{ m.gemm('dg2fv_z_slice', 'tmpY', '{{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::dg2fv', 'lim',
      'yx*'~NUMBER_OF_DATA_PADDED,
      '0',
      '0') | indent(4) }}{##}
    for (int p = 0; p < {{NUMBER_OF_DOF_LIMITER}}; p++) {
      solver.mapDiscreteMaximumPrincipleObservables(observables, lim + p * {{NUMBER_OF_DATA_PADDED}});
      for (int n = 0; n < {{NUMBER_OF_OBSERVABLES}}; n++) {
        min[n] = std::min(min[n], observables[n]);
        max[n] = std::max(max[n], observables[n]);
      }
    }
  }
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
  tarch::freeMemory(lim, tarch::MemoryLocation::Heap);
{% endif %}{# DIMENSIONS==2 #}
}
