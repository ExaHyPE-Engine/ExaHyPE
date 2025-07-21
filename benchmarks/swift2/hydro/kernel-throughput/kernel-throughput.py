import os
import argparse
import numpy as np

import peano4
import swift2
import swift2.sphtools as sphtools
import dastgen2

import shutil


########################################################################################
# Parse user input
########################################################################################


Kernels = [ "drift", "kick1", "kick2", "density", "force" ]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SPH kernel benchmarking script")
    parser.add_argument(
        "-j",
        "--parallel-builds",
        dest="j",
        type=int,
        default=-1,
        help="Parallel builds",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        default=False,
        help="Verbose",
    )
    parser.add_argument(
        "-d",
        "--dimensions",
        dest="dimensions",
        required=True,
        type=int,
        help="Dimensions of problem",
    )
    parser.add_argument(
        "-maxp",
        "--max-particle-number",
        dest="max_particle_number",
        default=65536,
        type=int,
        help="Max number of particles to be tested",
    )
    parser.add_argument(
        "-minp",
        "--min-particle-number",
        dest="min_particle_number",
        default=1,
        type=int,
        help="Min number of particles to be tested",
    )
    parser.add_argument(
        "-s",
        "--samples",
        dest="samples",
        default=16,
        type=int,
        help="Number of overall samples",
    )
    parser.add_argument(
        "-wis",
        "--work-item-size",
        dest="work_item_size",
        default=1024,
        type=int,
        help="Size of a work item",
    )
    parser.add_argument(
        "-wir",
        "--work-item-repeats",
        dest="work_item_repeats",
        default=1,
        type=int,
        help="Size of a work item",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output_file",
        default="kernel-benchmarks",
        help="Executable file name",
    )
    parser.add_argument(
        "-pd",
        "--peano-dir",
        dest="peanodir",
        default="../../../..",
        help="Peano4 directory",
    )
    parser.add_argument(
        "-m",
        "--mode",
        dest="mode",
        choices=["release", "stats", "trace", "asserts", "debug"],
        default="release",
        help="Pick build type",
    )
    parser.add_argument(
        "--enable-kernel",
        dest="kernels",
        choices=Kernels,
        default=Kernels,
        nargs="+",
        help="Pick kernels that have to be tested",
    )
    parser.add_argument(
        "--enable-soa-conversion",
        dest="soa_conversion",
        choices=['true', 'false', 'ignore'],
        default='ignore',
        help="Enable compiler-driver AoS to SoA conversions. Triggers adding flags to the compiler call, while ignore adds nothing and can therefore be used with unpatched compilers, too"
    )
    parser.add_argument(
        "--enable-soa-offload",
        dest='soa_offload',
        choices=['true', 'false'],
        default='false',
        help="Enable OpenMP offload for converted kernels - you still must ensure OpenMP flags are set up correctly!",
    )
    parser.add_argument(
        "--validate",
        dest="validate",
        choices=['true', 'false'],
        default='true',
        help="Validate that results of all kernels yield the same result"
    )
    args = parser.parse_args()

    ########################################################################################
    # Create SWIFT project and particle species
    ########################################################################################

    project_namespace = ["benchmarks", "swift2", "hydro", "kernel_throughput"]

    project = swift2.Project(
        namespace=project_namespace, project_name="Benchmarks", executable=args.output_file
    )

    # Initialise the SPH particle --------------------------------------------------
    # If multiple particle species are present, we can specify the parameters
    # independently for each case.

    name = "HydroPart"

    dimensions = args.dimensions
    hydro_dimensions = 2  # dimensions for the hydro

    particle = swift2.particle.SPHLeapfrogFixedSearchRadius(
        name=name,
        dimensions_hydro=hydro_dimensions,
        cfl_factor=0.01,
        initial_time_step_size=0.01,
        constant_time_step_size=True,
        swift_project_namespace=project_namespace,
        particles_per_cell=100,
        min_h=1.0,
        max_h=1.0,
    )

    ########################################################################################
    # Set parameters for the SPH simulation
    #
    # We mainly use default arguments and we stick to the minimum that we need to let the
    # code compile.
    ########################################################################################

    # HYDRO_DIMENSION can be smaller than the Peano4 dimensions
    HYDRO_DIMENSIONS = hydro_dimensions

    # Time integration parameters
    # GLOBAL_TIME_STEP_SIZE = args.timestep_size
    #   CFL_CONSTANT = args.cfl_factor

    # EoS parameter ---------------------------------------------------------------
    gamma_hydro_list = [str(5 / 3), str(7 / 5), str(4 / 3), str(2 / 1)]
    gamma_hydro_list_symbols = [
        "HYDRO_GAMMA_5_3",
        "HYDRO_GAMMA_7_5",
        "HYDRO_GAMMA_4_3",
        "HYDRO_GAMMA_2_1",
    ]
    GAMMA = gamma_hydro_list[0]
    GAMMA_HYDRO_SYMBOL = gamma_hydro_list_symbols[0]

    # SPH kernel -------------------------------------------------------

    kernel = "quartic_spline"
    KERNEL_SYMBOL = sphtools.sph_kernel_macro_name[kernel]
    KERNEL_SUPPORT_RAD = sphtools.sph_kernel_H_over_h[HYDRO_DIMENSIONS - 1][kernel]

    particle.h_hydro_max = 0.01

    ########################################################################################
    # Add particle species to project
    #
    # Notice that you any change to the particle will not take effect after adding
    # the particle_set to the project
    ########################################################################################
    particle.data.expose_all_attributes_in_header_file()
    particle_set = project.add_particle_species(particle)

    ########################################################################################
    # Set global simulation parameters
    #
    # Most of them are fake in the sense that we won't use them anyway. Build modes for
    # example are used however.
    ########################################################################################
    if args.mode == "release":
        build_mode = peano4.output.CompileMode.Release
    if args.mode == "stats":
        build_mode = peano4.output.CompileMode.Stats
    if args.mode == "asserts":
        build_mode = peano4.output.CompileMode.Asserts
    if args.mode == "trace":
        build_mode = peano4.output.CompileMode.Trace
    if args.mode == "debug":
        build_mode = peano4.output.CompileMode.Debug

    # Set Peano4 simulation parameters ---------------------------------------------
    offset = [0, 0, 0]
    domain_size = [1, 1, 1]
    periodic_boundary_conditions = [False, False, False]

    project.set_global_simulation_parameters(
        dimensions=dimensions,
        offset=offset,
        domain_size=domain_size,
        min_end_time=1.0,
        max_end_time=0.0,
        first_plot_time_stamp=0.0,
        time_in_between_plots=0.0,
        periodic_BC=periodic_boundary_conditions,
        plotter_precision=8,
    )

    project.set_Peano4_installation(args.peanodir, build_mode)


    ###############################################################################
    # Generate plain Peano 4 project
    ###############################################################################
    #
    # The value of some constants such as the adiabatic index cannot be arbitrary
    # but rather we have a set of pre-defined options. The same happens for some
    # functions, e.g. the SPH smoothing kernel. This allows us to define some
    # symbols and fix the values at compile time.
    #
    #
    ###############################################################################

    peano4_project = project.generate_Peano4_project(verbose=args.verbose)

    # Some sanity checks ----------------------------------------------------------
    if GAMMA not in gamma_hydro_list:
        print(f"Please check the value of GAMMA. You have chosen: {GAMMA}")

    # Export Constants.h file ------------------------------------------------------
    peano4_project.constants.export_const_with_type(
        "HYDRO_DIMENSIONS", str(HYDRO_DIMENSIONS), "double"
    )
    #    peano4_project.constants.export_const_with_type(
    #        "HYDRO_PART_NUMBER", str(HYDRO_PART_NUMBER), "double"
    #    )
    #    peano4_project.constants.export_const_with_type(
    #        "GLOBAL_TIME_STEP_SIZE", str(GLOBAL_TIME_STEP_SIZE), "double"
    #    )
    peano4_project.constants.export_const_with_type("GAMMA", GAMMA, "double")

    # Export symbols for compilation (e.g. for SPH kernel) -----------------------
    peano4_project.output.makefile.add_CXX_flag(
        "-D" + "HYDRO_DIMENSION=" + str(HYDRO_DIMENSIONS)
    )
    peano4_project.output.makefile.add_CXX_flag("-D" + GAMMA_HYDRO_SYMBOL)
    peano4_project.output.makefile.add_CXX_flag("-D" + KERNEL_SYMBOL)

    if args.validate == 'true':
        peano4_project.output.makefile.add_CXX_flag("-DValidateSWIFTKernelOutcomes")
        
    if args.soa_conversion == 'true':
        peano4_project.output.makefile.add_CXX_flag("-fsoa-conversion-attributes")
    elif args.soa_conversion == 'false':
        peano4_project.output.makefile.add_CXX_flag("-fno-soa-conversion-attributes")
    else:
        pass # do not add any flags that an unsuspecting compiler might be unaware of
    if args.soa_offload == 'true':
        peano4_project.output.makefile.add_CXX_flag("-DSOA_OFFLOAD")

    peano4_project.output.makefile.add_cpp_file( "NohProblem.cpp" )

    peano4_project.constants.export_const_with_type("Benchmark_MaxParticles", str(args.max_particle_number), "int")
    peano4_project.constants.export_const_with_type("Benchmark_MinParticles", str(args.min_particle_number), "int")
    peano4_project.constants.export_const_with_type("Benchmark_Samples", str(args.samples), "int")
    peano4_project.constants.export_const_with_type("Benchmark_WorkItemSize", str(args.work_item_size), "int")
    peano4_project.constants.export_const_with_type("Benchmark_WorkItemRepeats", str(args.work_item_repeats), "int")

    for kernel in Kernels:
      peano4_project.constants.export_boolean("Benchmark_Test" + kernel, kernel in args.kernels )



    ###############################################################################
    # Compilation
    #
    # Before we start the compilation, we add our real benchmark driver.
    ###############################################################################
    peano4_project.output.makefile.add_cpp_file("BenchmarkDriver.cpp")

    dirs = ["vertexdata/", "repositories/", "globaldata/", "observers/", "Constants.h"]
    for dir in dirs:
        shutil.rmtree(dir, ignore_errors=True)

    peano4_project.generate(
        overwrite=peano4.output.Overwrite.Default, throw_away_data_after_generation=True
    )
    peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
