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
    [QIns, QOuts, patchCentres, patchSizes, timeStamps, timeStepSizes](auto idx) {
      {% if DIMENSIONS == 3 %}
      auto [patchIndex, x, y, z]                                    = idx;
      {% else %}
      auto [patchIndex, x, y]                                       = idx;
      {% endif %}
      const {{COMPUTE_PRECISION}}* const QIn                        = QIns[patchIndex];
      {{COMPUTE_PRECISION}}* const QOut                             = QOuts[patchIndex];
      const tarch::la::Vector<Dimensions, double>& patchCentre      = patchCentres[patchIndex];
      const tarch::la::Vector<Dimensions, double>& patchSize        = patchSizes[patchIndex];
      const double timeStamp                                        = timeStamps[patchIndex];
      const double timeStepSize                                     = timeStepSizes[patchIndex];
      {% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep                             = 0.0;
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
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      return maxEigenvalueAfterTimeStep;
      {% endif %}
    }
  );
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                                       = patchData.numberOfCells;
  {{STORAGE_PRECISION}}** const QIns                              = patchData.QIn;
  {{STORAGE_PRECISION}}** const QOuts                             = patchData.QOut;
  const tarch::la::Vector<Dimensions, double>* const patchCentres = patchData.cellCentre;
  const tarch::la::Vector<Dimensions, double>* const patchSizes   = patchData.cellSize;
  const double* const timeStamps                                  = patchData.t;
  const double* const timeStepSizes                               = patchData.dt;

  {{ loopOverVolumes() }}
{% endmacro %}
