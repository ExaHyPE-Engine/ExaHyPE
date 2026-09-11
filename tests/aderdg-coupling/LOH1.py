import os, sys
import argparse
import exahype2

sys.path.insert(0, os.path.abspath("../../applications/exaseis/cartesian"))
import Elastic

def parse_arguments():
  parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
  parser.add_argument(
    "-h",
    "--help",
    action="store_true",
    help="show this help message and continue with project help",
  )
  parser.add_argument("-et", "--end-time", type=float, default=0.3)
  parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
  parser.add_argument("-l", "--grid-levels", type=int, default=None)
  parser.add_argument("-ut", "--use-tracers", type=bool, default=True)
  args, _ = parser.parse_known_args()
  if args.help:
    parser.print_help()
    print()
  return args

args = parse_arguments()
order = args.degrees_of_freedom

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"],
    "LOH1",
    executable="coupled_LOH1",
    dimensions=3,
    offset=[-2.025, 0.0, -2.025],
    size=[12.15, 12.15, 12.15],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 3:
  raise ValueError(
    f"LOH1 coupling is a 3D testcase, but the project was configured for "
    f"{project.dimensions}D."
  )


hpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="hpElastic",  order=order,
  unknowns={"v": 3, "sigma": 6},
  auxiliary_variables={"rho":1, "cp": 1, "cs": 1},
  time_step_relaxation=0.9
)

hpSolver.add_kernel_optimisations(
  precision = "fp64", is_linear=True
)

lpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="lpElastic",  order=order,
  unknowns={"v": 3, "sigma": 6},
  auxiliary_variables={"rho":1, "cp": 1, "cs": 1},
  time_step_relaxation=0.9
)

lpSolver.add_kernel_optimisations(
  precision = "fp32", is_linear=True
)

if args.use_tracers:
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

  theSolver.add_user_solver_includes("""
#include "../../applications/exaseis/riemannSolvers/cartesianRoutines.h"
#include "../../applications/exaseis/riemannSolvers/riemannsolverIsotropic.h"
""")

  project.add_solver(theSolver)

  if args.use_tracers:
    project.add_action_set_to_timestepping(
        exahype2.tracer.DiscontinuousGalerkinTracing(
            tracer_particles, theSolver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
            projection_kernel_arguments="""
        marker,
        {{ORDER}},
        kernels::{{SOLVER_NAME}}::Quadrature<"""+theSolver._solution_persistent_storage_precision+""">::nodes,
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
        *particle
            """
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=tracer_particles, solver=theSolver,
            filename="LOH1",
            time_delta_between_two_snapshots=0.01,
            output_precision=10
        )
    )

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
project.generate_and_build()
