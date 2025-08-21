
import peano4, exahype2
import os

from exahype2.solvers.aderdg.ADERDG import Polynomials

import Elastic

min_level           = 3
max_depth           = 0
order               = 5
precision           = "float"
e_t                 = 2.0
plot_dt             = 0.1
polynomials         = "legendre"
use_tracers         = True

size    = [ 9.0, 9.0, 9.0]  # thousand meters
offset  = [-4.5, 0., -4.5]  # thousand meters

unknowns            = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}

max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

project = exahype2.Project( ["applications", "exahype2", "exaseis"], ".",
  executable="LOH_level_"+str(min_level)+"_order_"+str(order)+"_"+polynomials+"_precision_"+str(precision) )

theSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="elastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9,
)

theSolver.add_kernel_optimisations(
  is_linear=True, polynomials=(Polynomials.Gauss_Lobatto if polynomials=="lobatto" else Polynomials.Gauss_Legendre)
  ,solution_persistent_storage_precision=precision
  ,predictor_computation_precisions=[precision]
  ,corrector_computation_precision=precision
)

theSolver.set_implementation(
  initial_conditions    = Elastic.initial(),
  boundary_conditions   = Elastic.boundary(),
  max_eigenvalue   = Elastic.eigenvalue(),
  flux          = Elastic.flux(),
  refinement_criterion = Elastic.refinement_criterion(),
  riemann_solver=Elastic.riemann_solver(),
  number_of_point_sources = 1,
  point_source=Elastic.point_source(),
  init_point_source_location=Elastic.init_point_source()
)

theSolver.add_user_solver_includes("""
#include "../ExaSeis_core/Numerics/cartesianRoutines.h"
#include "../ExaSeis_core/Numerics/riemannsolverIsotropic.h"
""")

project.add_solver(theSolver)

if use_tracers:
    tracer_particles = project.add_tracer(name="Tracer", particle_attributes=12)

    project.add_action_set_to_initialisation(
        exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=tracer_particles,
            coordinates=[
              [0.000, 0., 0.693], [0.000, 0., 5.543], [0.000, 0., 10.392],
              [0.490, 0., 0.490], [3.919, 0., 3.919], [7.348, 0., 7.3480],
              [0.577, 0., 0.384], [4.612, 0., 3.075], [8.647, 0., 5.7640]
            ]
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
            filename="tracers/Cartesian_level_"+str(min_level)+"_order_"+str(order)+"_"+polynomials+"_precision_"+str(precision),
            data_delta_between_two_snapsots=1e16, time_delta_between_two_snapsots=0.01,
            output_precision=10, clear_database_after_flush=False
        )
    )

    # project.add_action_set_to_timestepping(
    #   exahype2.tracer.NewDGTracer(
    #     coordinates=[
    #       [0.000, 0., 0.693], [0.000, 0., 5.543], [0.000, 0., 10.392],
    #       [0.490, 0., 0.490], [3.919, 0., 3.919], [7.348, 0., 7.3480],
    #       [0.577, 0., 0.384], [4.612, 0., 3.075], [8.647, 0., 5.7640]
    #     ],
    #     solver=theSolver,
    #     filename="tracers/Cartesian_level_"+str(min_level)+"_order_"+str(order)+"_"+polynomials+"_precision_"+str(precision),
    #     data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
    #     output_precision=10, clear_database_after_flush=False
    # ))

    if not os.path.exists("tracers"):
        os.makedirs("tracers")

    theSolver._abstract_solver_user_declarations += "double QuadraturePoints1d[Order+1];"
    theSolver._constructor_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1d
    );"""

if plot_dt > 0.0:
  project.set_output_path("solutions")

project.set_global_simulation_parameters(
  dimensions            = 3,
  offset                = offset[0:3],
  size                  = size[0:3],
  min_end_time          = e_t,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_dt,
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation("../../../../", peano4.output.CompileMode.Release)
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True)
