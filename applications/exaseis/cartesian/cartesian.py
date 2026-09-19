import argparse

import exahype2

import Elastic

def parse_arguments():
  parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
  parser.add_argument("-h", "--help", action="store_true", help="show this help message and continue with project help")
  parser.add_argument("-et", "--end-time", type=float, default=2.0)
  parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
  parser.add_argument("-l", "--grid-levels", type=int, default=None)
  parser.add_argument("-p", "--precision", type=str, default="float")
  parser.add_argument("-lob", "--lobatto", action="store_true", help="use Gauss-Lobatto quadrature nodes, otherwise defaults to Gauss-Legendre")
  parser.add_argument("-ut", "--use-tracers", action=argparse.BooleanOptionalAction, default=True)
  args, _ = parser.parse_known_args()
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
  offset=[-4.5, 0., -4.5],  # thousand meters
  size=[ 9.0, 9.0, 9.0],  # thousand meters
  periodic_bc=[False, False, False],
  executable="ExaSeis",
  min_end_time=args.end_time,
  max_end_time=args.end_time,
  min_grid_levels=args.grid_levels,
  max_grid_levels=args.grid_levels,
)
if project.dimensions != 3:
  raise ValueError("The Cartesian ExaSeis application is three-dimensional.")

theSolver=exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name="elastic",  order=order,
  unknowns={"v": 3, "sigma": 6},
  auxiliary_variables={"rho":1, "cp": 1, "cs": 1},
  time_step_relaxation=0.9,
)

theSolver.add_kernel_optimisations(
  is_linear=True, polynomials=(exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto if args.lobatto else exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Legendre)
  ,precision=args.precision
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

    project.add_action_set_to_timestepping(
        exahype2.tracer.DiscontinuousGalerkinTracing(
            tracer_particles, theSolver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
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

project.generate_and_build()
