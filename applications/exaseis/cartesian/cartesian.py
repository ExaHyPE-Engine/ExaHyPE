import argparse

import exahype2

from exahype2.solvers.aderdg.ADERDG import Polynomials

import Elastic

min_level           = 3
max_depth           = 0
order               = 5
precision           = "float"
e_t                 = 2.0
polynomials         = "legendre"
use_tracers         = True

size    = [ 9.0, 9.0, 9.0]  # thousand meters
offset  = [-4.5, 0., -4.5]  # thousand meters

unknowns            = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}


def parse_arguments():
  parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
  parser.add_argument("-h", "--help", action="store_true", help="show this help message and continue with project help")
  parser.add_argument("-et", "--end-time", type=float, default=e_t)
  parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=order)
  parser.add_argument("-l", "--grid-levels", type=int, default=None)
  parser.add_argument("-lmin", "--min-grid-levels", type=int, default=min_level)
  parser.add_argument("-lmax", "--max-grid-levels", type=int, default=min_level + max_depth)
  args, _ = parser.parse_known_args()
  if args.grid_levels is not None:
      args.min_grid_levels = args.grid_levels
      args.max_grid_levels = args.grid_levels
  if args.help:
    parser.print_help()
    print()
  return args


args = parse_arguments()
order = args.degrees_of_freedom

project = exahype2.Project(
  ["applications", "exahype2", "exaseis"],
  "Cartesian",
  dimensions=3,
  offset=offset,
  size=size,
  periodic_bc=[False, False, False],
  executable="ExaSeis",
  min_end_time=args.end_time,
  max_end_time=args.end_time,
  min_grid_levels=args.min_grid_levels,
  max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 3:
  raise ValueError("The Cartesian ExaSeis application is three-dimensional.")

theSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="elastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  time_step_relaxation=0.9,
)

theSolver.add_kernel_optimisations(
  is_linear=True, polynomials=(Polynomials.Gauss_Lobatto if polynomials=="lobatto" else Polynomials.Gauss_Legendre)
  ,precision=precision
)

theSolver.set_implementation(
  initial_conditions          = Elastic.initial(),
  boundary_conditions         = Elastic.boundary(),
  max_eigenvalue              = Elastic.eigenvalue(),
  flux                        = Elastic.flux(),
  refinement_criterion        = Elastic.refinement_criterion(),
  riemann_solver              = Elastic.riemann_solver(),
  number_of_point_sources     = 1,
  point_source                = Elastic.point_source(),
  init_point_source_location  = Elastic.init_point_source()
)

theSolver.add_user_solver_includes("""
#include "../riemannSolvers/cartesianRoutines.h"
#include "../riemannSolvers/riemannsolverIsotropic.h"
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
        exahype2.tracer.DiscontinuousGalerkinTracing(
            tracer_particles, theSolver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=tracer_particles, solver=theSolver,
            filename="tracers/Cartesian_level_"+str(args.min_grid_levels)+"_order_"+str(order)+"_"+polynomials+"_precision_"+str(precision),
            time_delta_between_two_snapshots=0.01,
            output_precision=10
        )
    )

    theSolver._abstract_solver_user_declarations += "double QuadraturePoints1D[Order+1];"
    # TODO: This is not good in many ways. A private variable member of the solver should not be hijacked like this!
    theSolver._start_simulation_implementation += """
    std::copy_n(
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
      (Order+1),
      QuadraturePoints1D
    );"""

project.generate_and_build()
