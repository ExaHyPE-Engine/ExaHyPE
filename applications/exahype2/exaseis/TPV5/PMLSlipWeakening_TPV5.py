import os, sys, peano4, exahype2

from exahype2.solvers.aderdg.ADERDG import Polynomials

sys.path.insert(0, os.path.abspath("../PML"))
import PMLElastic
import PMLSlipWeakening

project = exahype2.Project(["exahype2", "elastic"], "pml", executable="TPV5")


unknowns = {"v": 3, "sigma": 6, "u": 3, "pml": 27}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "dmp_pml": 3, "jacobian": 1,
    "metric_derivative": 9,
    "curve_grid": 3,
}

offset  = [-2.,  0., -2.]
size    = [40.5, 40.5, 40.5]
end_time = 7.0  # 10.0
order = 5
min_level = 2
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

precision = "fp64"

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    initial_conditions    = PMLSlipWeakening.initial(),
    boundary_conditions   = PMLElastic.boundary(),
    eigenvalues           = PMLElastic.eigenvalue(),
    flux                  = PMLElastic.flux(),
    ncp                   = PMLElastic.ncp(),
    material_parameters   = PMLElastic.multiplyMaterialParameterMatrix(),
    source_term           = PMLElastic.algebraic_source()
)

theSolver.add_kernel_optimisations(
    is_linear=True,
    polynomials=Polynomials.Gauss_Lobatto,
    initialise_patches=True,
    riemann_solver_implementation=PMLSlipWeakening.riemann_solver(),
)

theSolver._finish_time_step_implementation += PMLSlipWeakening.finish_time_step_implementation()
theSolver._abstract_solver_user_declarations += PMLSlipWeakening.abstractDeclarations()
theSolver._constructor_implementation += PMLSlipWeakening.abstractConstructor()
theSolver.add_user_solver_includes(PMLSlipWeakening.userIncludes())

project.add_solver(theSolver)

project.add_action_set_to_timestepping(
    exahype2.tracer.NewDGTracer(
    coordinates=[
          # off-fault at surface
          [17.0, 0.0,  8.0],
          [17.0, 0.0, 32.0],
          [23.0, 0.0,  8.0],
          [23.0, 0.0, 32.0],
          # off-fault at hypocenter depth
          [17.0, 7.5,  8.0],
          [17.0, 7.5, 32.0],
          [23.0, 7.5,  8.0],
          [23.0, 7.5, 32.0],
          # on-fault at surface
          [20.0, 0.0,  8.0],
          [20.0, 0.0, 12.5],
          [20.0, 0.0, 15.5],
          [20.0, 0.0, 20.0], #surface above fault hypocenter
          [20.0, 0.0, 24.5],
          [20.0, 0.0, 27.5],
          [20.0, 0.0, 32.0],

          [20.0, 7.5, 15.50], #above fault hypocenter
          [20.0, 7.5, 24.50], #below fault hypocenter

          #on-fault at hypocenter depth
          [20.0, 7.5,  8.0],
          [20.0, 7.5, 12.5], #center of left patch
          [20.0, 7.5, 15.5],
          [20.0, 7.5, 20.0], #on fault hypocenter
          [20.0, 7.5, 24.5],
          [20.0, 7.5, 27.5], #center of right patch
          [20.0, 7.5, 32.0],
    ],
    solver=theSolver,
    filename="TPV5_level_"+str(min_level)+"-order-"+str(order)+"-precision-"+str(precision),
    data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
    output_precision=10, clear_database_after_flush=False
))

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False, False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.output.makefile.set_CXX_compiler("mpicxx")
peano4_project.output.makefile.add_CXX_flag("-fopenmp -DUSE_ASAGI")
peano4_project.output.makefile.add_library(
    "-fopenmp -llua -leasi -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi -DUSE_ASAGI"
)
peano4_project.build(make_clean_first=True)
