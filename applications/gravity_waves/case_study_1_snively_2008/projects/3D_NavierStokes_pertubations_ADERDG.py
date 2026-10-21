import argparse
import sys
import os
import numpy as np

import exahype2
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../functions")))
from atmosphere_navier_stokes_pertubations import Atmosphere
# from exahype2.fv.scheme import Limiter          # add near the top


from run_config import embed_run_config




stratified_atmosphere = Atmosphere()


size = [1200e3, 1200e3, 220e3]
# size = [1200e3, 600e3, 1]
end_time = 6000
order = 5
# order = 2
unknowns = {"rho": 1, "j": 3, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "gradients": 3}

use_limiting = False


restart_from = None

if restart_from is None:
    initial_conditions = stratified_atmosphere.initial_conditions()
    restart_time = 0



name = "Thunderstorm_NS_pertub"
output_directory = "Executables"
os.makedirs(output_directory, exist_ok=True)


project = exahype2.Project(
    namespace=["applications", "gravity_waves", "thunderstorm"],
    project_name=name,
    executable=name,
    directory=output_directory,
    dimensions=3,
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
)


solver.set_pde_terms(
    initial_condition          = initial_conditions,
    boundary_conditions        = stratified_atmosphere.boundary_conditions_pertb_1_3(),
    diffusive_flux             = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver             = stratified_atmosphere.riemann_solver(),
    flux=False,
    source=stratified_atmosphere.source_with_TS_3D(
        2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
        2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
        600e3,                 # x_0
        12e3,                  # z_0
        600,                   # t_0
        1e-7,                  # F_0/rho
        30e3,                  # sigma_x
        3e3,                   # sigma_z
        2000,                  # sigma_t
    time_offset=restart_time),
    physical_admissibility = stratified_atmosphere.physical_admissibility_criterion_true(),
    #dmp_observables = stratified_atmosphere.map_observerables()

)
solver.add_includes(
    "tarch/reader/CSVReader.h",
    "tarch/reader/VTKSampler.h",
    "iostream",
    "cmath",
    "algorithm",
    "cstdlib"
)

solver.add_declarations(Atmosphere.CSV_CONSTANTS)
solver.add_declarations(Atmosphere.FLUID_PROPERTIES)



solver.add_declarations(f"""
  // current simulation time, read by isPhysicallyAdmissible
  static inline double limiterTime = {float(restart_time)};
""")
solver.on_start_time_step(f"""
  limiterTime = getMinTimeStamp() + {float(restart_time)};
""")


if use_limiting:
    solver.add_subcell_limiter(
        exahype2.fv.MusclHancock(limiter=Limiter.mclim),
        number_of_dmp_observables = 0,
        dmp_relaxation_parameter  = 0.0,
        dmp_differences_scaling   = 1e-3,
    )


# the executable writes these into <outputPath>/run_config/ when a run starts
here = os.path.dirname(os.path.abspath(__file__))
embed_run_config(
    solver,
    name,
    source_files=[os.path.abspath(__file__), os.path.join(here, "../../functions/atmosphere_navier_stokes_pertubations.py")],
    settings={
        "project": name,
        "size": size,
        "end_time": end_time,
        "order": order,
        "unknowns": unknowns,
        "auxiliary_variables": aux_var,
        "restart_from": restart_from,
        "restart_time": restart_time,
    },
)

project.add_solver(solver)



project.generate_and_build()
