import argparse
import os
import sys

import exahype2

from exahype2.solvers.aderdg.ADERDG import Polynomials

sys.path.insert(0, os.path.abspath("../../applications/exaseis/cartesian"))
import Elastic
degrees_of_freedom = 6

order = degrees_of_freedom - 1

polynomials = "legendre"
use_tracers = True

offset = [-2.025, 0.0, -2.025]
size = [12.15, 12.15, 12.15]

unknowns            = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}


def parse_arguments():
  parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
  parser.add_argument(
    "-h",
    "--help",
    action="store_true",
    help="show this help message and continue with project help",
  )
  parser.add_argument("-et", "--end-time", type=float, default=0.3)
  parser.add_argument("-l", "--grid-levels", type=int, default=None)
  parser.add_argument("-lmin", "--min-grid-levels", type=int, default=2)
  parser.add_argument("-lmax", "--max-grid-levels", type=int, default=2)
  args, _ = parser.parse_known_args()
  if args.grid_levels is not None:
      args.min_grid_levels = args.grid_levels
      args.max_grid_levels = args.grid_levels
  if args.help:
    parser.print_help()
    print()
  return args


args = parse_arguments()

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"],
    "LOH1",
    executable="coupled_LOH1",
    dimensions=3,
    size=size[0:3],
    offset=offset[0:3],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

if project.dimensions != 3:
  raise ValueError(
    f"LOH1 coupling is a 3D testcase, but the project was configured for "
    f"{project.dimensions}D."
  )


hpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="hpElastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  time_step_relaxation=0.9
)

hpSolver.add_kernel_optimisations(
  precision = "fp64"
)

lpSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="lpElastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  time_step_relaxation=0.9
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
#include "../../applications/exaseis/riemannSolvers/cartesianRoutines.h"
#include "../../applications/exaseis/riemannSolvers/riemannsolverIsotropic.h"
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
            filename="tracers/Cartesian_o"+str(order)+"_"+polynomials+theSolver.get_name_of_global_instance(),
            time_delta_between_two_snapshots=0.01,
            output_precision=10
        )
    )

    theSolver._abstract_solver_user_declarations += "double QuadraturePoints1D[Order+1];"
    theSolver._start_simulation_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1D
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
project.generate_and_build()
