# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os, sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import NavierStokes

import math


class NavierStokesABCFlow(Scenario):
    """
    Arnold-Beltrami-Childress flow, described e.g. in doi.org/10.1016/j.jcp.2016.05.009.

    Somewhat analogous to Taylor-Green vortex, but in 3D.
    """

    _dimensions = 3
    _offset = -math.pi
    _domain_size = 2 * math.pi
    _periodic_bc = True
    _plot_dt = 0.0
    _end_time = 0.1

    _equation = NavierStokes(
        dimensions=3,
        use_advection=False,
        use_background_state=False,
        use_gravity=False,
        use_viscosity=True,
        gamma=1.4,
        cv=1.0,
        gas_constant=0.4,
        reference_viscosity=0.01,
        Pr=0.7,
    )

    def __init__(self):
        return

    def analytical_solution(self):
        return (
            """
    constexpr auto gamma = 1.4;
    constexpr auto q0 = 0.0;
    constexpr auto referenceViscosity = 0.01;

    const auto c = 100 / gamma;
    const auto Ft = std::exp(-1 * referenceViscosity * t);
    const auto pressure = c -
        (std::sin(x[1]) * std::cos(x[0]) + std::sin(x[0]) * std::cos(x[2]) +
        std::sin(x[2]) * std::cos(x[1])) *
            Ft * Ft;

    const double rho = 1.0;
    solution[Shortcuts::rho] = rho;
    solution[Shortcuts::j+0] = (std::sin(x[2]) + std::cos(x[1])) * Ft;
    solution[Shortcuts::j+1] = (std::sin(x[0]) + std::cos(x[2])) * Ft;
    solution[Shortcuts::j+2] = (std::sin(x[1]) + std::cos(x[0])) * Ft;

    auto j = &solution[Shortcuts::j];
    auto Z = 0.0;
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
