import peano4, exahype2

project = exahype2.Project(["exahype2", "elastic"], "pml", executable="TPV5_2D")

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
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)


theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
    material_parameters=exahype2.solvers.PDETerms.User_Defined_Implementation,
    # point_source=1,
)

theSolver.add_kernel_optimisations(
    is_linear=True,
    # polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto,
    riemann_solver_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation
)

project.add_solver(theSolver)
project.set_output_path("solutions")

tracer_particles = project.add_tracer(name="Tracer", attribute_count=17)

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
    peano4.toolbox.particles.api.UpdateParallelState(particle_set=tracer_particles)
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
        filename="tracer-TPV5-"+str(min_level)+"-o-"+str(order),
        data_delta_between_two_snapsots=1e16, time_delta_between_two_snapsots=0.01,
        output_precision=10, clear_database_after_flush=False
    )
)

project.set_global_simulation_parameters(
    dimensions=2,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.1,
    periodic_BC=[False, False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=32)
