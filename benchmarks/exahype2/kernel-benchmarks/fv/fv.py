# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

# Use only the Euler application for now. TODO: This should be generalised later!
sys.path.insert(0, os.path.abspath("../../../../applications/exahype2/euler"))
from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 2
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[Shortcuts::rhoU + 2]  = 0.0;
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = -Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 Finite Volumes Kernel Benchmarking Script")
parser.set_defaults(
    min_depth=4,
    degrees_of_freedom=16,
)

parser.add_argument(
    "-s",
    "--samples",
    type=int,
    default=0,
    help="Set number of samples for benchmarking, if not set, run until scenario end",
)

args = parser.parse_args()

size = [1.0, 1.0, 1.0]

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarks",
    directory=".",
    executable="KernelBenchmarks",
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=(1.1 * min(size[0 : args.dimensions]) / (3.0**args.min_depth)),
    max_volume_h=(1.1 * min(size[0 : args.dimensions]) / (3.0**args.min_depth)),
    time_step_relaxation=0.5,
    use_enclave_tasking=True,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=size[0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[False] * args.dimensions,
)

project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)

project = project.generate_Peano4_project(verbose=False)

project.constants.export_constexpr_with_type(
    "GridLength", str(3**args.min_depth), "int"
)
project.constants.export_const_with_type(
    "PlotDescription",
    f'"{fv_solver.plot_description}"',
    "std::string",
)
project.constants.export_constexpr_with_type(
    "ShouldPlot", "true" if args.number_of_snapshots > 0 else "false", "bool"
)

project.constants.export_constexpr_with_type(
    "UseAMR", "true" if args.amr_levels > 0 else "false", "bool"
)

project.constants.export_constexpr_with_type("Samples", str(args.samples), "int")

makefile = project.output.makefile
makefile.add_cpp_file("KernelBenchmarks-main.cpp")

makefile.add_h_file("DataRepository.h")
makefile.add_cpp_file("DataRepository.cpp")

makefile.add_h_file("EnclaveDataRepository.h")
makefile.add_cpp_file("EnclaveDataRepository.cpp")

makefile.set_target_device(args.target_device)
makefile.add_CXX_flag("-DGAMMA=1.4")

project.build()
