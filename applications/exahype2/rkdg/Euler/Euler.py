#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os, sys
import peano4
import exahype2
from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import (
    FaceProjections,
)

project = exahype2.Project(
    ["examples", "exahype2", "euler"], "euler", ".", executable="Euler"
)

#
# Add the Finite Volumes solver
#
unknowns = 5
auxiliary_variables = 0
time_step_size = 0.1
max_h = 0.03
min_h = max_h * 0.9 / 3.0

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#

rk_order = 2
dg_order = 4

thesolver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep(
    "euler",
    rk_order,
    exahype2.solvers.GaussLegendreBasis(dg_order),
    FaceProjections.Solution,
    unknowns,
    auxiliary_variables,
    min_h,
    max_h,
    flux=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    time_step_relaxation=0.9,
)

thesolver.set_implementation(
    refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

project.add_solver(thesolver)

dimensions = 2
build_mode = peano4.output.CompileMode.Release

#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
    dimensions=2,
    offset=[0.0, 0.0],
    size=[1.0, 1.0],
    min_end_time=0.04,
    max_end_time=0.04,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.001,  # snapshots
    periodic_BC=[False, False, False],
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing(
    "toolbox::loadbalancing::RecursiveSubdivision",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../../", build_mode)
peano4_project = project.generate_Peano4_project(False)

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: Euler")
print(
    "Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-euler.peano-patch-file"
)
