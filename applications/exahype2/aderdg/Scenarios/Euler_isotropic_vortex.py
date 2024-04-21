from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Euler

class Euler_isotropic_vortex(Scenario):

    """
    Scenario reproduced from Ioratti, Dumbser & Loubère, https://doi.org/10.1007/s10915-020-01209-w (p. 43)
    """

    _plot_dt     = 0.0
    _offset      = -5.
    _domain_size = 10.
    _end_time    = 10.0
    _periodic_bc = True

    def __init__(
        self
    ):
        self._dimensions  = 2
        self._equation    = Euler(dimensions=2, gamma=1.4)

    def initial_conditions(self):
        return """
  static constexpr double epsilon = 5.0;
  static constexpr double pi = std::numbers::pi;
  static constexpr double i_pi = 1./pi;
  static constexpr double eos_gamma = 1.4;

  const double r = tarch::la::norm2(x);
  const double du = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (- x[1]);
  const double dv = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (x[0]);
  const double dTemp = -(eos_gamma - 1.) * epsilon*epsilon / 8. / eos_gamma / pi / pi *
                 exp(1. - r * r);
  const double drho = pow(1. + dTemp, 1. / (eos_gamma - 1.)) - 1.;
  const double dp = pow(1. + dTemp, eos_gamma / (eos_gamma - 1.)) - 1.;

  constexpr double rho_inf = 1.;
  constexpr double p_inf = 1.;
  constexpr double u_inf = 1.;
  constexpr double v_inf = 1.;

	Q[0] = rho_inf + drho; // fluid density
	Q[1] = Q[0] * (u_inf + du); // momentum
	Q[2] = Q[0] * (v_inf + dv);
	// total energy = internal energy + kinetic energy
	Q[3] = (p_inf + dp) / (eos_gamma-1) + 0.5*Q[0] * ((u_inf+du)*(u_inf+du)+(v_inf+dv)*(v_inf+dv));
"""

    def analytical_solution(self):
        return """
  /*
    The initial condition is transported without deformation in a periodic
    domain [-5., +5.], i.e. the value at a given point is the value at the
    position x - v*t but accounting for periodicity.
    Therefore we find the point x - v*t, and then shift this by instances
    of 10.0 until we find the point within the domain. 
  */
  constexpr double rho_inf = 1.;
  constexpr double p_inf = 1.;
  constexpr double u_inf = 1.;
  constexpr double v_inf = 1.;

  const double t_capped = t  - (int) t / 10;
  tarch::la::Vector<Dimensions,double> pos = { (x[0] - u_inf*t_capped) < -5.0 ? (x[0] - u_inf*t_capped) + 10. : (x[0] - u_inf*t_capped), 
                                               (x[1] - v_inf*t_capped) < -5.0 ? (x[1] - v_inf*t_capped) + 10. : (x[1] - v_inf*t_capped) };

  static constexpr double epsilon = 5.0;
  static constexpr double pi = std::numbers::pi;
  static constexpr double i_pi = 1./pi;
  static constexpr double eos_gamma = 1.4;

  const double r = tarch::la::norm2(pos);
  const double du = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (- pos[1]);
  const double dv = 0.5 * epsilon / pi * exp(0.5 * (1. - r * r)) * (pos[0]);
  const double dTemp = -(eos_gamma - 1.) * epsilon*epsilon / 8. / eos_gamma / pi / pi *
                 exp(1. - r * r);
  const double drho = pow(1. + dTemp, 1. / (eos_gamma - 1.)) - 1.;
  const double dp = pow(1. + dTemp, eos_gamma / (eos_gamma - 1.)) - 1.;

	sol[0] = rho_inf + drho; // fluid density
	sol[1] = sol[0] * (u_inf + du); // momentum
	sol[2] = sol[0] * (v_inf + dv);
	// total energy = internal energy + kinetic energy
	sol[3] = (p_inf + dp) / (eos_gamma-1) + 0.5*sol[0] * ((u_inf+du)*(u_inf+du)+(v_inf+dv)*(v_inf+dv));
"""