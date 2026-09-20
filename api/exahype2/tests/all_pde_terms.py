# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2

project = exahype2.Project(
    namespace=["tests", "exahype2", "api"],
    project_name="AllPDETerms",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=0.01,
    max_end_time=0.01,
    min_grid_levels=2,
    max_grid_levels=2,
)

nothing = "\n"
keep = "return ::exahype2::RefinementCommand::Keep;"
one = "return 1.0;"
zero = "return 0.0;"

fv_solver = exahype2.fv.Solver(
    name="AllPDETermsFV",
    patch_size=4,
    unknowns=2,
    auxiliary_variables=1,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.MusclHancock(),
)
fv_solver.set_pde_terms(
    initial_condition=nothing,
    boundary_conditions=nothing,
    refinement_criterion=keep,
    max_eigenvalue=one,
    flux=nothing,
    ncp=nothing,
    source=nothing,
    riemann_solver=zero,
    limited_difference=zero,
    reconstruction=nothing,
    analytical_solution=nothing,
)
fv_solver.measure_error()
project.add_solver(fv_solver)

# The second finite-volume solver names its unknowns, stores them in single precision and
# computes in double, takes a PDE term the project defines and measures the error of its tracers.
godunov_solver = exahype2.fv.Solver(
    name="AllPDETermsGodunovFV",
    patch_size=4,
    unknowns={"rho": 1, "v": 2},
    auxiliary_variables={"material": 1},
    time_stepping=exahype2.TimeStepping.Fixed(0.1),
    scheme=exahype2.fv.Godunov(),
    precision=exahype2.Precision(storage="float", compute="double"),
)
godunov_solver.set_pde_terms(
    initial_condition=nothing,
    boundary_conditions=True,
    refinement_criterion=keep,
    max_eigenvalue=one,
    flux=nothing,
    source=nothing,
    diffusive_source=nothing,
    particle_analytical_solution=nothing,
)
project.add_solver(godunov_solver)

tracer_particles = project.add_tracer(
    name="AllPDETermsTracer",
    particle_attributes={"unknowns": {"value": 1}},
    solver=godunov_solver,
)
godunov_solver.trace(tracer_particles)
godunov_solver.measure_particle_error(tracer_particles)

nonlinear_solver = exahype2.dg.ader.Solver(
    name="AllPDETermsNonlinearADERDG",
    order=2,
    unknowns=2,
    auxiliary_variables=1,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
)
nonlinear_solver.set_pde_terms(
    initial_condition=nothing,
    boundary_conditions=nothing,
    refinement_criterion=keep,
    max_eigenvalue=one,
    diffusive_flux=nothing,
    ncp=nothing,
    source=nothing,
    diffusive_source=nothing,
    riemann_solver=nothing,
    subcell_riemann_solver=zero,
    physical_admissibility="return true;",
    dmp_observables=nothing,
    dmp_relaxation_parameter="return specifiedRelaxationParameter;",
    analytical_solution=nothing,
)
nonlinear_solver.add_subcell_limiter(
    exahype2.fv.Godunov(), number_of_dmp_observables=1, dmp_relaxation_parameter=1.0e-4
)
nonlinear_solver.measure_error()
project.add_solver(nonlinear_solver)

linear_solver = exahype2.dg.ader.Solver(
    name="AllPDETermsLinearADERDG",
    order=2,
    unknowns=2,
    auxiliary_variables=1,
    time_stepping=exahype2.TimeStepping.Fixed(0.1),
    basis=exahype2.dg.GaussLobatto,
    linear=True,
    point_sources=1,
)
linear_solver.set_pde_terms(
    initial_condition=nothing,
    boundary_conditions=nothing,
    flux=nothing,
    ncp=nothing,
    source=nothing,
    material_parameters=nothing,
    point_source=nothing,
    point_source_locations=nothing,
    riemann_solver=nothing,
)
project.add_solver(linear_solver)

rk_solver = exahype2.dg.rk.Solver(
    name="AllPDETermsRKDG",
    rk_order=2,
    dg_order=2,
    unknowns=2,
    auxiliary_variables=1,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
)
rk_solver.set_pde_terms(
    initial_condition=nothing,
    boundary_conditions=nothing,
    refinement_criterion=keep,
    max_eigenvalue=one,
    flux=nothing,
    ncp=nothing,
    source=nothing,
    riemann_solver=nothing,
    analytical_solution=nothing,
)
rk_solver.measure_error()
project.add_solver(rk_solver)

project.generate_and_build()
