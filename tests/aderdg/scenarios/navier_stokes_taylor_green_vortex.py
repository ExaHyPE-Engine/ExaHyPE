# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os, sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import NavierStokes

import math


class NavierStokesTaylorGreenVortex(Scenario):
    """
    Originally described in doi.org/10.1098/rspa.1937.0036, the Taylor-Green Vortex.

    is a scenario for the incompressible Navier-Stokes equations essentially describing
    rotating segments with constant density.

    As the scenario has a known analytical solution it is commonly used as a benchmark.
    """

    _dimensions = 2
    _offset = 0.0
    _domain_size = 2 * math.pi
    _periodic_bc = True
    _end_time = 10.0

    _equation = NavierStokes(
        dimensions=2,
        use_advection=False,
        use_background_state=False,
        use_gravity=False,
        use_viscosity=True,
        gamma=1.4,
        cv=1.0,
        gas_constant=0.4,
        reference_viscosity=0.1,
        Pr=0.7,
    )

    def __init__(self):
        return

    def analytical_solution(self):
        return (
            """
    constexpr auto gamma = 1.4;
    constexpr auto q0 = 0;
    constexpr auto referenceViscosity = 0.1;
    const double Ft = std::exp(-2 * referenceViscosity * t);

    const double rho = 1;
    solution[Shortcuts::rho] = rho;
    solution[Shortcuts::j+0] = std::sin(x[0]) * std::cos(x[1]) * Ft;
    solution[Shortcuts::j+1] = -1 * std::cos(x[0]) * std::sin(x[1]) * Ft;

    const double p_0 = 100. / gamma;
    const double pressure = p_0 + ((rho * Ft * Ft) / 4) *
                                      (std::cos(2 * x[0]) + std::cos(2 * x[1]));
    auto j = &solution[Shortcuts::j];
    auto Z = 0;
    """
            + self._equation.evaluate_energy()
            + """
    solution[Shortcuts::E] = E;
"""
        )

    def initial_conditions(self):
        return (
            """
    double t = 0.;
    auto solution = Q;
"""
            + self.analytical_solution()
        )
