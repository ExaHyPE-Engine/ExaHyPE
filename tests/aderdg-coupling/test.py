# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys

import exahype2

sys.path.insert(0, os.path.abspath("../aderdg/"))
sys.path.insert(0, os.path.abspath("../aderdg/scenarios"))
import scenarios

available_precisions = ("float", "double", "fp32", "fp64", "fp16", "bf16")

scenario_factories = {
    "AcousticPlanarWaves": lambda: scenarios.AcousticPlanarWaves(dimensions=2),
    "AdvectionLinear": scenarios.AdvectionLinear,
    "ElasticPlanarWaves": lambda: scenarios.ElasticPlanarWaves(dimensions=2),
    "EulerGaussianBell": scenarios.EulerGaussianBell,
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex,
    "SWERadialDamBreak": scenarios.SWERadialDamBreak,
    "SWERestingLake": scenarios.SWERestingLake,
}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    parser.add_argument(
        "-p",
        "--polynomials",
        type=int,
        choices=(0, 1),
        default=1,
        help="Polynomial type, 0 is Gauss-Legendre, 1 is Gauss-Lobatto",
    )
    parser.add_argument(
        "-pr", "--precision", choices=available_precisions, default="fp64"
    )
    parser.add_argument(
        "-s", "--scenario", choices=scenario_factories.keys(), default=None
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

if args.scenario is None and args.help:
    scenario_name = "SWERadialDamBreak"
    scenario = scenario_factories[scenario_name]()
elif args.scenario is None:
    while True:
        try:
            scenario_name = input(
                "Which of the following scenarios would you like to try out?\n"
                + " - ".join(scenario_factories.keys())
                + "\n"
            )
            scenario = scenario_factories[scenario_name]()
        except KeyError:
            continue
        else:
            break
else:
    scenario_name = args.scenario
    scenario = scenario_factories[scenario_name]()

order = args.degrees_of_freedom - 1
end_time = scenario._end_time if args.end_time is None else args.end_time

basis = exahype2.dg.GaussLegendre if args.polynomials == 0 else exahype2.dg.GaussLobatto

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"],
    "ADERDGFusedCoupling",
    dimensions=scenario._dimensions,
    size=[scenario._domain_size for _ in range(3)],
    offset=[scenario._offset for _ in range(3)],
    periodic_bc=[scenario._periodic_bc for _ in range(3)],
    executable=scenario.__class__.__name__,
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != scenario._dimensions:
    raise ValueError(
        f"Scenario {scenario_name} is {scenario._dimensions}D, but the project "
        f"was configured for {project.dimensions}D."
    )

solver = exahype2.dg.ader.Solver(
    name=scenario.__class__.__name__,
    order=order,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=basis,
    linear=scenario._equation.is_linear,
    precision=args.precision,
)

solver.set_pde_terms(
    initial_condition=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    max_eigenvalue=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    ncp=scenario._equation.ncp(),
    riemann_solver=scenario._equation.riemann_solver(),
)

project.add_solver(solver)

solver2 = exahype2.dg.ader.Solver(
    name=scenario.__class__.__name__ + "2",
    order=order,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=basis,
    linear=scenario._equation.is_linear,
    precision=args.precision,
)

solver2.set_pde_terms(
    initial_condition=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    max_eigenvalue=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    ncp=scenario._equation.ncp(),
    riemann_solver=scenario._equation.riemann_solver(),
)

project.add_solver(solver2)

from fuseADERSolvers import fuse_ader_solvers

fuse_ader_solvers(
    solver,
    solver2,
    "\nmarker.getX()[0]<=(DomainOffset[0]+0.5*DomainSize[0]+0.51*marker.getH()[0])\n",
    "\nmarker.getX()[0]>=(DomainOffset[0]+0.5*DomainSize[0]+0.49*marker.getH()[0])\n",
)

if scenario.analytical_solution() is not None:
    solver.set_pde_terms(analytical_solution=scenario.analytical_solution())
    solver.measure_error(
        file="Error_" + scenario.__class__.__name__,
        every=scenario._end_time / 100.0,
    )
    solver2.set_pde_terms(analytical_solution=scenario.analytical_solution())
    solver2.measure_error(
        file="Error2_" + scenario.__class__.__name__,
        every=scenario._end_time / 100.0,
    )

project.generate_and_build()
