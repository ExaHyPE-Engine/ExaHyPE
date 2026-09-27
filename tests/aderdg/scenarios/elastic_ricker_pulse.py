# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Elastic


class ElasticRickerPulse(Scenario):
    """
    Scenario analogous to Käser, Herrmann & de la Puente https://doi.org/10.1111/j.1365-246X.2008.03781.x (Chapter 2.2, p. 992)

    An analytical solution is available, but the quadrature measures the error inaccurately whenever the
    signal sits away from the centre of a cell.
    Time-frequency misfit analysis measures the error accurately (like Käser et al. above).
    It takes a tracer that records the results at a point in the domain and a misfit computation over
    that record, for example with ObsPy.

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

  // The pulse is centred in the leftmost cell. It reaches the left edge of the
  // domain, so the periodic boundary places part of it on the right edge.
  const auto tau  = 1.25*std::numbers::pi*(x[0]-h[0]/2);
  const auto tau2 = 1.25*std::numbers::pi*(x[0]-DomainSize[0]-h[0]/2);
  const auto amplitudeX = (1-2*tau*tau)*std::exp(-tau*tau) + (1-2*tau2*tau2)*std::exp(-tau2*tau2);
  constexpr auto amplitudeY = 0.;
  // Stresses
  Q[Shortcuts::sigma+0] = -amplitudeX*(lambda+2*mu)  + amplitudeY*lambda;         //xx
  Q[Shortcuts::sigma+1] = -amplitudeX*lambda         + amplitudeY*(lambda+2*mu);  //yy
  Q[Shortcuts::sigma+2] = -amplitudeX*mu + amplitudeY*mu;                         //xy

  // Velocities
  Q[Shortcuts::v+0] = amplitudeX*cp + amplitudeY*cs; //vx
  Q[Shortcuts::v+1] = amplitudeY*cp + amplitudeX*cs; //vy

  // Auxiliary variables
  Q[Shortcuts::rho] = rho;  //rho
  Q[Shortcuts::cp]  = cp;   //cp
  Q[Shortcuts::cs]  = cs;   //cs
"""

    def analytical_solution(self):
        return """
  // Constant auxiliary parameters
  constexpr auto cp   = 1;
  constexpr auto cs   = 1;

  // Width of a cell on the finest level. The pulse starts centred in the leftmost cell.
  const double h = ::exahype2::runner::RuntimeParameters::divideBySubdivisionFactorPower(
    DomainSize[0], ::exahype2::runner::RuntimeParameters::getInstance().getMaxGridLevels()
  );

  auto distance = std::fmod( (x[0]-t-h/2), DomainSize[0]);
  const auto tau  = 1.25*std::numbers::pi*(distance);
  const auto amplitudeX = (1-2*tau*tau)*std::exp(-tau*tau);
  constexpr auto amplitudeY = 0.;

  // Stresses
  solution[Shortcuts::sigma+0] = Q[Shortcuts::sigma+0];
  solution[Shortcuts::sigma+1] = Q[Shortcuts::sigma+1];
  solution[Shortcuts::sigma+2] = Q[Shortcuts::sigma+2];

  // Velocities
  solution[Shortcuts::v+0] = amplitudeX*cp + amplitudeY*cs; //vx
  solution[Shortcuts::v+1] = Q[Shortcuts::v+1];
"""
