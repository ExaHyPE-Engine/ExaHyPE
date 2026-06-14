import argparse

import exahype2

unknowns = {"v": 2, "sigma": 3, "u": 2}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "jacobian": 1, "metric_derivative": 4,
    "curve_grid": 2,
}

offset  = [0.0, 0.0]
size    = [40., 40.]
end_time = 5.0 # 10.0
order = 5
min_level = 3
max_depth = 2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=end_time)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=order)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=min_level)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=min_level + max_depth)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
order = args.degrees_of_freedom
end_time = args.end_time

project = exahype2.Project(
    ["applications", "exahype2", "exaseis"],
    "pml",
    dimensions=2,
    offset=offset,
    size=size,
    periodic_BC=[False, False],
    executable="TPV5_2D",
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The TPV5 2D ExaSeis application is two-dimensional.")


theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    time_step_relaxation=0.9,
)

theSolver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
    material_parameters=exahype2.solvers.PDETerms.User_Defined_Implementation,
    riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

theSolver.add_kernel_optimisations(
    is_linear=True,
    # polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto,
)

project.add_solver(theSolver)

tracer_particles = project.add_tracer(name="Tracer", particle_attributes=17)

project.add_action_set_to_initialisation(
    exahype2.tracer.InsertParticlesByCoordinates(
        particle_set=tracer_particles,
        coordinates=[
            [19.2593, 0],   #on fault at surface
            [19.2593, 2.5],
            [19.2593, 4.5],
            [19.2593, 6.5],
            [19.2593, 7.5], #on fault hypocenter 
            [19.2593, 8.5],
            [19.2593, 10.5],
            [19.2593, 12.5],
            [19.2593, 14.5]
        ],
    )
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DiscontinuousGalerkinTracing(
        tracer_particles, theSolver,
        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
    )
)

project.add_action_set_to_timestepping(
    exahype2.tracer.DumpTracerIntoDatabase(
        particle_set=tracer_particles, solver=theSolver,
        filename="tracer-TPV5-"+str(args.min_grid_levels)+"-o-"+str(order),
        data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
        output_precision=10, clear_database_after_flush=False
    )
)

peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True)
