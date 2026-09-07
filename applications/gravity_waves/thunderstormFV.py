import argparse
import sys
import os

import exahype2

from atmosphereE import Atmosphere
from exahype2.solvers.aderdg import Polynomials


stratified_atmosphere = Atmosphere()

# size = [1_200e3, 180e3, 1_200e3]
# size = [10e3, 10e3, 1]
size = [1200e3, 220e3, 1]
end_time = 10
order = 5
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "viscosity": 1}
use_limiting = True


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
    min_grid_levels=4,
    max_grid_levels=4,
)

solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name                 = "FVSolver",
    patch_size           = 12,
    time_step_relaxation = 0.5,
    unknowns             = unknowns,
    auxiliary_variables = aux_var
    # auxiliary_variables  = {"grad": 8, "height": 1, "backgroundstate": 2},
)


solver.set_implementation(
    initial_conditions         = initial_conditions,
    boundary_conditions        = stratified_atmosphere.boundary_conditions_1_3(),
    flux                       = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver = stratified_atmosphere.fv_riemann_solver(),
    source_term=stratified_atmosphere.source_with_TS(time_offset=restart_time),

)
solver.add_user_solver_includes('#include "tarch/reader/CSVReader.h"\n')
solver.add_user_solver_includes('#include "tarch/reader/VTKHDFSampler.h"\n')

solver.add_solver_constants(Atmosphere.CSV_CONSTANTS)
solver.add_solver_constants(Atmosphere.VISCOSITY_MODEL)

project.add_solver(solver)

project.generate_and_build()