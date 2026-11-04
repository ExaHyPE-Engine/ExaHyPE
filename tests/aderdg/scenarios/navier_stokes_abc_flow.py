# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import NavierStokes

import math


class NavierStokesABCFlow(Scenario):
    """
    The Arnold-Beltrami-Childress flow, the 3D counterpart of the Taylor-Green vortex, described in
    doi.org/10.1016/j.jcp.2016.05.009.
    """

    _dimensions = 3
    _offset = -math.pi
    _domain_size = 2 * math.pi
    _periodic_bc = True
    _end_time = 0.1

    _equation = NavierStokes(
        dimensions=3,
        use_advection=False,
        use_background_state=False,
        use_gravity=False,
        gamma=1.4,
        cv=1.0,
        gas_constant=0.4,
        reference_viscosity=0.01,
        prandtl_number=0.7,
    )

    def __init__(self):
        return

    def analytical_solution(self):
        return (
            """
    constexpr auto AdiabaticIndex = 1.4;
    constexpr auto HeatRelease = 0;
    constexpr auto ReferenceViscosity = 0.01;

    const auto meanPressure = 100 / AdiabaticIndex;
    const auto decayFactor = std::exp(-1 * ReferenceViscosity * t);
    const auto pressure = meanPressure -
        (std::sin(x[1]) * std::cos(x[0]) + std::sin(x[0]) * std::cos(x[2]) +
        std::sin(x[2]) * std::cos(x[1])) *
            decayFactor * decayFactor;

    const double rho = 1;
    solution[Shortcuts::rho] = rho;
    solution[Shortcuts::j+0] = (std::sin(x[2]) + std::cos(x[1])) * decayFactor;
    solution[Shortcuts::j+1] = (std::sin(x[0]) + std::cos(x[2])) * decayFactor;
    solution[Shortcuts::j+2] = (std::sin(x[1]) + std::cos(x[0])) * decayFactor;

    const auto* const j = &solution[Shortcuts::j];
    const auto Z = 0;
    """
            + self._equation.evaluate_energy()
            + """
    solution[Shortcuts::E] = E;
"""
        )

    def initial_conditions(self):
        return """
    const double t = 0.;
    auto* const solution = Q;
""" + self.analytical_solution()
