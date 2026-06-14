# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Elastic


class ElasticRickerPulse(Scenario):
    """
    Scenario analogous to Käser, Herrmann & de la Puente https://doi.org/10.1111/j.1365-246X.2008.03781.x (Chapter 2,2, p. 992)

    While an analytical solution is available, the quadrature struggles to properly measure the error whenever the signal is not
    at the center of a cell.
    A better way to measure the error is to use time-frequency misfit analysis (like Käser et. al. above).
    To do this one would use tracing to measure the results at a point in the domain, then compute the misfits using e.g. osby.

    A tutorial on computing misfits can be found at https://docs.obspy.org/tutorial/code_snippets/time_frequency_misfit.html
    """

    _offset = 0.0
    _domain_size = 30.0
    _periodic_bc = True
    _dimensions = 2

    def __init__(self, iterations=1):
        self._end_time = self._domain_size * iterations
        self._equation = Elastic(dimensions=2)

    def initial_conditions(self):
        return """
  // Lamé parameters
  constexpr auto mu     =  1;
  constexpr auto lambda = -1;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 1;
  constexpr auto cs   = 1;

  // The pulse is centered in the middle of the leftmost cell.
  // Since it is near the left edge of the domain, part of it has to
  // be on the right side of the domain because of periodic boundaries.
  const auto tau  = 1.25*std::numbers::pi*(x[0]-h[0]/2);
  const auto tau2 = 1.25*std::numbers::pi*(x[0]-DomainSize[0]-h[0]/2);
  const auto val_x = (1-2*tau*tau)*std::exp(-tau*tau) + (1-2*tau2*tau2)*std::exp(-tau2*tau2);
  constexpr auto val_y = 0.;
  // Stresses
  Q[Shortcuts::sigma+0] = -val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  Q[Shortcuts::sigma+1] = -val_x*lambda         + val_y*(lambda+2*mu);  //yy
  Q[Shortcuts::sigma+2] = -val_x*mu + val_y*mu;                         //xy

  // Velocities
  Q[Shortcuts::v+0] = val_x*cp + val_y*cs; //vx
  Q[Shortcuts::v+1] = val_y*cp + val_x*cs; //vy

  // Auxiliary variables
  Q[Shortcuts::rho] = rho;  //rho
  Q[Shortcuts::cp]  = cp;   //cp
  Q[Shortcuts::cs]  = cs;   //cs
"""

    def analytical_solution(self):
        return """
  // Lamé parameters
  constexpr auto mu     =  1;
  constexpr auto lambda = -1;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 1;
  constexpr auto cs   = 1;

  // Computing the cell size as it is not available in the analytical sol
  const double minAdmissiblePatchSize = ::exahype2::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);
  const double depth = (std::log(30.0 / minAdmissiblePatchSize) + std::log(3.0) - 1.0) / std::log(3.0);
  const double h = 30./std::pow(3,depth);

  auto distance = std::fmod( (x[0]-t-h/2), DomainSize[0]);
  const auto tau  = 1.25*std::numbers::pi*(distance);
  const auto val_x = (1-2*tau*tau)*std::exp(-tau*tau);
  constexpr auto val_y = 0.;

  // Stresses
  solution[Shortcuts::sigma+0] = Q[Shortcuts::sigma+0];
  solution[Shortcuts::sigma+1] = Q[Shortcuts::sigma+1];
  solution[Shortcuts::sigma+2] = Q[Shortcuts::sigma+2];

  // Velocities
  solution[Shortcuts::v+0] = val_x*cp + val_y*cs; //vx
  solution[Shortcuts::v+1] = Q[Shortcuts::v+1];
"""
