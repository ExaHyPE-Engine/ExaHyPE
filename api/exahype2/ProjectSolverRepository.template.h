// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "runner/RepositoryCallbacks.h"

{% for item in SOLVERS %}
#include "{{ item[0] }}.h"
{% endfor %}

namespace {{ NAMESPACE | join("::") }} {
  {% for item in SOLVERS %}
  extern {{ item[0] }} {{ item[1] }};
  {% endfor %}

  double getMinTimeStamp();
  double getMaxTimeStamp();
  double getMinTimeStepSize();
  double getMaxTimeStepSize();

  void startGridConstructionStep();
  void startGridInitialisationStep();
  void startDynamicMeshRefinementStep();

  void startTimeStep();
  void startPlottingStep();

  void finishGridConstructionStep();
  void finishGridInitialisationStep();
  void finishDynamicMeshRefinementStep();

  void finishTimeStep();
  void finishPlottingStep();

  void startSimulation();
  void finishSimulation();

  bool mayPlot();

  bool isFirstGridSweepOfTimeStep();
  bool isLastGridSweepOfTimeStep();

  void suspendSolversForOneGridSweep();

  int getNumberOfDoFs();

  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> getSolverStatistics();
  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> getParticleStatistics();

  std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> getConfigurationSummary();
} // namespace {{ NAMESPACE | join("::") }}
