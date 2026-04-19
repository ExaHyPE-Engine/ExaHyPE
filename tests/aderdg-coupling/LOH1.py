
import peano4, exahype2
import os, sys

from exahype2.solvers.aderdg.ADERDG import Polynomials

sys.path.insert(0, os.path.abspath("../../applications/exaseis/cartesian"))
import Elastic

parser = exahype2.ArgumentParser("ExaHyPE 2 - ADER-DG Coupled Solvers Testing Script")
parser.set_defaults(
    min_depth=2,
    degrees_of_freedom=6,
    number_of_snapshots=0,
)
args = parser.parse_args()

order = args.degrees_of_freedom - 1

if args.build_mode == "Debug":
    end_time = 0.1
else:
    end_time = 2.0

polynomials = "legendre"
use_tracers = True

offset = [-2.025, 0.0, -2.025]
size = [12.15, 12.15, 12.15]

unknowns            = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}

max_h               = 1.1 * min(size) / (3.0**args.min_depth)
min_h               = max_h * 3.0 ** (-args.amr_levels)

project = exahype2.Project(["tests", "exahype2", "aderdg"], ".",
  executable="coupled_LOH1" )

hpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="hpElastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9
)

hpSolver.add_kernel_optimisations(
  precision = "fp64"
)

lpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="lpElastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9
)

lpSolver.add_kernel_optimisations(
  precision = "fp32"
)

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

  if not os.path.exists("tracers"):
      os.makedirs("tracers")


from fuseADERSolvers import fuseADERSolvers
fuseADERSolvers(hpSolver, lpSolver, """
  ( marker.getX()[0] < DomainOffset[0] + 9.1*marker.getH()[0] and
    marker.getX()[1] <                   8.1*marker.getH()[1] and
    marker.getX()[2] < DomainOffset[2] + 11.1*marker.getH()[2])
""", """
  ( marker.getX()[0] > DomainOffset[0] + 8.9*marker.getH()[0] or
    marker.getX()[1] >                   7.9*marker.getH()[1] or
    marker.getX()[2] > DomainOffset[2] + 10.9*marker.getH()[2])
"""
)


for theSolver in [hpSolver, lpSolver]:

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

  theSolver.add_kernel_optimisations(
    is_linear=True, polynomials=(Polynomials.Gauss_Lobatto if polynomials=="lobatto" else Polynomials.Gauss_Legendre)
  )

  theSolver.add_user_solver_includes("""
  #include "../../applications/exaseis/ExaSeis_core/Numerics/cartesianRoutines.h"
  #include "../../applications/exaseis/ExaSeis_core/Numerics/riemannsolverIsotropic.h"
  """)

  project.add_solver(theSolver)

  if use_tracers:
    project.add_action_set_to_timestepping(
        exahype2.tracer.DiscontinuousGalerkinTracing(
            tracer_particles, theSolver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=tracer_particles, solver=theSolver,
            filename="tracers/Cartesian_l"+str(args.min_depth)+"_o"+str(order)+"_"+polynomials+theSolver.get_name_of_global_instance(),
            data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
            output_precision=10, clear_database_after_flush=False
        )
    )

    theSolver._abstract_solver_user_declarations += "double QuadraturePoints1d[Order+1];"
    theSolver._start_simulation_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1d
    );"""

# sys.path.insert(0, os.path.abspath("./coupling"))
# from coupling import StaticCoupling
# limiter = StaticCoupling(
#   name            = "limitingSolver",
#   regularSolver   = hpSolver,
#   limitingSolver  = lpSolver,
#   limiting_criterion_implementation = """
#   // return 
#   return // 9x8x11
#     x[0] > -2.025 && x[0] <  2.025 &&
#                      x[1] <  3.600 &&  
#     x[2] > -2.025 && x[2] <= 2.925
#   ;
# """
# )
# project.add_solver(limiter)


if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
  dimensions            = 3,
  offset                = offset[0:3],
  size                  = size[0:3],
  min_end_time          = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = time_in_between_plots,
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
peano4_project = project.generate_Peano4_project("False")
peano4_project.build(make_clean_first=True)
