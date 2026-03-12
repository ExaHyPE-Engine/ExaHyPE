// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridStatistics.h"

#include "exahype2/RefinementControl.h"

#include "toolbox/loadbalancing/LoadBalancer.h"

{% for item in SOLVERS -%}
#include "{{ item[0] }}.h"
{% endfor %}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{% endfor %}

  {% for item in SOLVERS -%}
  extern {{ item[0] }} {{ item[1] }};
  {% endfor %}

  extern ::exahype2::RefinementControl refinementControl;

  extern toolbox::loadbalancing::LoadBalancer loadBalancer;

  static constexpr double LogTimeStepIntervalInSeconds = {{LOG_TIME_STEP_INTERVAL_IN_SECONDS}};

  /**
   * Return the global min time stamp over all solvers.
   *
   * There are two different flavours of a minimal time stamp: On the one hand,
   * there's a global minimum time stamp over all solvers. This might not
   * be the min time stamp after the last update. If you have local time
   * stepping, then some cells might just have done a tiny time step,
   * whereas the big cells still span a large time span. Hence, on the other
   * hand, there's also a (time-)local time stamp.
   */
  double getMinTimeStamp();

  /**
   * Return max time stamp of all solvers.
   *
   * This routine returns the minimum over the maximum time stamp of all
   * solvers. It is important that we use the minimum here: We usually want
   * to know how far in the future all the solvers are, but we still want to
   * know what a valid time stamp is. So we have to take the minimum here.
   */
  double getMaxTimeStamp();
  double getMinTimeStepSize();
  double getMaxTimeStepSize();
  double getMinMeshSize();
  double getMaxMeshSize();

  /**
   * This routine returns the minimum of the max admissible mesh sizes.
   */
  double getMaxAdmissibleMeshSize();

  /**
   * Return max of min admissible mesh sizes.
   */
  double getMinAdmissibleMeshSize();

  std::size_t getMaxNumberOfPatches();
  std::size_t getMaxNumberOfFaces();

  void startGridConstructionStep();
  void startGridInitialisationStep();

  /**
   * Start a new time step
   *
   * The solvers analyse their current solver state and toggle it. After
   * this routine is called, the solvers' states remain invariant until
   * the next startTimeStep() call or a suspension. Notably,
   * finishTimeStep() does not alter the solvers' states.
   */
  void startTimeStep();
  void startPlottingStep();

  void finishGridConstructionStep();
  void finishGridInitialisationStep();

  /**
   * @see startTimeStep()
   */
  void finishTimeStep();
  void finishPlottingStep();

  void logTimeStep();

  void startSimulation();
  void finishSimulation();

  bool mayPlot();

  /**
   * Is this the first sweep of a time step
   *
   * This query should be used with care within a user function and only once
   * you have studied isLastGridSweepOfTimeStep(). startTimeStep() switches
   * into this state. That is, when you call startTimeStep() and it turns out
   * to be the first sweep, the predicate will be true after startTimeStep()
   * has been called, and it will be true once the first mesh sweep has
   * terminated and the control comes back to the main routine. It is nice to
   * query stuff, but you should not "disturb" the solvers' mesh sweep
   * sequence. If you have a Runge-Kutta solver for example, the fact that this
   * predicate holds means that you've just finished the very first mesh
   * run-through of a time step, but the time step overall is not finished
   * yet.
   */
  bool isFirstGridSweepOfTimeStep();

  /**
   * Is this the last sweep of a time step
   *
   * Some solvers require multiple mesh sweeps to realise their behaviour.
   * Runge-Kutta solvers and enclave solvers are the most prominent examples.
   * Once you call startTimeStep() for the last time, the internal states will
   * toggle and this predicate will hold. After the time step has terminated
   * and finishTimeStep() has been called, this predicate holds. That is, you
   * can query it in the main and find out that a solver has just finished its
   * time step.
   */
  bool isLastGridSweepOfTimeStep();

  void suspendSolversForOneGridSweep();

  int getNumberOfDoFs();

  void initLogFilters();

  /**
   * Concatenates all the toString() results from the solvers.
   *
   * Gives you one long string of the states of the solvers. If you study the
   * states within the main of the simulation (the application driver so to
   * say), these states denote the states that the solvers will have in the
   * upcoming mesh sweep.
   */
  std::string getSolverStateDescriptions();

{% for item in NAMESPACE -%}
}
{%- endfor %}
