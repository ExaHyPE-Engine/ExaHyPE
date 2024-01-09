import peano4
import exahype2

import argparse

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep import (
    GlobalAdaptiveTimeStep,
)
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials


dimensions = 3

project = exahype2.Project(
    ["examples", "exahype2", "elastic"],
    ".",
    executable="ELASTIC" + str(dimensions) + "D",
)


unknowns = {"sigma": 6, "v": 3}
auxiliary_variables = {"rho": 1, "cp": 1, "cs": 1}

order               = 3
min_level           = 2
max_depth           = 0
max_h               = 1.1 / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

theSolver = GlobalAdaptiveTimeStep(
    name                  = "elastic",
    order                 = order,
    unknowns              = unknowns,
    auxiliary_variables   = auxiliary_variables,
    min_cell_h            = min_h,
    max_cell_h            = max_h,
    time_step_relaxation  = 0.5,
    flux  = exahype2.solvers.PDETerms.User_Defined_Implementation,
    # ncp   = exahype2.solvers.PDETerms.User_Defined_Implementation
)

theSolver.add_kernel_optimizations(is_linear=True)


project.add_solver(theSolver)

build_mode = peano4.output.CompileMode.Release

offset  = [0.0, 0.0, 0.0]
size    = [1.0, 1.0, 1.0]

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=0.15,
    max_end_time=0.15,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.01,
    periodic_BC=[False, False, False],
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing(
    "toolbox::loadbalancing::strategies::RecursiveBipartition",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../../", build_mode)
peano4_project = project.generate_Peano4_project("False")

peano4_project.output.makefile.add_cpp_file("riemannSolver.cpp")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=32)
