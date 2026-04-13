# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
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

parser = exahype2.ArgumentParser()
parser.add_argument(
    "--solver",
    choices=["fv", "aderdg"],
    default="aderdg",
    help="Pick solver family.",
)
parser.set_defaults(
    output="solutions",
    end_time=1.0,
    number_of_snapshots=10,
    degrees_of_freedom=9,
    min_depth=1,
    amr_levels=0,
    load_balancing_quality=0.95,
)
args = parser.parse_args()

project = exahype2.Project(
    namespace=["applications", "exahype2", "exagrype"],
    project_name="GaugeWave",
    directory=".",
    executable="ExaGRyPE",
)

project.set_output_path(args.output)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        True,
        True,
        True,
    ],
)

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

max_cell_h = 1.1 * min(size) / (3.0**args.min_depth)
min_cell_h = max_cell_h * 3.0 ** (-args.amr_levels)

if args.solver == "fv":
    max_volume_h = max_cell_h / args.degrees_of_freedom
    min_volume_h = min_cell_h / args.degrees_of_freedom
    solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="CCZ4Solver",
        patch_size=args.degrees_of_freedom,
        unknowns=unknowns,
        auxiliary_variables=0,
        min_volume_h=min_volume_h,
        max_volume_h=max_volume_h,
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
        min_cell_h=min_cell_h,
        max_cell_h=max_cell_h,
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
    )
elif args.solver == "aderdg":
    exahype2.solvers.aderdg.ErrorMeasurement(
        solver,
        error_measurement_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
        output_file_name="Error_GaugeWave_ADERDG",
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

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)

for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.output.makefile.add_cpp_file("CCZ4Solver.cpp")
project.output.makefile.add_cpp_file("InitialCondition.cpp")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
