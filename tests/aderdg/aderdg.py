# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

import scenarios

available_precisions = (
    "float",
    "double",
    "fp32",
    "fp64",
    "fp16",
    "bf16",
)

scenario_factories = {
    "AdvectionLinear": scenarios.AdvectionLinear,
    "AcousticPlanarWaves": lambda: scenarios.AcousticPlanarWaves(dimensions=2),
    "ElasticPlanarWaves": lambda: scenarios.ElasticPlanarWaves(dimensions=2),
    "ElasticRickerPulse": scenarios.ElasticRickerPulse,
    "EulerGaussianBell": scenarios.EulerGaussianBell,
    "EulerGaussianBell3D": scenarios.EulerGaussianBell3D,
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex,
    "SWERadialDamBreak": scenarios.SWERadialDamBreak,
    "SWERestingLake": scenarios.SWERestingLake,
    "SWESteadyVortex": scenarios.SWESteadyVortex,
    "NavierStokesCosineBubble": scenarios.NavierStokesCosineBubble,
    "NavierStokesTwoBubbles": scenarios.NavierStokesTwoBubbles,
    "NavierStokesTaylorGreen": scenarios.NavierStokesTaylorGreenVortex,
    "NavierStokesABCFlow": scenarios.NavierStokesABCFlow,
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
            break
else:
    scenario_name = args.scenario
    scenario = scenario_factories[scenario_name]()

order = args.degrees_of_freedom - 1
end_time = scenario._end_time if args.end_time is None else args.end_time

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
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
        f"The scenario {scenario_name} runs in {scenario._dimensions}D only."
    )

equation = scenario._equation
solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=order,
    unknowns=equation.num_unknowns,
    auxiliary_variables=equation.num_auxiliary_variables,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=exahype2.dg.GaussLegendre,
    linear=equation.is_linear,
    precision=args.precision,
)

solver.set_pde_terms(
    initial_condition=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    max_eigenvalue=equation.eigenvalues(),
    flux=None if equation.use_diffusive_flux else equation.flux(),
    diffusive_flux=equation.flux() if equation.use_diffusive_flux else None,
    ncp=equation.ncp(),
    source=equation.source(),
    riemann_solver=equation.riemann_solver(),
)

if scenario.analytical_solution() is not None:
    solver.set_pde_terms(analytical_solution=scenario.analytical_solution())
    solver.measure_error(
        file="Error_" + scenario.__class__.__name__,
        every=scenario._end_time / 100.0,
    )

project.add_solver(solver)

project.generate_and_build()
