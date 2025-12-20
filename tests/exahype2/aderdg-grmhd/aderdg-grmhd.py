# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4, exahype2

from numpy import pi

from GRMHD import max_eigenvalue, flux, ncp

# available_precisions = {
#     "bf16", "fp16", "fp32", "fp64"
# }

available_scenarios = ["AlfvenWave", "MichelSphericalAccretion"]

parser = exahype2.ArgumentParser("ExaHyPE 2 - ADER-DG GRMHD Testing Script")
# parser.add_argument("-pr", "--precision", default="fp64", help="|".join(available_precisions))
parser.add_argument(
    "-s", "--scenario", default="AlfvenWave", help="|".join(available_scenarios)
)
parser.set_defaults(
    min_depth=1,
    degrees_of_freedom=6,
)
args = parser.parse_args()

order = args.degrees_of_freedom - 1
max_h = 1.1 * 6.0 / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

if args.scenario == "MichelSphericalAccretion":
    dimensions = 3
    offset = [2.2, 2.2, 2.2]
    size = [6.0, 6.0, 6.0]
    end_time = 1.0
    periodic_bc = False
    function_name = "initialAccretionDisc3D"
elif args.scenario == "AlfvenWave":
    dimensions = 2
    offset = [0.0, 0.0]
    size = [2 * pi, 2 * pi]
    end_time = 16.45
    periodic_bc = True
    function_name = "AlfvenWaveTest"

if args.build_mode == "Debug":
    end_time = 0.1

initial = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, 0.,
    Q
  );
"""
)

boundary_condition = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, t,
    Qoutside
  );
"""
)

analytical_solution = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, t,
    solution
  );
"""
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name=args.scenario,
    directory=".",
    executable="ExaHyPE",
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    unknowns={
        "rho": 1,
        "vel": 3,
        "E": 1,
        "B": 3,
        "psi": 1,
        "lapse": 1,
        "shift": 3,
        "gij": 6,
    },
    auxiliary_variables=0,  # TODO: true?
)

solver.add_kernel_optimisations(
    polynomials=polynomials,
    is_linear=False,
    # precision=args.precision
)

solver.set_implementation(
    initial_conditions=initial,
    boundary_conditions=boundary_condition,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    ncp=ncp,
)

solver.add_user_solver_includes(
    """
#include "InitialData.h"
#include "PDE.h"
"""
)

solver.set_plotter(args.plotter)

exahype2.solvers.aderdg.ErrorMeasurement(
    solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error_"
    + args.scenario
    + "_md_"
    + str(args.min_depth)
    + "_o_"
    + str(order),
)

project.add_solver(solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[periodic_bc, periodic_bc, periodic_bc][0:dimensions],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.build(make_clean_first=True)
