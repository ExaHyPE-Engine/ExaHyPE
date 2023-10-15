import peano4
import exahype2
import os

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

dimensions = 2

project = exahype2.Project(
    ["exahype2", "acoustic"], ".", executable="Acoustic" + str(dimensions) + "D"
)

# stability condition: CFL for Ader-DG
# dt <= c*dx / (lambda_max*(2N+1)
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]
# here lambda_max = 100
# dt <= 0.1 * 0.03 / 700 \approx  0.000004
# dt <= 0.1 * 0.03 / 1100 \approx  0.0000025

unknowns = 4
auxiliary_variables = 0

max_h = 0.1
min_h = 0.9 * max_h / 3.0

timestep = 0.00001

theSolver = GlobalFixedTimeStep(
    name="acousticSolver",
    order=3,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_size=timestep,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
)


theSolver.set_implementation(
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation
)
theSolver.add_kernel_optimizations(
    is_linear=True, use_kernel_generator=True, architecture="noarch", use_Eigen=True
)

project.add_solver(theSolver)

build_mode = peano4.output.CompileMode.Release

offset = [0.0, 0.0, 0.0]
size = [1.0, 1.0, 1.0]

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=1500 * timestep,
    max_end_time=1500 * timestep,
    first_plot_time_stamp=0.0,
    time_in_between_plots=10 * timestep,
    periodic_BC=[False, False, False],
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing(
    "toolbox::loadbalancing::RecursiveBipartition",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../../../", build_mode)
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
