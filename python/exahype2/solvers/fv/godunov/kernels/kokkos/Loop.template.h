// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% import 'Loop.template.h' as loop with context %}{# get template macros #}

{% macro loopOverPatches() %}
  const int     numberOfPatches                    = patchData.numberOfCells;
  const {{STORAGE_PRECISION}}* const QInFlattened  = patchData.QInFlattened;
  {{STORAGE_PRECISION}}* const       QOutFlattened = patchData.QOutFlattened;
  auto*         patchCentres                       = patchData.cellCentre;
  auto*         patchSizes                         = patchData.cellSize;
  const double  timeStamp                          = patchData.t;
  const double  timeStepSize                       = patchData.dt;

  double maxEigenvalueAfterTimeStep = 0.0;

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_reduce(
    "riemann-patch-loop",
    teamPolicy,
    KOKKOS_LAMBDA (const Kokkos::TeamPolicy<>::member_type& teamMember, {{COMPUTE_PRECISION}}& maxEigenvalueAfterTimeStep) {
    const int patchIndex                                    = teamMember.league_rank();
    const tarch::la::Vector<Dimensions, double> patchCentre = patchCentres[patchIndex];
    const tarch::la::Vector<Dimensions, double> patchSize   = patchSizes[patchIndex];

    const {{COMPUTE_PRECISION}}* const QIn = QInFlattened + (patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %});
    {{COMPUTE_PRECISION}}* const QOut      = QOutFlattened + (patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %});
    {% if DIMENSIONS == 3 %}
    Kokkos::parallel_for(
      Kokkos::TeamThreadMDRange(
        teamMember,
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}}
      ), [=, &maxEigenvalueAfterTimeStep](const int x, const int y, const int z) {
        const auto volume = ::exahype2::volumeIndex(x, y, z);
    {% else %}
    Kokkos::parallel_for(
      Kokkos::TeamThreadMDRange(
        teamMember,
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}}
      ), [=, &maxEigenvalueAfterTimeStep](const int x, const int y) {
        const auto volume = ::exahype2::volumeIndex(x, y);
    {% endif %}
        const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
        {{ riemann.createMemoryOnStack() | indent(4) }}
        {{ riemann.gatherQ() | indent(4) }}

        {{ loop.loopOverDimensions() | indent(4) }}

        {% if COMPUTE_MAX_EIGENVALUE==True %}
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }}
        {% endif %}

        {{ riemann.scatterQ() | indent(4) }}
    });
  }, Kokkos::Max<{{COMPUTE_PRECISION}}>(maxEigenvalueAfterTimeStep));

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
{% endmacro %}
