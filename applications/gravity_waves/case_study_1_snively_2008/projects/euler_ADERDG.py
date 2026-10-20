import argparse
import sys
import os

import exahype2

from atmosphereE import Atmosphere

import numpy as np


stratified_atmosphere = Atmosphere()

# size = [1_200e3, 180e3, 1_200e3]
# size = [10e3, 10e3, 1]
size = [1200e3, 220e3, 1]
end_time = 10
order = 2
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "viscosity": 1}
use_limiting = False


# restart_from = '/work_fast/go78vuz/EXAHYPE/mario_new_limiting/applications/gravity_waves/restart_3_1BC/patch/ADERDGSolver.vtkhdf'
# restart_step = -61
# restart_time = 4000

file = '/work_fast/go78vuz/EXAHYPE/GW/applications/gravity_waves/AMR/patch/ADERDGSolver.vtkhdf'
# restart_from = '/work_fast/go78vuz/EXAHYPE/mario_new_limiting/applications/gravity_waves/restart_26_08/patch/ADERDGSolver.vtkhdf'
restart_from = file
restart_step = 30
restart_time = 3000

# A run of a build without HDF5 support plots VTU instead, and its .pvd
# collection restarts just as well: the step is the entry in that collection,
# and restart_time the time stamp the sampler logs for it.
# restart_from = 'Thunderstorm_Euler_lim_AMR2.output/patch/ADERDGSolver.pvd'
# restart_step = -72
# restart_time = 3000

restart_from = None

if restart_from is None:
    initial_conditions = stratified_atmosphere.initial_conditions()
    restart_time = 0

else:
    initial_conditions = stratified_atmosphere.initial_conditions_from_file(
        restart_from, restart_step)

if use_limiting:
    name = "Thunderstorm_Euler_lim"
else:
    name = "Thunderstorm_Euler"


project = exahype2.Project(
    namespace=["applications", "gravity_waves", "thunderstorm"],
    project_name=name,
    executable=name,
    dimensions=2,
    offset=[0, 0, 0],
    size=size,
    periodic_bc=[False, False, False],
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=4,
    max_grid_levels=5,
)

solver = exahype2.dg.ader.Solver(
    name                 = "ADERDGSolver",
    order                = order,
    time_stepping = exahype2.TimeStepping.Adaptive(0.5),
    unknowns             = unknowns,
    auxiliary_variables = aux_var
    # auxiliary_variables  = {"grad": 8, "height": 1, "backgroundstate": 2},
)


solver.set_pde_terms(
    initial_condition         = initial_conditions,
    boundary_conditions        = stratified_atmosphere.boundary_conditions(),
    flux                       = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver=stratified_atmosphere.riemann_solver(),
    diffusive_flux=False,
    source=stratified_atmosphere.source_with_TS(
        2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
        2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
        600e3,                 # x_0
        12e3,                  # z_0
        600,                   # t_0
        0,                  # F_0/rho
        30e3,                  # sigma_x
        3e3,                   # sigma_z
        2000,                  # sigma_t
    time_offset=restart_time),
    # source = stratified_atmosphere.source(),
    refinement_criterion = stratified_atmosphere.refinement_criterion()
)

solver.add_includes(
    "tarch/reader/CSVReader.h",
    "tarch/reader/VTKSampler.h",
    "iostream"
)

solver.add_declarations(Atmosphere.CSV_CONSTANTS)
solver.add_declarations(Atmosphere.VISCOSITY_MODEL)

# solver.add_kernel_optimisations(
#     polynomials = Polynomials.Gauss_Lobatto
# )


if use_limiting:
    # The robust fallback scheme a troubled cell is recomputed with, plus the two
    # tests that decide whether a cell is troubled. Both tests live in Limiter.cpp.
    solver.add_subcell_limiter(
        limiter_solver                   = exahype2.solvers.fv.godunov.subcell_scheme(
            riemann_solver=stratified_atmosphere.fv_riemann_solver()
        ),
        number_of_dmp_observables        = 0,      # relative density pertubation
        dmp_relaxation_parameter         = 0.001,   # absolute floor on the widening
        dmp_differences_scaling          = 0.001,   # widening relative to the local spread
        dmp_observables                  = stratified_atmosphere.map_observerables(),
        physical_admissibility_criterion = stratified_atmosphere.physical_admissibility_criterion(),
    )


# if use_limiting:
#     # The robust fallback scheme a troubled cell is recomputed with, plus the two
#     # tests that decide whether a cell is troubled. Both tests live in Limiter.cpp.
#     solver.add_subcell_limiter(
#         limiter_solver                   = exahype2.solvers.fv.godunov.subcell_scheme(
#             riemann_solver=stratified_atmosphere.fv_riemann_solver()
#         ),
#         number_of_dmp_observables        = 2,      # relative density pertubation
#         dmp_relaxation_parameter         = 0.001,   # absolute floor on the widening
#         dmp_differences_scaling          = 0.001,   # widening relative to the local spread
#         dmp_observables                  = stratified_atmosphere.map_observerables(),
#         physical_admissibility_criterion = stratified_atmosphere.physical_admissibility_criterion(),
#     )


# exahype2.solvers.aderdg.ErrorMeasurement(
#     solver,
#     error_measurement_implementation="""
#         solution[Shortcuts::E] = Q[Shortcuts::backgroundstate + 2];


#     """,
#     output_file_name="Error_E",
#     measured_unknowns=["E"],   
# )


# exahype2.solvers.aderdg.ErrorMeasurement(
#     solver,
#     error_measurement_implementation="""
#         solution[0] = Q[Shortcuts::backgroundstate + 0];
#         solution[1] = 0.0;
#         solution[2] = 0.0;
#         solution[3] = Q[Shortcuts::backgroundstate + 2];
#     """,
#     output_file_name="Error_Equilibrium",
# )


project.add_solver(solver)


project.generate_and_build()
