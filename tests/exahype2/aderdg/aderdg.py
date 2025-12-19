# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

import scenarios

available_precisions = {"bf16", "fp16", "fp32", "fp64"}

available_scenarios = {
    "AdvectionLinear": scenarios.AdvectionLinear(),
    "AcousticPlanarWaves": scenarios.AcousticPlanarWaves(dimensions=2),
    "ElasticPlanarWaves": scenarios.ElasticPlanarWaves(dimensions=2),
    "EulerGaussianBell": scenarios.EulerGaussianBell(),
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex(),
    "SWERadialDamBreak": scenarios.SWERadialDamBreak(),
    "SWERestingLake": scenarios.SWERestingLake(),
    "NavierStokesCosineBubble": scenarios.NavierStokesCosineBubble(),
    "NavierStokesTwoBubbles": scenarios.NavierStokesTwoBubbles(),
    "NavierStokesTaylorGreen": scenarios.NavierStokesTaylorGreenVortex(),
    "NavierStokesABCFlow": scenarios.NavierStokesABCFlow(),
}

parser = exahype2.ArgumentParser("ExaHyPE 2 - ADER-DG Testing Script")
parser.add_argument("-pr", "--precision", default="fp64", help="|".join(available_precisions))
parser.add_argument("-s", "--scenario", default=None, help="|".join(available_scenarios.keys()))
parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=4,
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

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name=scenario.__class__.__name__,
    directory=".",
    executable="ExaHyPE",
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
)

solver.add_kernel_optimisations(
    polynomials=polynomials,
    is_linear=scenario._equation.is_linear,
    precision=args.precision
)

solver.set_implementation(
    initial_conditions=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    max_eigenvalue=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    use_diffusive_flux=scenario._equation.use_diffusive_flux,
    ncp=scenario._equation.ncp(),
    source_term=scenario._equation.source(),
    riemann_solver=scenario._equation.riemann_solver(),
)

solver.set_plotter(args.plotter)

if scenario.analytical_solution() != exahype2.solvers.PDETerms.None_Implementation:
    exahype2.solvers.aderdg.ErrorMeasurement(
        solver,
        error_measurement_implementation=scenario.analytical_solution(),
        output_file_name="Error_" + scenario.__class__.__name__,
    )

project.add_solver(solver)
project.set_output_path("solutions")
scenario.set_global_simulation_parameters(project)
project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation("../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.build(make_clean_first=True)
