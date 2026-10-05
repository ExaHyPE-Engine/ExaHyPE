# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

domain_size = 1.0
size = [domain_size, domain_size, domain_size]
offset = [-domain_size / 2, -domain_size / 2, -domain_size / 2]

float_parameters = {
    "Itau": 1.0,
    "K1": 0.1,
    "K2": 0.0,
    "K3": 0.5,
    "Eta": 1.0,
    "F": 0.75,
    "Xi": 1.0,
    "E": 1.0,
    "C": 1.0,
    "Mu": 0.2,
    "Ds": 1.0,
    "Sk": 0.0,
    "Bs": 0.0,
    "K0": 0.0,
}

int_parameters = {
    "LapseType": 0,
}

constants = {
    **{f"CCZ4{name}": (value, "double") for name, value in float_parameters.items()},
    **{f"CCZ4{name}": (value, "int") for name, value in int_parameters.items()},
}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("--degrees-of-freedom", type=int, default=9)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=1)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=1)
    parser.add_argument(
        "--solver",
        choices=["fv", "aderdg", "rkdg"],
        default="aderdg",
        help="Pick solver family.",
    )
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["applications", "exahype2", "exagrype"],
    project_name="GaugeWave",
    dimensions=3,
    offset=offset,
    size=size,
    periodic_bc=[True, True, True],
    executable="ExaGRyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 3:
    raise ValueError("The gauge-wave application is three-dimensional.")

unknowns = {
    "gamma_tilde": 6,
    "A_tilde": 6,
    "theta": 1,
    "gamma_hat": 3,
    "alpha": 1,
    "beta": 3,
    "b": 3,
    "grad_alpha": 3,
    "grad_beta": 9,
    "grad_gamma_tilde": 18,
    "K": 1,
    "phi": 1,
    "grad_phi": 3,
}

if args.solver == "fv":
    solver = exahype2.fv.Solver(
        name="CCZ4Solver",
        patch_size=args.degrees_of_freedom,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.1),
        scheme=exahype2.fv.Godunov(),
    )
elif args.solver == "aderdg":
    solver = exahype2.dg.ader.Solver(
        name="CCZ4Solver",
        order=args.degrees_of_freedom - 1,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.1),
    )
else:
    solver = exahype2.dg.rk.Solver(
        name="CCZ4Solver",
        rk_order=min(args.degrees_of_freedom - 1, 4),
        dg_order=args.degrees_of_freedom - 1,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_stepping=exahype2.TimeStepping.Adaptive(0.1),
    )

solver.set_pde_terms(
    initial_condition=True,
    refinement_criterion=True,
    ncp=True,
    source=True,
    max_eigenvalue=True,
    analytical_solution=True,
)
solver.postprocess_updated_patch = True

if args.solver == "fv":
    solver.measure_error(file="Error_GaugeWave_FV", every=args.end_time / 100.0)
elif args.solver == "aderdg":
    solver.measure_error(file="Error_GaugeWave_ADERDG", every=args.end_time / 100.0)
else:
    solver.measure_error(file="Error_GaugeWave_RKDG", every=args.end_time / 100.0)

project.add_solver(solver)

for const_name, (const_value, const_type) in constants.items():
    project.add_constant(const_name, const_value, const_type)

project.output.makefile.add_include_directory("../../..")
project.output.makefile.add_h_file("CCZ4Solver.cpph")
project.generate_and_build()
