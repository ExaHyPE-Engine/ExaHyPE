import argparse
import sys
import os
import numpy as np

import exahype2

from atmosphereE_pertubations import Atmosphere
from exahype2.fv.scheme import Limiter          # add near the top
from f_wave import f_wave_riemannsolver, max_eigenavalue
from modal_filter import modal_filter_kernel


from run_config import embed_run_config



stratified_atmosphere = Atmosphere()


size = [1200e3, 220e3, 1]
# size = [1200e3, 600e3, 1]
end_time = 6000
order = 5
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "viscosity": 1}
use_limiting = True
use_modal_filter = False

file = '/work_fast/go78vuz/EXAHYPE/GW/applications/gravity_waves/AMR/patch/ADERDGSolver.vtkhdf'
# restart_from = '/work_fast/go78vuz/EXAHYPE/mario_new_limiting/applications/gravity_waves/restart_26_08/patch/ADERDGSolver.vtkhdf'
restart_from = file
restart_step = 30
restart_time = 3000


restart_from = None

if restart_from is None:
    initial_conditions = stratified_atmosphere.initial_conditions()
    restart_time = 0

else:
    initial_conditions = stratified_atmosphere.initial_conditions_from_file(
        restart_from, restart_step)



name = "Thunderstorm_Euler_pertub"


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
    boundary_conditions        = stratified_atmosphere.boundary_conditions_pertb_1_3(),
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
        1e-7,                  # F_0/rho
        30e3,                  # sigma_x
        3e3,                   # sigma_z
        2000,                  # sigma_t
    time_offset=restart_time,
    physical_admissibility = stratified_atmosphere.physical_admissibility_criterion(),
    dmp_observables = stratified_atmosphere.map_observerables()

)
solver.add_includes(
    "tarch/reader/CSVReader.h",
    "tarch/reader/VTKSampler.h",
    "iostream"
)

solver.add_declarations(Atmosphere.CSV_CONSTANTS)

if use_modal_filter:
    solver.postprocess_updated_patch = modal_filter_kernel(
        order              = order,
        number_of_unknowns = sum(unknowns.values()),
        number_of_data     = sum(unknowns.values()) + sum(aux_var.values()),
        damping_rate       = 0.05,   # 1/s, e-folding rate of the highest mode
        filter_order       = 4,
        cutoff_mode        = None,   # N//2: lower modes untouched
        min_height         = None,   # e.g. 80e3 to filter only above 80 km
    )
    solver.add_includes("cmath")

if use_limiting:
    solver.add_subcell_limiter(
        exahype2.fv.MusclHancock(limiter=Limiter.mclim),
        number_of_dmp_observables = 1,
        dmp_relaxation_parameter  = 0.0,
        dmp_differences_scaling   = 1e-3,
    )


# the executable writes these into <outputPath>/run_config/ when a run starts
here = os.path.dirname(os.path.abspath(__file__))
embed_run_config(
    solver,
    name,
    source_files=[os.path.abspath(__file__), os.path.join(here, "atmosphereE_pertubations.py")],
    settings={
        "project": name,
        "size": size,
        "end_time": end_time,
        "order": order,
        "unknowns": unknowns,
        "auxiliary_variables": aux_var,
        "restart_from": restart_from,
        "restart_step": restart_step,
        "restart_time": restart_time,
    },
)

project.add_solver(solver)



project.generate_and_build()
