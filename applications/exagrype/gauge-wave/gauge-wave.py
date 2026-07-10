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
    "g": 0.0,
    "Xi": 1.0,
    "E": 1.0,
    "C": 1.0,
    "Mu": 0.2,
    "Ds": 1.0,
    "Sk": 0.0,
    "Bs": 0.0,
    "K0": 0.0,
    "domain_r": domain_size / 2,
    "KOSigma": 8.0,
}

int_parameters = {
    "LapseType": 0,
    "tp_grid_setup": 0,
}

constants = {
    **{f"CCZ4{name}": (value, "double") for name, value in float_parameters.items()},
    **{f"CCZ4{name}": (value, "int") for name, value in int_parameters.items()},
}

max_eigenvalue = """
  return ::applications::exahype2::exagrype::maxEigenvalue<Shortcuts>(Q, normal % DIMENSIONS);
"""

nonconservative_product = """
  ::applications::exahype2::exagrype::ncp<Shortcuts, NumberOfUnknowns>(BTimesDeltaQ, Q, deltaQ, normal);
"""

source_term = """
  ::applications::exahype2::exagrype::source<Shortcuts>(S, Q, t, x);
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=1.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=9)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=1)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=1)
    parser.add_argument(
        "--solver",
        choices=["fv", "aderdg"],
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
    solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="CCZ4Solver",
        patch_size=args.degrees_of_freedom,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_step_relaxation=0.1,
    )
    solver.postprocess_updated_patch = """
  {
    int index = 0;
    for (int i = 0; i < {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}}; i++) {
      applications::exahype2::exagrype::enforceCCZ4constraints(QOut + index);
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""
elif args.solver == "aderdg":
    solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
        name="CCZ4Solver",
        order=args.degrees_of_freedom - 1,
        unknowns=unknowns,
        auxiliary_variables=0,
        time_step_relaxation=0.1,
    )
    solver.postprocess_updated_patch = """
  {
    int index = 0;
    for (int i = 0; i < {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}}; i++) {
      applications::exahype2::exagrype::enforceCCZ4constraints(QNew + index);
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""
else:
    raise ValueError(f"Unknown solver family: {args.solver}")

solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=nonconservative_product,
    source_term=source_term,
    max_eigenvalue=max_eigenvalue,
    flux=exahype2.solvers.PDETerms.None_Implementation,
)

if args.solver == "fv":
    exahype2.solvers.fv.ErrorMeasurement(
        solver,
        error_measurement_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
        output_file_name="Error_GaugeWave_FV",
        time_delta_between_snapshots=args.end_time / 100.0,
    )
elif args.solver == "aderdg":
    exahype2.solvers.aderdg.ErrorMeasurement(
        solver,
        error_measurement_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
        output_file_name="Error_GaugeWave_ADERDG",
        time_delta_between_snapshots=args.end_time / 100.0,
    )

solver.add_user_solver_includes(
    """
#include "InitialCondition.h"
#include "../EnforceCCZ4Constraints.h"
#include "../MaxEigenvalue.h"
#include "../NonconservativeProduct.h"
#include "../SourceTerm.h"
"""
)

project.add_solver(solver)

for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.output.makefile.add_cpp_file("CCZ4Solver.cpp")
project.output.makefile.add_cpp_file("InitialCondition.cpp")
project.generate_and_build()
