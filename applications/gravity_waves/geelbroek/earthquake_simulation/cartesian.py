# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

import Elastic
import numpy as np

def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-p", "--precision", type=str, default="float")
    parser.add_argument(
        "-lob",
        "--lobatto",
        action="store_true",
        help="use Gauss-Lobatto quadrature nodes, otherwise defaults to Gauss-Legendre",
    )
    parser.add_argument(
        "-ut", "--use-tracers", action=argparse.BooleanOptionalAction, default=True
    )
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
    offset=[-3, 0.0, -3],  # kilometres
    size=[10.0, 10.0, 10.0],  # kilometres
    periodic_bc=[False, False, False],
    executable="ExaSeis",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=3,
    max_grid_levels=4,
) 
if project.dimensions != 3:
    raise ValueError("The Cartesian ExaSeis application is three-dimensional.")

theSolver = exahype2.dg.ader.Solver(
    name="geelbroek",
    order=order,
    unknowns={"v": 3, "sigma": 6},
    auxiliary_variables={"rho": 1, "cp": 1, "cs": 1},
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=exahype2.dg.GaussLobatto if args.lobatto else exahype2.dg.GaussLegendre,
    linear=True,
    precision=args.precision,
    point_sources=1,
)

theSolver.set_pde_terms(
    initial_condition=Elastic.initial(),
    boundary_conditions=Elastic.boundary(),
    max_eigenvalue=Elastic.eigenvalue(),
    flux=Elastic.flux(),
    refinement_criterion=Elastic.refinement_criterion(),
    riemann_solver=Elastic.riemann_solver(),
    point_source=Elastic.point_source(),
    point_source_locations=Elastic.init_point_source(),
)

theSolver.add_includes("applications/exaseis/riemannSolvers/ElasticRiemannSolver.h")



# The robust fallback scheme a troubled cell is recomputed with. The Riemann
# solver, the DMP observables and the physical admissibility test are PDE terms.
theSolver.set_pde_terms(
    subcell_riemann_solver=Elastic.riemann_solver_fv(),
    dmp_observables=Elastic.map_observerables(),
    physical_admissibility=Elastic.physical_admissibility_criterion(),
)
theSolver.add_subcell_limiter(
    exahype2.fv.Godunov(),
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=0.001,   # absolute floor on the widening
    dmp_differences_scaling=0.001,    # widening relative to the local spread
)


project.add_solver(theSolver)
project.output.makefile.add_include_directory("../../../..")



def surface_tracer_grid(dx, x_range=(-3.0, 7.0), z_range=(-3.0, 7.0),
                        y=0.0, margin=0.05):
    xs = np.arange(x_range[0] + margin, x_range[1] - margin + 1e-9, dx)
    zs = np.arange(z_range[0] + margin, z_range[1] - margin + 1e-9, dx)
    return [[round(float(x), 4), y, round(float(z), 4)] for x in xs for z in zs]

if args.use_tracers:
    tracer_coordinates = surface_tracer_grid(0.1)
    for station in [            
                [0.000, 0.0, 0.6],     #ELE
                [0.000, 0.0, 2.7],      #ASS1
                [0.000, 0.0, 6.6],      #ASS2
                ]: 
        tracer_coordinates.append(station)
        tracer_particles = project.add_tracer(name="Tracer", particle_attributes=12)
        tracer_particles.insert_by_coordinates(tracer_coordinates)




    tracer_particles.dump_into_database(
        filename="Station",
        time_delta_between_two_snapshots=0.01,
        output_precision=10,
    )
    theSolver.trace(
        tracer_particles,
        exahype2.particles.Forces(
            interpolation_scheme=2,
            integration_scheme=0,
            force_method=0,
            fields_to_interpolate="0,1,2,3,4,5,6,7,8,9,10,11",
        ),
        initialisation=False,
    )

project.generate_and_build()
