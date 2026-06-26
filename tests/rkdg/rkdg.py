# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys

import exahype2

sys.path.insert(0, os.path.abspath("../aderdg"))
import scenarios

scenario_factories = {
    "AdvectionLinear": scenarios.AdvectionLinear,
    "AcousticPlanarWaves": lambda: scenarios.AcousticPlanarWaves(dimensions=2),
    "ElasticPlanarWaves": lambda: scenarios.ElasticPlanarWaves(dimensions=2),
    "EulerGaussianBell": scenarios.EulerGaussianBell,
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex,
    # "SWERadialDamBreak": scenarios.SWERadialDamBreak(),
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
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=3)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    parser.add_argument("-s", "--scenario", choices=scenario_factories.keys(), default=None)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

if args.scenario is None and args.help:
    scenario_name = "AdvectionLinear"
    scenario = scenario_factories[scenario_name]()
elif args.scenario is None:
    while True:
        try:
            s = input(
                "Which of the following scenarios would you like to test?\n"
                + " - ".join(scenario_factories.keys())
                + "\n"
            )
            scenario_name = s
            scenario = scenario_factories[scenario_name]()
        except KeyError:
            continue
        else:
            # User has specified a valid scenario
            break
else:
    scenario_name = args.scenario
    scenario = scenario_factories[scenario_name]()

order = args.degrees_of_freedom - 1
end_time = scenario._end_time if args.end_time is None else args.end_time

project = exahype2.Project(
    namespace=["tests", "exahype2", "rkdg"],
    project_name=scenario.__class__.__name__,
    dimensions=scenario._dimensions,
    size=[scenario._domain_size for _ in range(3)],
    offset=[scenario._offset for _ in range(3)],
    periodic_bc=[scenario._periodic_bc for _ in range(3)],
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

solver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep(
    name="RKDGSolver",
    rk_order=order,
    polynomials=exahype2.solvers.GaussLegendreBasis(4),
    time_step_relaxation=0.9,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
)

solver.set_implementation(
    initial_conditions=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    eigenvalues=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    ncp=scenario._equation.ncp(),
)

project.add_solver(solver)

project.generate_and_build()
