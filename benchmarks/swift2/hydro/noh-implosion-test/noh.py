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

Parallelisation_DomainDecomposition = "domain-decomposition"
Parallelisation_TaskTree = "task-tree"
Parallelisation_TaskGraph = "task-graph"
ParallelisationVariants = [
    Parallelisation_DomainDecomposition,
    Parallelisation_TaskTree,
    Parallelisation_TaskGraph,
]

Storage_Scattered = "scattered"
Storage_ContinuousPerVertex = "continuous-per-vertex"
Storage_GlobalContinuous = "global-continuous"
StorageVariants = [Storage_ContinuousPerVertex, Storage_GlobalContinuous]

Sorting_MultiscaleSort = "multiscale-sort"
Sorting_BucketSort = "bucket-sort"
SortingVariants = [Sorting_MultiscaleSort, Sorting_BucketSort]

KernelOptimisation_NoOptimisation = "no-optimisation"
KernelOptimisation_OuterGuards = "outer-guards"
KernelOptimisation_VectoriseAll = "vectorise-all"
KernelOptimisation_VectoriseDistanceChecks = "vectorise-distance-checks"

BasicKernelOptimisationVariants = [
    KernelOptimisation_NoOptimisation,
    KernelOptimisation_OuterGuards,
]
AllKernelOptimisationVariants = [
    KernelOptimisation_NoOptimisation,
    KernelOptimisation_OuterGuards,
    KernelOptimisation_VectoriseAll,
    KernelOptimisation_VectoriseDistanceChecks,
]


