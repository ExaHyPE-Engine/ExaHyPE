import peano4
import exahype2
import os

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.GlobalAdaptiveTimeStep import (
    GlobalAdaptiveTimeStep,
)
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project(["exahype2", "elastic"], "pml", executable="LOH1")


unknowns = {"v": 3, "sigma": 6, "pml": 27}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "dmp_pml": 3, "jacobian": 1,
    "metric_derivative": 9,
    "curve_grid": 3,
}

offset = [-2.28662, 0.0, -2.28662]
size = [16.3333, 16.3333, 16.3333]
end_time = 1.0  # 10.0
order = 5
min_level = 2
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)


theSolver = GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
    material_parameters=exahype2.solvers.PDETerms.User_Defined_Implementation,
    point_source=1,
)

theSolver.add_kernel_optimizations(
    is_linear=True,
    architecture="noarch",
    polynomials=Polynomials.Gauss_Lobatto,
    initialize_patches=True,
    riemann_solver_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

project.add_solver(theSolver)


tracer_particles = project.add_tracer(name="Tracer", attribute_count=55)

project.add_action_set_to_initialisation(
    exahype2.tracer.InsertParticlesByCoordinates(
        particle_set=tracer_particles,
        coordinates=[
          [0.000, 0., 0.693], [0.000, 0., 5.543], [0.000, 0., 10.392],
          [0.490, 0., 0.490], [3.919, 0., 3.919], [7.348, 0., 7.3480],
          [0.577, 0., 0.384], [4.612, 0., 3.075], [8.647, 0., 5.7640]
        ],
    )
)

project.add_action_set_to_timestepping(
    peano4.toolbox.particles.api.UpdateParallelState(particle_set=tracer_particles)
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DiscontinuousGalerkinTracing(
        tracer_particles, theSolver,
        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
    )
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles, solver=theSolver,
        filename="tracer-PML-d-"+str(min_level)+"-o-"+str(order),
        data_delta_between_two_snapsots=1e16, time_delta_between_two_snapsots=0.000001,
        output_precision=10, clear_database_after_flush=False
    )
)

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.000,
    periodic_BC=[False, False, False],
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project("False")

peano4_project.output.makefile.set_CXX_compiler("mpicxx")
peano4_project.output.makefile.add_CXX_flag("-fopenmp -DUSE_ASAGI")
peano4_project.output.makefile.add_library(
    "-fopenmp -lcurvi -leasi -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi -DUSE_ASAGI"
)

peano4_project.build(make_clean_first=True, number_of_parallel_builds=32)
