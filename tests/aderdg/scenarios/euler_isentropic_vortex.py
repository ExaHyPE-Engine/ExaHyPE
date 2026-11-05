# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Euler


class EulerIsentropicVortex(Scenario):
    """
    Scenario reproduced from Ioratti, Dumbser & Loubère, https://doi.org/10.1007/s10915-020-01209-w (p. 43)
    """

    _offset = -5.0
    _domain_size = 10.0
    _end_time = 10.0
    _periodic_bc = True

    def __init__(self, velocity=1.0):
        self._dimensions = 2
        self._equation = Euler(dimensions=2, gamma=1.4)
        self._velocity = velocity

    def initial_conditions(self):
        return (
            """
  static constexpr auto epsilon = 5;
  static constexpr auto pi = std::numbers::pi;
  static constexpr auto eos_gamma = 1.4;

  const auto r = tarch::la::norm2(x);
  const auto du = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (- x[1]);
  const auto dv = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (x[0]);
  const auto dTemp = -(eos_gamma - 1.) * epsilon*epsilon / 8. / eos_gamma / pi / pi *
                 exp(1. - r * r);
  const auto drho = pow(1. + dTemp, 1. / (eos_gamma - 1.)) - 1.;
  const auto dp = pow(1. + dTemp, eos_gamma / (eos_gamma - 1.)) - 1.;

  constexpr auto rho_inf = 1;
  constexpr auto p_inf = 1;
  constexpr auto u_inf = """
            + str(self._velocity)
            + """;
  constexpr auto v_inf = """
            + str(self._velocity)
            + """;

  Q.rho  = rho_inf + drho;
  Q.v(0) = Q.rho * (u_inf + du);
  Q.v(1) = Q.rho * (v_inf + dv);
  Q.E    = (p_inf + dp) / (eos_gamma-1) + 0.5*Q.rho * ((u_inf+du)*(u_inf+du)+(v_inf+dv)*(v_inf+dv));
"""
        )

    def analytical_solution(self):
        return (
            """
  // The initial condition moves unchanged through the periodic domain [-5, 5]. The solution at x
  // is the initial value at x - v*t, wrapped by 10 into the domain.
  constexpr auto rho_inf = 1.;
  constexpr auto p_inf = 1.;
  constexpr auto u_inf = """
            + str(self._velocity)
            + """;
  constexpr auto v_inf = """
            + str(self._velocity)
            + """;

  const auto t_capped = std::fmod(t,10.);
  tarch::la::Vector<DIMENSIONS,double> pos = { (x[0] - u_inf*t_capped) < -5.0 ? (x[0] - u_inf*t_capped) + 10. : (x[0] - u_inf*t_capped),
                                               (x[1] - v_inf*t_capped) < -5.0 ? (x[1] - v_inf*t_capped) + 10. : (x[1] - v_inf*t_capped) };

  static constexpr auto epsilon = 5.0;
  static constexpr auto pi = std::numbers::pi;
  static constexpr auto eos_gamma = 1.4;

  const auto r = tarch::la::norm2(pos);
  const auto du = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (- pos[1]);
  const auto dv = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (pos[0]);
  const auto dTemp = -(eos_gamma - 1.) * epsilon*epsilon / 8. / eos_gamma / pi / pi *
                 exp(1. - r * r);
  const auto drho = pow(1. + dTemp, 1. / (eos_gamma - 1.)) - 1.;
  const auto dp = pow(1. + dTemp, eos_gamma / (eos_gamma - 1.)) - 1.;

  solution.rho  = rho_inf + drho;
  solution.v(0) = solution.rho * (u_inf + du);
  solution.v(1) = solution.rho * (v_inf + dv);
  solution.E    = (p_inf + dp) / (eos_gamma-1) + 0.5*solution.rho * ((u_inf+du)*(u_inf+du)+(v_inf+dv)*(v_inf+dv));
"""
        )
