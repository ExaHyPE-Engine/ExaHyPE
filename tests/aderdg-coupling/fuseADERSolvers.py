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
        default=0,
        help="Polynomial type, 0 is Gauss-Legendre, 1 is Gauss-Lobatto",
    )
    parser.add_argument(
        "-pr", "--precision", choices=available_precisions, default="fp64"
    )
    parser.add_argument(
        "-pr2",
        "--second-precision",
        choices=available_precisions,
        default="fp32",
        help="Precision of the solver that owns the upper part of the domain",
    )
    parser.add_argument(
        "-s",
        "--scenario",
        choices=scenario_factories.keys(),
        default="SWERadialDamBreak",
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
scenario = scenario_factories[args.scenario]()
order = args.degrees_of_freedom - 1
end_time = scenario._end_time if args.end_time is None else args.end_time

basis = exahype2.dg.GaussLegendre if args.polynomials == 0 else exahype2.dg.GaussLobatto

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"],
    "ADERDGCoupling",
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
        f"The scenario {args.scenario} runs in {scenario._dimensions}D only."
    )

# The plane between the two solvers is a cell boundary of the coarsest level and therefore of
# every finer level.
interface_plane = (
    "(DomainOffset(0) + DomainSize(0) / peano4::sfc::ActiveCurve::SubdivisionFactor)"
)

solvers = []
for name, precision, predicate in (
    (scenario.__class__.__name__, args.precision, "x(0) < " + interface_plane),
    (
        scenario.__class__.__name__ + "2",
        args.second_precision,
        "x(0) > " + interface_plane,
    ),
):
    solver = exahype2.dg.ader.Solver(
        name=name,
        order=order,
        unknowns=scenario._equation.num_unknowns,
        auxiliary_variables=scenario._equation.num_auxiliary_variables,
        time_stepping=exahype2.TimeStepping.Adaptive(0.9),
        basis=basis,
        linear=scenario._equation.is_linear,
        precision=precision,
    )
    solver.set_pde_terms(
        initial_condition=scenario.initial_conditions(),
        boundary_conditions=scenario.boundary_conditions(),
        max_eigenvalue=scenario._equation.eigenvalues(),
        flux=scenario._equation.flux(),
        ncp=scenario._equation.ncp(),
        riemann_solver=scenario._equation.riemann_solver(),
    )
    if scenario.analytical_solution() is not None:
        solver.set_pde_terms(analytical_solution=scenario.analytical_solution())
        solver.measure_error(file="Error_" + name, every=scenario._end_time / 100.0)
    solver.restrict_to_cells(predicate)
    solver.add_includes("peano4/sfc/ActiveCurve.h")
    project.add_solver(solver)
    solvers.append(solver)

project.couple(solvers[0], solvers[1])

project.generate_and_build()
