import argparse
import sys
import os
import numpy as np

import exahype2

from atmosphereE import Atmosphere
from exahype2.fv.scheme import Limiter          # add near the top

from f_wave import max_eigenavalue
stratified_atmosphere = Atmosphere()


# size = [1_200e3, 180e3, 1_200e3]
# size = [10e3, 10e3, 1]
size = [1200e3, 220e3, 1]
end_time = 10
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "viscosity": 1}
use_limiting = True
level = 4

# restart_from = '/work_fast/go78vuz/EXAHYPE/mario_new_limiting/applications/gravity_waves/restart_3_1BC/patch/ADERDGSolver.vtkhdf'
# restart_step = -61
# restart_time = 4000


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
    project_name='Thunderstorm_Euler_FV',
    executable='Thunderstorm_Euler_FV',
    dimensions=2,
    offset=[0, 0, 0],
    size=size,
    periodic_bc=[False, False, False],
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=level,
    max_grid_levels=level,
)



#Testing Wave propagation algorithm
solver = exahype2.fv.Solver(
    name                 = "FVSolver",
    # patch_size           = 5,
    patch_size           = 12,
    time_stepping        = exahype2.TimeStepping.Adaptive(0.5),
    scheme               = exahype2.fv.WavePropagation(limiter=Limiter.mclim),
    unknowns             = unknowns,
    auxiliary_variables = aux_var
)


solver.set_pde_terms(
    initial_condition         = initial_conditions,
    boundary_conditions        = stratified_atmosphere.boundary_conditions(),
    flux                       = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver = max_eigenavalue(),
    source =stratified_atmosphere.source_with_TS_no_gravity(
        0.011,   # omega 
        0.00011,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
        450e3,                 # x_0
        12e3,                  # z_0
        2000,                   # t_0
        0.33*0.011,                  # F_0
        30e3,                  # sigma_x
        4e3,                   # sigma_z
        600,                  # sigma_t
        time_offset=restart_time),

)




# solver = exahype2.fv.Solver(
#     name                 = "FVSolver",
#     patch_size           = 12,
#     time_stepping        = exahype2.TimeStepping.Adaptive(0.5),
#     scheme               = exahype2.fv.Godunov(),
#     unknowns             = unknowns,
#     auxiliary_variables = aux_var
# )


# solver.set_pde_terms(
#     initial_condition         = initial_conditions,
#     boundary_conditions        = stratified_atmosphere.boundary_conditions(),
#     flux                       = stratified_atmosphere.flux(),
#     max_eigenvalue             = stratified_atmosphere.eigenvalues(),
#     riemann_solver = f_wave_riemannsolver(),
#     source =stratified_atmosphere.source_with_TS(
#         2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
#         2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
#         600e3,                 # x_0
#         12e3,                  # z_0
#         600,                   # t_0
#         1e-7,                  # F_0/rho
#         30e3,                  # sigma_x
#         3e3,                   # sigma_z
#         2000,                  # sigma_t
#     time_offset=restart_time),
# )



# solver = exahype2.fv.Solver(   # test instead of gundunov FV
#     name                 = "FVSolver",
#     patch_size           = 12,
#     time_stepping        = exahype2.TimeStepping.Adaptive(0.5),
#     scheme               = exahype2.fv.MusclHancock(),
#     unknowns             = unknowns,
#     auxiliary_variables = aux_var
# )
 

# solver.set_pde_terms(
#     initial_condition         = initial_conditions,
#     boundary_conditions        = stratified_atmosphere.boundary_conditions(),
#     flux                       = stratified_atmosphere.flux(),
#     max_eigenvalue             = stratified_atmosphere.eigenvalues(),
#     riemann_solver = stratified_atmosphere.fv_riemann_solver(),
#     source =stratified_atmosphere.source_with_TS(
#         2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
#         2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
#         600e3,                 # x_0
#         12e3,                  # z_0
#         600,                   # t_0
#         1e-7,                  # F_0/rho
#         30e3,                  # sigma_x
#         3e3,                   # sigma_z
#         2000,                  # sigma_t
#     time_offset=restart_time),
#         limiter             = Limiter.mclim,               # add this
# )




# solver.set_implementation(
#     initial_conditions         = initial_conditions,
#     boundary_conditions        = stratified_atmosphere.boundary_conditions(),
#     flux                       = stratified_atmosphere.flux(),
#     max_eigenvalue             = stratified_atmosphere.eigenvalues(),
#     riemann_solver = f_wave_riemannsolver(),
#     source_term=stratified_atmosphere.source_with_TS_no_gravity(
#         2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
#         2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
#         600e3,                 # x_0
#         12e3,                  # z_0
#         600,                   # t_0
#         1e-7,                  # F_0/rho
#         30e3,                  # sigma_x
#         3e3,                   # sigma_z
#         2000,                  # sigma_t
#     time_offset=restart_time),
#         limiter             = Limiter.mclim,               # add this


# )

solver.add_includes(
    "tarch/reader/CSVReader.h",
    "iostream"
)
# solver.add_user_solver_includes('#include "tarch/reader/VTKHDFSampler.h"\n')

solver.add_declarations(Atmosphere.CSV_CONSTANTS)
solver.add_declarations(Atmosphere.VISCOSITY_MODEL)

R = 8.31446261815324  # ideal gas constant in J / (mol * K)
GRAVITY = 9.81  # gravitational acceleration in m/s^2



# Tets post processing 
PP = """
    //check where PP is written
"""

solver.postprocess_updated_patch = PP


project.output.makefile.add_compile_definition(f"""GRAVITY={GRAVITY}""")
project.output.makefile.add_compile_definition(f"""UNIVERSAL_GAS_CONSTANT={R}""")
project.output.makefile.add_compile_definition(
    f"""MOLAR_MASS_ATOMIC_OXYGEN=0.016"""
)  # in kg/mol
project.output.makefile.add_compile_definition(
    f"""MOLAR_MASS_MOLECULAR_OXYGEN=0.032"""
)  # in kg/mol
project.output.makefile.add_compile_definition(
    f"""MOLAR_MASS_NITROGEN=0.028"""
)  # in kg/mol


project.add_solver(solver)

project.generate_and_build()