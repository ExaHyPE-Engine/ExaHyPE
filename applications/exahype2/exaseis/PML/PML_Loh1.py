import os, peano4, exahype2

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.GlobalAdaptiveTimeStep import (
    GlobalAdaptiveTimeStep,
)
from exahype2.solvers.aderdg.ADERDG import Polynomials

import PMLElastic

unknowns = {"v": 3, "sigma": 6, "pml": 27}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "dmp_pml": 3, "jacobian": 1,
    "metric_derivative": 9,
    "curve_grid": 3,
}

# offset = [-2.28662, 0.0, -2.28662]
# size = [16.3333, 16.3333, 16.3333]
offset = [-2.025, 0.0, -2.025]
size = [12.15, 12.15, 12.15]

end_time = 10.0  # 10.0
order = 3
min_level = 3
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

precision = "fp64"
use_tracers = True

project = exahype2.Project(["exahype2", "elastic"], "pml", executable="LOH1_l_"+str(min_level)+"_o_"+str(order)+"_pr_"+precision)

theSolver = GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    point_source=1,
)

theSolver._init_point_sources_implementation = PMLElastic.init_point_source()
theSolver._point_sources_implementation      = PMLElastic.point_source()

theSolver.add_kernel_optimisations(
    is_linear=True,
    polynomials=Polynomials.Gauss_Lobatto,
    initialise_patches=True,
    riemann_solver_implementation = PMLElastic.riemann_solver(),
    precision = precision
)

theSolver.set_implementation(
initial_conditions    = PMLElastic.initial(),
boundary_conditions   = PMLElastic.boundary(),
eigenvalues           = PMLElastic.eigenvalue(),
refinement_criterion  = PMLElastic.refinement_criterion(),
flux                  = PMLElastic.flux(),
ncp                   = PMLElastic.ncp(),
material_parameters   = PMLElastic.multiplyMaterialParameterMatrix(),
source_term           = PMLElastic.algebraic_source()
)

theSolver._abstract_solver_user_declarations += PMLElastic.abstractDeclarations()
theSolver._constructor_implementation += PMLElastic.abstractConstructor()
theSolver.add_user_solver_includes(PMLElastic.userIncludes())

project.add_solver(theSolver)

if use_tracers:
    project.add_action_set_to_timestepping(
    exahype2.tracer.NewDGTracer(
        coordinates=[
        [0.000, 0., 0.693], [0.000, 0., 5.543], [0.000, 0., 10.392],
        [0.490, 0., 0.490], [3.919, 0., 3.919], [7.348, 0., 7.3480],
        [0.577, 0., 0.384], [4.612, 0., 3.075], [8.647, 0., 5.7640]
        ],
        solver=theSolver,
        filename="new_PML_level_"+str(min_level)+"-order-"+str(order)+"-precision-"+str(precision),
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
    time_in_between_plots=0.000,
    periodic_BC=[False, False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation("../../../../", peano4.output.CompileMode.Release)
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.output.makefile.set_CXX_compiler("mpicxx")
peano4_project.output.makefile.add_CXX_flag("-fopenmp -DUSE_ASAGI")
peano4_project.output.makefile.add_library(
    "-fopenmp -leasi -llua -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi -DUSE_ASAGI"
)
peano4_project.build(make_clean_first=True)
