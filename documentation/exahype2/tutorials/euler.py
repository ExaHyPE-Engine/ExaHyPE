###
### A sample Python file for this problem:
###
import peano4, exahype2

domain_size = [120.0, 120.0]
domain_offset = [-10.0, -60.0]

min_level = 5  # Increase for finer mesh

unknowns = {"p": 1, "v": 2, "E": 1}
auxiliary_variables = {}
max_h = 1.1 * min(domain_size) / (3.0**min_level)
min_h = max_h

my_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="EulerSolver",
    patch_size=22,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
)

my_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

exahype2_project = exahype2.Project(
    namespace=["tutorials", "exahype2", "euler"],
    directory=".",
    project_name="Airfoil",
    executable="Airfoil",
)

exahype2_project.add_solver(my_solver)
exahype2_project.set_output_path("solutions")

exahype2_project.set_global_simulation_parameters(
    dimensions=2,
    size=domain_size,
    offset=domain_offset,
    min_end_time=10.0,
    max_end_time=10.0,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.5,
    periodic_BC=[False, False],
)

exahype2_project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
exahype2_project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release)
peano4_project = exahype2_project.generate_Peano4_project()
peano4_project.output.makefile.add_cpp_file("EulerSolver.cpp")
peano4_project.generate()
peano4_project.build(make=True, make_clean_first=False, throw_away_data_after_build=False)
