# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser()
parser.set_defaults(
    end_time=50000.0,
    min_depth=5,
)
args = parser.parse_args()

xLeft, xRight = -1514683.37644735, 1514574.46378654
yLeft, yRight = -1108652.87362612, 1176250.82246588

size = [abs(xLeft) + abs(xRight), abs(yLeft) + abs(yRight)]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVRusanovSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    number_of_enclave_tasks=args.ntasks,
)

riemann_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

riemann_solver.set_plotter(args.plotter)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="ArtificalMeteoTsunami",
    directory=".",
    executable=f"ArtificialMeteoTsunami.{args.build_mode}",
)
project.add_solver(riemann_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=[xLeft, yLeft],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

constants = {
    "GRAV": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
    "aeaEllipsoidA": [
        6378137.0,
        "double",
    ],  # Semi-major axis of the WGS84 ellipsoid (in meters)
    "aeaEllipsoidE": [
        0.08181919084262157,
        "double",
    ],  # First numerical eccentricity of the WGS84 ellipsoid
    "aeaStdParallel1": [18.3333333, "double"],  # Standard Parallel 1 (degrees)
    "aeaStdParallel2": [31.6666667, "double"],  # Standard Parallel 2 (degrees)
    "aeaLon0": [-84.0, "double"],  # Central Meridian (degrees)
    "aeaLat0": [25.0, "double"],  # Latitude of Origin (degrees)
    "aeaEarthRadius": [6371000.0, "double"],  # Projection Earth Radius
    "pointLonA": [-85.107, "double"],
    "pointLatA": [30.747, "double"],
    "pointLonB": [-81.562, "double"],
    "pointLatB": [23.4480, "double"],
    "waterDensity": [1000 + 25, "double"],  # Mean value of 24.50 and 25.50
}

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees_init}, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_h_file("FVRusanovSolver.h")
project.output.makefile.add_cpp_file("FVRusanovSolver.cpp")
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
