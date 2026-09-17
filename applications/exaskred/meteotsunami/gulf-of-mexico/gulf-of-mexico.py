# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=100.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=7)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

raster_offset = [-1514683.37644735, -1108652.87362612]
raster_size = [3029257.84023389, 2284903.696092]
mesh_side = max(raster_size)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="MeteotsunamiGulfOfMexico",
    dimensions=2,
    offset=[
        raster_offset[0] - 0.5 * (mesh_side - raster_size[0]),
        raster_offset[1] - 0.5 * (mesh_side - raster_size[1]),
    ],
    size=[mesh_side, mesh_side],
    periodic_bc=[False, False],
    executable="ExaSkred-Meteotsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Gulf of Mexico meteotsunami application is two-dimensional.")

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=max(1, args.degrees_of_freedom - 1),
    unknowns={"h": 1, "hu": project.dimensions, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.6,
)
aderdg_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
    riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation,
)
aderdg_solver.add_subcell_limiter(
    limiter_solver=exahype2.solvers.fv.godunov.subcell_scheme(
        riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation
    ),
    number_of_dmp_observables=4,
    dmp_relaxation_parameter=1.0e-2,
    dmp_differences_scaling=1.0e-3,
    dmp_observables=exahype2.solvers.PDETerms.User_Defined_Implementation,
    physical_admissibility_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
)
project.add_solver(aderdg_solver)

for name, kind, value in (
    ("bathymetryFile", "string", "bathymetry/GulfOfMexicoBathymetry.nc"),
    ("bathymetryOffset", ("double", 2), raster_offset),
    ("bathymetrySize", ("double", 2), raster_size),
    ("projectionCentralMeridian", "double", -84.0),
    ("projectionOriginLatitude", "double", 25.0),
    ("projectionFirstParallel", "double", 18.3333333),
    ("projectionSecondParallel", "double", 31.6666667),
    ("projectionSemiMajorAxis", "double", 6378137.0),
    ("projectionEccentricity", "double", 0.08181919084262157),
    ("deepestSeaFloor", "double", 7500.0),
    ("referenceOceanDepth", "double", 999.0),
    ("minimumWaterHeight", "double", 10.0),
    ("coastalDepth", "double", 20.0),
    ("waterDensity", "double", 1000.0),
    ("manningCoefficient", "double", 0.025),
    ("earthAngularVelocity", "double", 7.2921150e-5),
    ("trackStartLongitude", "double", -85.107),
    ("trackStartLatitude", "double", 30.747),
    ("trackEndLongitude", "double", -81.562),
    ("trackEndLatitude", "double", 23.4480),
    ("trackSpeed", "double", 20.0),
    ("pressureLeadingAmplitude", "double", 3761.0),
    ("pressureTrailingAmplitude", "double", 466.5),
    ("pressureLeadingWidth", "double", 0.31),
    ("pressureTrailingWidth", "double", 0.5),
    ("pressureCrossTrackWidth", "double", 1.0),
):
    project.add_runtime_parameter(name, kind, value)

project.constants.export_constexpr_with_type("g", str(9.81), "double")

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_cpp_file("../../AlbersEqualArea.cpp")
project.output.makefile.add_cpp_file("../../Forcing.cpp")
project.output.makefile.add_cpp_file("ADERDGSolver.cpp")
project.generate_and_build()
