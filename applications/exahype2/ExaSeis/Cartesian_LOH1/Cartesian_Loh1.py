import peano4
import exahype2

import argparse

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep import (
    GlobalAdaptiveTimeStep,
)
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project(
    ["examples", "exahype2", "elastic"], "loh", ".", executable="LOH1"
)

# stability condition: CFL for Ader-DG
# dt <= c*dx / d*(lambda_max*(2N+1)
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]
# here lambda_max = 6.0,                 77.76
# Divisions of domain: [16.333, 5.444, 1.815, 0.6049, 0.202, 0.067]
# for order 3 dt <= 0.1*0.6049/(3*6.0*(2*3+1))     = 0.00048008 > 1*e**-04
# for order 5 dt <= 0.045*0.6049/(3*6.0*(2*5+1))   = 0.00013745 > 1*e**-04

unknowns = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho": 1, "cp": 1, "cs": 1}

dimensions = 3
order = 3
offset = [-9, 0, -9]
size = [18, 18, 18]
end_time = 1.0  # 5.0
order = 3
min_level = 3
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

theSolver = GlobalAdaptiveTimeStep(
    name="loh",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    material_parameters=exahype2.solvers.PDETerms.User_Defined_Implementation,
    point_source=1,
)

theSolver.add_kernel_optimizations(
    is_linear=True,
    use_kernel_generator=True,
    riemann_solver_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
)
theSolver.set_implementation(
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation
)

project.add_solver(theSolver)


tracer_particles = project.add_tracer(name="Tracer", attribute_count=12, plot=False)
project.add_action_set_to_initialisation(
    exahype2.tracer.InsertParticlesByCoordinates(
        particle_set=tracer_particles,
        coordinates=[[0.000, 0.0, 0.693], [7.348, 0.0, 7.348]],
    )
)
project.add_action_set_to_timestepping(
    peano4.toolbox.particles.UpdateParallelState(particle_set=tracer_particles)
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DiscontinuousGalerkinTracing(
        tracer_particles,
        theSolver,
        # time_stepping_kernel="toolbox::particles::staticPosition",
        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
    )
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles,
        solver=theSolver,
        filename="Tracer-LOH1-cartesian",
        data_delta_between_two_snapsots=1e16,
        time_delta_between_two_snapsots=0.001,
    )
)


build_mode = peano4.output.CompileMode.Release


project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
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

peano4_project.output.makefile.add_cpp_file("riemannSolverLOH.cpp")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=8)
