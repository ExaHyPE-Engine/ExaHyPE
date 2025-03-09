import os, peano4, exahype2

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.GlobalAdaptiveTimeStep import (
    GlobalAdaptiveTimeStep,
)
from exahype2.solvers.aderdg.ADERDG import Polynomials

import CurvilinearElastic

project = exahype2.Project(["exahype2", "elastic"], "curvi", executable="LOH1")

unknowns = {"v": 3, "sigma": 6}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "jacobian": 1, "metric_derivative": 9,
    "curve_grid": 3,
}

#offset = [-2.28662, 0.0, -2.28662]
#size = [16.3333, 16.3333, 16.3333]

end_time = 1.0  # 10.0
min_level = 3
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)
order = 3

precision = "float"

use_tracers = True

theSolver = GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.9,
    point_source=1
)

theSolver._init_point_sources_implementation = CurvilinearElastic.init_point_source()
theSolver._point_sources_implementation      = CurvilinearElastic.point_source()

theSolver.add_kernel_optimisations(
    is_linear=True,
    polynomials=Polynomials.Gauss_Lobatto,
    initialise_patches=True,
    riemann_solver_implementation = CurvilinearElastic.riemann_solver(),
    precision = precision

)

theSolver.set_implementation(
  initial_conditions    = CurvilinearElastic.initial(),
  boundary_conditions   = CurvilinearElastic.boundary(),
  eigenvalues           = CurvilinearElastic.eigenvalue(),
  refinement_criterion  = CurvilinearElastic.refinement_criterion(),
  flux                  = CurvilinearElastic.flux(),
  ncp                   = CurvilinearElastic.ncp(),
  material_parameters   = CurvilinearElastic.multiplyMaterialParameterMatrix()
)

theSolver._abstract_solver_user_declarations += CurvilinearElastic.abstractDeclarations()
theSolver._constructor_implementation += CurvilinearElastic.abstractConstructor()
theSolver.add_user_solver_includes(CurvilinearElastic.userIncludes())

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
        filename="tracer-Curvilinear-level-"+str(min_level)+"-order-"+str(order)+"-precision-"+str(precision),
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

project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project("False")

peano4_project.output.makefile.set_CXX_compiler("mpicxx")
peano4_project.output.makefile.add_CXX_flag("-fopenmp -DUSE_ASAGI")
peano4_project.output.makefile.add_library(
    "-fopenmp -lcurvi -leasi -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi -DUSE_ASAGI"
)

peano4_project.build(make_clean_first=True)
