# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sys, os, argparse
import peano4, exahype2

sys.path.insert(0, os.path.abspath("../../../../tests/exahype2/aderdg"))
import scenarios

available_scenarios = {
    "AcousticPlanarWaves2D": scenarios.AcousticPlanarWaves(dimensions=2),
    "AcousticPlanarWaves3D": scenarios.AcousticPlanarWaves(dimensions=3),
    "AdvectionLinear": scenarios.AdvectionLinear(),
    "EulerGaussianBell": scenarios.EulerGaussianBell(),
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex(),
    "SWERadialDamBreak": scenarios.SWERadialDamBreak(),
    "SWERestingLake": scenarios.SWERestingLake(),
}
parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Finite Volumes Kernel Benchmarking Script"
)

parser.add_argument(
    "-m",
    "--build-mode",
    choices=peano4.output.CompileModes,
    default=peano4.output.CompileModes[0],  # Release
    help="|".join(peano4.output.CompileModes),
)

parser.add_argument(
    "-d",
    "--depth",
    type=int,
    default=3,
    help="Number of levels in the tree. Must be > 1.",
)

parser.add_argument(
    "-ps",
    "--patch-size",
    type=int,
    default=16,
    help="Number of finite volumes in a patch",
)

parser.add_argument(
    "-sc",
    "--scenario",
    choices=available_scenarios.keys(),
    default="SWERadialDamBreak",
    help="Scenario that should be used as a base for the benchmarking",
)

parser.add_argument(
    "-p",
    "--plot",
    action="store_true",
    help="Set if you want the simulation output",
)

args = parser.parse_args()
scenario = available_scenarios[args.scenario]
dimensions = scenario._dimensions
depth = args.depth
end_time = scenario._end_time
should_plot = args.plot
size = [scenario._domain_size, scenario._domain_size, scenario._domain_size]
offset = [scenario._offset, scenario._offset, scenario._offset]
plot_description = ""
variable_names = []

if type(scenario._equation.num_unknowns) is dict:
    variable_names += list(scenario._equation.num_unknowns.keys())
if type(scenario._equation.num_auxiliary_variables) is dict:
    variable_names += list(scenario._equation.num_auxiliary_variables.keys())

plot_description = ", ".join(variable_names)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarks",
    directory=".",
    executable="KernelBenchmarks",
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=int(args.patch_size),
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
    min_volume_h=(1.1 * min(size[0:dimensions]) / (3.0**depth)),
    max_volume_h=(1.1 * min(size[0:dimensions]) / (3.0**depth)),
    time_step_relaxation=0.5,
    use_enclave_tasking=True,
)

fv_solver.set_implementation(
    initial_conditions=scenario.initial_conditions(),
    boundary_conditions="""
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
""",
    flux=scenario._equation.flux(),
    max_eigenvalue=scenario._equation.eigenvalues(),
)

project.add_solver(fv_solver)

project.set_global_simulation_parameters(
    dimensions=dimensions,
    size=size[0:dimensions],
    offset=offset[0:dimensions],
    min_end_time=scenario._end_time,
    max_end_time=scenario._end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.1,
    periodic_BC=[False] * dimensions,
)

project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)

project.set_output_path("solutions")

project = project.generate_Peano4_project(verbose=False)

project.constants.export_constexpr_with_type("GridLength", str(3**depth), "int")
project.constants.export_const_with_type(
    "PlotDescription",
    f'"{plot_description}"',
    "std::string",
)
project.constants.export_constexpr_with_type(
    "ShouldPlot", "true" if should_plot else "false", "bool"
)

makefile = project.output.makefile
makefile.add_cpp_file("KernelBenchmarks-main.cpp")
makefile.add_h_file("DataRepository.h")
makefile.add_cpp_file("DataRepository.cpp")
project.build()
