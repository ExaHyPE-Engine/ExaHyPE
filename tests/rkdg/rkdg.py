# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath("../aderdg"))
import scenarios

available_scenarios = {
    "AdvectionLinear": scenarios.AdvectionLinear(),
    "AcousticPlanarWaves": scenarios.AcousticPlanarWaves(dimensions=2),
    "ElasticPlanarWaves": scenarios.ElasticPlanarWaves(dimensions=2),
    "EulerGaussianBell": scenarios.EulerGaussianBell(),
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex(),
    # "SWERadialDamBreak": scenarios.SWERadialDamBreak(),
    "SWERestingLake": scenarios.SWERestingLake(),
}

parser = exahype2.ArgumentParser("ExaHyPE 2 - RK-DG Testing Script")
parser.add_argument(
    "-s", "--scenario", default=None, help="|".join(available_scenarios.keys())
)
parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=3,
)
args = parser.parse_args()

if args.scenario is None:
    while True:
        try:
            s = input(
                "Which of the following scenarios would you like to test?\n"
                + " - ".join(available_scenarios.keys())
                + "\n"
            )
            scenario = available_scenarios[s]
        except KeyError:
            continue
        else:
            # User has specified a valid scenario
            break
else:
    scenario = available_scenarios[args.scenario]

order = args.degrees_of_freedom - 1
max_h = 1.1 * scenario._domain_size / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

project = exahype2.Project(
    namespace=["tests", "exahype2", "rkdg"],
    project_name=scenario.__class__.__name__,
    directory=".",
    executable="ExaHyPE",
)

solver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep(
    name="RKDGSolver",
    rk_order=order,
    polynomials=exahype2.solvers.GaussLegendreBasis(4),
    min_cell_h=min_h,
    max_cell_h=max_h,
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
if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = scenario._end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=scenario._dimensions,
    offset=[scenario._offset for i in range(scenario._dimensions)],
    size=[scenario._domain_size for i in range(scenario._dimensions)],
    min_end_time=scenario._end_time,
    max_end_time=scenario._end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[scenario._periodic_bc for i in range(scenario._dimensions)],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.build(make_clean_first=True)
