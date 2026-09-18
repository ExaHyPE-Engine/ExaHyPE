// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

constexpr int RungeKuttaSteps = {{RK_STEPS}};

/**
 * Butcher tableau of the solver's Runge-Kutta scheme.
 *
 * ButcherMatrix is flattened row major, i.e. entry (i,j) sits at
 * i * RungeKuttaSteps + j.
 */
class ButcherTableau final {
public:
  static void initButcherTableau();
  static void freeButcherTableau();

  inline static double* ButcherMatrix         = nullptr;
  inline static double* FinalEstimateWeights  = nullptr;
  inline static double* RelativeTimeStepSizes = nullptr;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