Choices = [
    parallelisation_variant + "." + storage + "." + sorting + "." + kernel_optimisation
    for parallelisation_variant in ParallelisationVariants
    for storage in [Storage_Scattered]
    for sorting in SortingVariants
    for kernel_optimisation in BasicKernelOptimisationVariants
] + [
    parallelisation_variant + "." + storage + "." + sorting + "." + kernel_optimisation
    for parallelisation_variant in ParallelisationVariants
    for storage in StorageVariants
    for sorting in SortingVariants
    for kernel_optimisation in AllKernelOptimisationVariants
]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SPH benchmarking script")
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
        "-c",
        "--clean",
        dest="clean_temporary_files",
        action="store_true",
        help="Remove temporary directories and output files before script starts",
    )
    parser.add_argument(
        "-np",
        "--particle-number",
        dest="particle_number",
        required=True,
        type=int,
        help="Particle number (1D)",
    )
    parser.add_argument(
        "-t",
        "--trees",
        dest="trees",
        default=0,
        type=int,
        help="Number of trees to be used",
    )
    parser.add_argument(
        "-et",
        "--end-time",
        dest="end_time",
        type=float,
        default=0.2,
        help="End of simulation",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output_file",
        default="noh2D",
        help="Executable file name",
    )
    parser.add_argument(
        "-plot",
        "--plot-delta",
        dest="plot_delta",
        type=float,
        default=0.01,
        help="Plot delta (0 switches off)",
    )
    parser.add_argument(
        "-dt",
        "--timestep-size",
        dest="timestep_size",
        type=float,
        default=1e-4,
        help="Global timestep size (initial value)",
    )
    parser.add_argument(
        "-cfl",
        "--cfl-factor",
        dest="cfl_factor",
        type=float,
        default=0.1,
        help="Value of the CFL constant",
    )
    parser.add_argument(
        "-ppc",
        "--particles-per-cell",
        dest="particles_per_cell",
        type=int,
        default=200,
        help="Particles per cell (200 is default)",
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
        "-rea",
        "--realisation",
        dest="realisation",
        choices=Choices,
        default=Choices[0],
        help="Realisation to use",
    )
    parser.add_argument(
        "-ms",
        "--mantissa-size",
        dest="mantissa_size",
        type=int,
        default=0,
        help="Mantissa size for clang truncation (if supported by compiler). Default of 0 disables any compression",
    )
    parser.add_argument(
        "-k",
        "--kernel",
        dest="kernel",
        type=str,
        default="quartic_spline",
        choices=sphtools.sph_kernel_list,
        help="SPH kernel function to use.",
    )
    args = parser.parse_args()

    ########################################################################################
    # Generate SWIFT project
    ########################################################################################

    project_namespace = ["benchmarks", "swift2", "noh"]

    project = swift2.Project(
        namespace=project_namespace, project_name="Noh", executable=args.output_file
    )

    if (
        args.realisation.split(".")[0] == Parallelisation_DomainDecomposition
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_separate_mesh_traversals_multiscale_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_separate_mesh_traversals_multiscale_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_DomainDecomposition
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_separate_mesh_traversals_bucket_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_separate_mesh_traversals_bucket_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_DomainDecomposition
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_separate_mesh_traversals_multiscale_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_separate_mesh_traversals_multiscale_sort_coalesced_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_DomainDecomposition
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_separate_mesh_traversals_bucket_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_separate_mesh_traversals_bucket_sort_coalesced_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskTree
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_tree_multiscale_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_tree_multiscale_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskTree
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_tree_bucket_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_tree_bucket_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskTree
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_tree_multiscale_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_tree_multiscale_sort_coalesced_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskTree
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_tree_bucket_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_tree_bucket_sort_coalesced_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskGraph
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_graph_multiscale_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_graph_multiscale_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskGraph
        and args.realisation.split(".")[1] == Storage_Scattered
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_graph_bucket_sort_scattered_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_graph_bucket_sort_scattered_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskGraph
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_MultiscaleSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_graph_multiscale_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_graph_multiscale_sort_coalesced_memory
        )
    elif (
        args.realisation.split(".")[0] == Parallelisation_TaskGraph
        and (
            args.realisation.split(".")[1] == Storage_ContinuousPerVertex
            or args.realisation.split(".")[1] == Storage_GlobalContinuous
        )
        and args.realisation.split(".")[2] == Sorting_BucketSort
    ):
        project.algorithm_steps_task_graph_compiler = (
            swift2.api.graphcompiler.particle_steps_onto_task_graph_bucket_sort_coalesced_memory
        )
        project.initialisation_steps_task_graph_compiler = (
            swift2.api.graphcompiler.initialisation_steps_onto_task_graph_bucket_sort_coalesced_memory
        )
    else:
        assert False, "realisation " + args.realisation + " not supported"

    # Initialise the SPH particle --------------------------------------------------
    # If multiple particle species are present, we can specify the parameters
    # independently for each case.

    name = "HydroPart"

    dimensions = 2
    hydro_dimensions = 2  # dimensions for the hydro

    particles_per_cell = args.particles_per_cell
    total_number_of_particles = args.particle_number**hydro_dimensions
    total_number_of_cells = total_number_of_particles / particles_per_cell
    cells_per_dimension = total_number_of_cells ** (1.0 / hydro_dimensions)
    max_cell_size = 1.0 / cells_per_dimension * 3.0

    print(
        "{} particles in total fit into {} cells, i.e. {} cells per dimension. This gives h={}".format(
            total_number_of_particles,
            total_number_of_cells,
            cells_per_dimension,
            max_cell_size,
        )
    )

    if args.realisation.split(".")[3] == KernelOptimisation_NoOptimisation:
        particle_interaction_kernel_realisation = (
            swift2.particle.SPHLeapfrogFixedSearchRadius.ParticleKernelRealisation.NO_OPTIMISATION
        )
    elif args.realisation.split(".")[3] == KernelOptimisation_OuterGuards:
        particle_interaction_kernel_realisation = (
            swift2.particle.SPHLeapfrogFixedSearchRadius.ParticleKernelRealisation.USE_OUTER_GUARDS
        )
    elif args.realisation.split(".")[3] == KernelOptimisation_VectoriseDistanceChecks:
        particle_interaction_kernel_realisation = (
            swift2.particle.SPHLeapfrogFixedSearchRadius.ParticleKernelRealisation.VECTORISE_WITH_SEPARATE_DISTANCE_CHECKS
        )
    elif args.realisation.split(".")[3] == KernelOptimisation_VectoriseAll:
        particle_interaction_kernel_realisation = (
            swift2.particle.SPHLeapfrogFixedSearchRadius.ParticleKernelRealisation.VECTORISE_ALL
        )
    else:
        raise NotImplemented()

    particle = swift2.particle.SPHLeapfrogFixedSearchRadius(
        name=name,
        dimensions_hydro=hydro_dimensions,
        cfl_factor=args.cfl_factor,
        initial_time_step_size=args.timestep_size,
        constant_time_step_size=True,
        swift_project_namespace=project_namespace,
        particles_per_cell=particles_per_cell,
        min_h=max_cell_size,
        max_h=max_cell_size,
        particle_interaction_kernel_realisation=particle_interaction_kernel_realisation,
    )

    ########################################################################################
    # Set parameters for the SPH simulation
    # If no parameters are passed to the swift2 project, it initializes with
    # default values. The values of the parameters can be modified or retrieved
    # using the set/get methods.
    ########################################################################################

    # HYDRO_DIMENSION can be smaller than the Peano4 dimensions
    HYDRO_DIMENSIONS = hydro_dimensions

    # Particle number along 1 dimension
    HYDRO_PART_NUMBER = args.particle_number

    # Time integration parameters
    GLOBAL_TIME_STEP_SIZE = args.timestep_size
    CFL_CONSTANT = args.cfl_factor

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

    kernel = args.kernel
    KERNEL_SYMBOL = sphtools.sph_kernel_macro_name[kernel]
    KERNEL_SUPPORT_RAD = sphtools.sph_kernel_H_over_h[HYDRO_DIMENSIONS - 1][kernel]

    hydro_h_max = max_cell_size / 2
    particle.h_hydro_max = hydro_h_max

    # particle.h_hydro_min = 1e-6
    # particle.h_max_iterations = 10
    # particle.h_tolerance = 1e-4
    # particle.eta_factor = 1.2348
    # particle.beta_av = 3.0
    # particle.alpha_av = 0.8

    # IMPORTANT: Reduce the mantissa size for particle attributes -------------------
    # Native size is 53 (double precision)
    if args.mantissa_size > 0:
        particle.mass.valid_mantissa_bits = args.mantissa_size
        particle.velocity.valid_mantissa_bits = args.mantissa_size
        particle.acceleration.valid_mantissa_bits = args.mantissa_size
        particle.density.valid_mantissa_bits = args.mantissa_size
        particle.pressure.valid_mantissa_bits = args.mantissa_size
        particle.smoothL.valid_mantissa_bits = args.mantissa_size
        particle.u.valid_mantissa_bits = args.mantissa_size
        particle.uDot.valid_mantissa_bits = args.mantissa_size
        particle.f.valid_mantissa_bits = args.mantissa_size
        particle.wcount_dh.valid_mantissa_bits = args.mantissa_size
        particle.rho_dh.valid_mantissa_bits = args.mantissa_size
        particle.hleft.valid_mantissa_bits = args.mantissa_size
        particle.hright.valid_mantissa_bits = args.mantissa_size
        particle.smoothL_prev.valid_mantissa_bits = args.mantissa_size
        particle.wcount.valid_mantissa_bits = args.mantissa_size
        particle.hDot.valid_mantissa_bits = args.mantissa_size
        particle.residual.valid_mantissa_bits = args.mantissa_size
        particle.balsara.valid_mantissa_bits = args.mantissa_size
        particle.rot_v.valid_mantissa_bits = args.mantissa_size
        particle.div_v.valid_mantissa_bits = args.mantissa_size
        particle.v_sig_AV.valid_mantissa_bits = args.mantissa_size
        particle.soundSpeed.valid_mantissa_bits = args.mantissa_size
        particle.v_full.valid_mantissa_bits = args.mantissa_size
        particle.u_full.valid_mantissa_bits = args.mantissa_size

    particle.data.expose_all_attributes_in_header_file()

    # Add particle to the SWIFT project --------------------------------------------
    # Notice that you any change to the particle will not take effect after adding
    # the particle_set to the project
    particle_set = project.add_particle_species(particle)

    if args.realisation.split(".")[1] == Storage_ContinuousPerVertex:
        particle_set.generator = (
            peano4.toolbox.particles.ParticleSetGenerator_ContinuousPerVertex(
                particle_set
            )
        )
        print("Benchmark: Use continuous per vertex data layout")
    elif args.realisation.split(".")[1] == Storage_GlobalContinuous:
        particle_set.generator = (
            peano4.toolbox.particles.ParticleSetGenerator_GlobalContinuous(particle_set)
        )
        print("Benchmark: Use globally continuous data layout")
    else:
        particle_set.generator = (
            peano4.toolbox.particles.ParticleSetGenerator_ScatteredOnHeap_IndexByList(
                particle_set
            )
        )

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
        min_end_time=args.end_time,
        max_end_time=0.0,
        first_plot_time_stamp=0.0,
        time_in_between_plots=args.plot_delta,
        periodic_BC=periodic_boundary_conditions,
        plotter_precision=8,
    )

    project.set_Peano4_installation(args.peanodir, build_mode)

    project.set_load_balancing(
        "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
        "new toolbox::loadbalancing::DefaultConfiguration({})".format(args.trees),
    )

    ###############################################################################
    # Initialisation snippet for the particle
    ###############################################################################

    #  Noh problem (2D) snippet  --------------------------------------------------
    #  The particle attributes are set via an initalisation snippet that sets
    #  the initial conditions specific for this problem, i.e. the velocity field
    #  that converges towards the center of the domain and that generates the
    #  collapse.

    ICs = swift2.scenario.NohProblemIC(dimensions=HYDRO_DIMENSIONS)
    initialisation_snippet = ICs.initialisation_snippet

    project.algorithm_step_initial_conditions.add_action_set(
        swift2.input.InsertParticlesAlongCartesianGrid(
            particle_set=particle_set,
            initialisation_call=initialisation_snippet,
            distance_between_particles_along_one_axis=1.0 / args.particle_number,
            additional_includes="""
    #include "swift2/kernels/legacy/equation_of_state.h"
    """,
        )
    )

    ###############################################################################
    # Set up the plotter
    ###############################################################################
    #
    # A plotter has to be told explicitly which data to visualisation. We visualise
    # the velocity here as scalar attribute. To some degree, this is redundant info
    # as we implicitly have the position of the particle and can "see" its
    # velocity when we play through a movie. But the explicit visualisation shows
    # how to handle attributes per particle.
    #
    # When we create the plotter, we have to inform it how to enumerate the
    # individual snapshots. Obviously, we can just count through the snapshots.
    # In this case, you have to pass the constant peano4.toolbox.particles.PlotParticlesInVTKFormat.CountTimeSteps
    # into the field time_stamp_evaluation. You can also define your own time
    # stamp. For this, it is useful to know that each particle definition in the
    # Python script will yield a particle class with the same name in the
    # subnamespace globaldata. Every particle class in return has a class attribute
    #
    #            getSpecies()
    #
    # which returns an object that holds global information about this particle
    # type.
    ###############################################################################
    particle_plotter = peano4.toolbox.particles.PlotParticlesInVTKFormat(
        "particles",
        particle_set,
        "globaldata::{}::getSpecies().getMinTimeStamp()".format(particle.name),
    )
    particle_plotter.add_attribute_to_plot(particle.part_id, 1)
    particle_plotter.add_attribute_to_plot(particle.velocity, dimensions)
    particle_plotter.add_attribute_to_plot(particle.acceleration, dimensions)
    particle_plotter.add_attribute_to_plot(particle.mass, 1)
    particle_plotter.add_attribute_to_plot(particle.density, 1)
    particle_plotter.add_attribute_to_plot(particle.pressure, 1)
    particle_plotter.add_attribute_to_plot(particle.u, 1)
    particle_plotter.add_attribute_to_plot(particle.smoothL, 1)
    if args.mode == "asserts" or args.mode == "debug":
        particle_plotter.add_attribute_to_plot(particle.density_neighbourcount, 1)
        particle_plotter.add_attribute_to_plot(particle.force_neighbourcount, 1)
    project.algorithm_step_plot.add_action_set(particle_plotter)

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

    print("-----------------------------------------------------------------------")
    print("SPH parameters summary:")
    print(f"Effective dimensions of the Simulation: {HYDRO_DIMENSIONS}")
    print(f"Global time-step size: {GLOBAL_TIME_STEP_SIZE}")
    print(f"Number of particles (1D): {HYDRO_PART_NUMBER}")
    print(f"Adiabatic index: {GAMMA}")
    print(f"Smoothing length free factor: {particle.eta_factor}")
    print("-----------------------------------------------------------------------")

    # Export Constants.h file ------------------------------------------------------
    peano4_project.constants.export_const_with_type(
        "HYDRO_DIMENSIONS", str(HYDRO_DIMENSIONS), "double"
    )
    peano4_project.constants.export_const_with_type(
        "HYDRO_PART_NUMBER", str(HYDRO_PART_NUMBER), "double"
    )
    peano4_project.constants.export_const_with_type(
        "GLOBAL_TIME_STEP_SIZE", str(GLOBAL_TIME_STEP_SIZE), "double"
    )
    peano4_project.constants.export_const_with_type("GAMMA", GAMMA, "double")

    # Export symbols for compilation (e.g. for SPH kernel) -----------------------
    peano4_project.output.makefile.add_CXX_flag(
        "-D" + "HYDRO_DIMENSION=" + str(HYDRO_DIMENSIONS)
    )
    peano4_project.output.makefile.add_CXX_flag("-D" + GAMMA_HYDRO_SYMBOL)
    peano4_project.output.makefile.add_CXX_flag("-D" + KERNEL_SYMBOL)

    ###############################################################################
    # Cleanup and compilation
    ###############################################################################

    # If we want to start a project from scratch, we may want to remove some of the generated code
    if args.clean_temporary_files:
        dirs = ["vertexdata/", "repositories/", "globaldata/", "observers/"]
        for dir in dirs:
            shutil.rmtree(dir)

    # It is safe (and recommended) to overwrite any existing main file, since this
    # is generated automatically by the Swift2 graph compiler.
    peano4_project.generate(
        overwrite=peano4.output.Overwrite.Always, throw_away_data_after_generation=True
    )
    peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)

    if args.clean_temporary_files:
        # remove output files from previous runs
        output_files = [
            f
            for f in os.listdir(".")
            if f.endswith(".peano-patch-file")
            or f.endswith(".vtu")
            or f.endswith(".pvd")
        ]
        for f in output_files:
            os.remove(f)
