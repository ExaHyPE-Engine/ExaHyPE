# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys

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


hpSolver = exahype2.dg.ader.Solver(
    name="hpElastic",
    order=order,
    unknowns={"v": 3, "sigma": 6},
    auxiliary_variables={"rho": 1, "cp": 1, "cs": 1},
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    linear=True,
    point_sources=1,
    precision="fp64",
)

lpSolver = exahype2.dg.ader.Solver(
    name="lpElastic",
    order=order,
    unknowns={"v": 3, "sigma": 6},
    auxiliary_variables={"rho": 1, "cp": 1, "cs": 1},
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    linear=True,
    point_sources=1,
    precision="fp32",
)

if args.use_tracers:
    tracer_particles = project.add_tracer(name="Tracer", particle_attributes=12)

    project.add_action_set_to_initialisation(
        exahype2.particles.InsertTracersByCoordinates(
            particle_set=tracer_particles,
            coordinates=[
                [0.000, 0.0, 0.693],
                [0.000, 0.0, 5.543],
                [0.000, 0.0, 10.392],
                [0.490, 0.0, 0.490],
                [3.919, 0.0, 3.919],
                [7.348, 0.0, 7.3480],
                [0.577, 0.0, 0.384],
                [4.612, 0.0, 3.075],
                [8.647, 0.0, 5.7640],
            ],
        )
    )

from fuseADERSolvers import fuse_ader_solvers

# A cell boundary of the coarsest level is a cell boundary of every finer level.
solver_interface_plane = (
    "(DomainOffset[1] + DomainSize[1] - DomainSize[1]"
    " / std::pow(peano4::sfc::ActiveCurve::SubdivisionFactor, ::runner::RuntimeParameters::getInstance().getGridLevels()))"
)
fuse_ader_solvers(
    hpSolver,
    lpSolver,
    "( marker.getX()[1] < " + solver_interface_plane + " + 0.1*marker.getH()[1] )",
    "( marker.getX()[1] > " + solver_interface_plane + " - 0.1*marker.getH()[1] )",
)


for theSolver in [hpSolver, lpSolver]:

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

    theSolver.add_includes(
        "../../applications/exaseis/riemannSolvers/cartesianRoutines.h",
        "../../applications/exaseis/riemannSolvers/riemannsolverIsotropic.h",
        "peano4/sfc/ActiveCurve.h",
        "runner/RuntimeParameters.h",
    )

    project.add_solver(theSolver)

    if args.use_tracers:
        theSolver.trace(
            tracer_particles,
            kernel="::exahype2::dg::projectAllValuesOntoParticle",
            initialisation=False,
        )

        project.add_action_set_to_timestepping(
            exahype2.particles.DumpTracerIntoDatabase(
                particle_set=tracer_particles,
                solver=theSolver,
                filename="LOH1-" + theSolver.name,
                time_delta_between_two_snapshots=0.01,
                output_precision=10,
            )
        )

project.generate_and_build()
