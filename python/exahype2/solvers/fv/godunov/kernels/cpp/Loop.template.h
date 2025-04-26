// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% import 'Loop.template.h' as loop with context %}{# get template macros #}

{% macro loopOverVolumes() %}
  auto ids = std::views::cartesian_product(
    std::views::iota(0, numberOfPatches),
    std::views::iota(0, {{ NUMBER_OF_VOLUMES_PER_AXIS }}),
    std::views::iota(0, {{ NUMBER_OF_VOLUMES_PER_AXIS }})
    {% if DIMENSIONS == 3 %}
    ,
    std::views::iota(0, {{ NUMBER_OF_VOLUMES_PER_AXIS }})
    {% endif %}
  );

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  maxEigenvalueAfterTimeStep = std::transform_reduce(std::execution::par_unseq, ids.begin(), ids.end(),
    std::numeric_limits<{{COMPUTE_PRECISION}}>::lowest(), []({{COMPUTE_PRECISION}} a, {{COMPUTE_PRECISION}} b) { return std::fmax(a, b); },
  {% else %}
  std::for_each_n(std::execution::par_unseq, ids.begin(), std::ranges::size(ids),
  {% endif %}
    [QIns, QOuts, patchCentres, patchSizes, timeStamp, timeStepSize](auto idx) {
      {% if DIMENSIONS == 3 %}
      auto [patchIndex, x, y, z] = idx;
      {% else %}
      auto [patchIndex, x, y] = idx;
      {% endif %}

      const {{COMPUTE_PRECISION}}* const QIn = &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
      {{COMPUTE_PRECISION}}* const QOut      = &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

      const tarch::la::Vector<Dimensions, double> patchCentre {
        patchCentres[patchIndex * {{DIMENSIONS}}],
        patchCentres[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %}
        ,
        patchCentres[patchIndex * {{DIMENSIONS}} + 2]
        {% endif %}
      };
      const tarch::la::Vector<Dimensions, double> patchSize {
        patchSizes[patchIndex * {{DIMENSIONS}}],
        patchSizes[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %}
        ,
        patchSizes[patchIndex * {{DIMENSIONS}} + 2]
        {% endif %}
      };

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep = 0.0;
      {% endif %}

      {% if DIMENSIONS == 3 %}
      const auto volume = ::exahype2::volumeIndex(x, y, z);
      {% else %}
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

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      return maxEigenvalueAfterTimeStep;
      {% endif %}
    }
  );
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches               = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QIns = patchData.QIn1D;
  {{COMPUTE_PRECISION}}* const QOuts      = patchData.QOut1D;
  const double* const patchCentres        = patchData.cellCentre[0].data();
  const double* const patchSizes          = patchData.cellSize[0].data();
  const double timeStamp                  = patchData.t;
  const double timeStepSize               = patchData.dt;

  {{ loopOverVolumes() }}
{% endmacro %}
