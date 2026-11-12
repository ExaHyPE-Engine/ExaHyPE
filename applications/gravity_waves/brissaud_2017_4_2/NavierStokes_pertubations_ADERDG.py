import argparse
import sys
import os
import numpy as np

import exahype2
from atmosphere_navier_stokes_pertubations_isothermal import Atmosphere
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../functions")))
from run_config import embed_run_config




stratified_atmosphere = Atmosphere()

domain_height = 400e3

size = [domain_height, domain_height, 1]
# size = [1200e3, 600e3, 1]
end_time = 500
# order = 2
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "gradients": 1}   # dTdz, heatflux: diagnostics written by heat_flux_postprocess


parser = argparse.ArgumentParser()
parser.add_argument("--order", type=int, default=5)
args = parser.parse_args()
order = args.order

name = f"Single_Wave_order{order}"

project = exahype2.Project(
    namespace=["applications", "gravity_waves", "thunderstorm"],
    project_name=name,
    executable=name,
    dimensions=2,
    offset=[0, 0, 0],
    size=size,
    periodic_bc=[True, False, False],
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=5,
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
    initial_condition          = stratified_atmosphere.initial_conditions(),
    boundary_conditions        = stratified_atmosphere.boundary_conditions(),
    diffusive_flux             = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver             = stratified_atmosphere.riemann_solver(),
    flux                       = False,
    source                     = stratified_atmosphere.source()

)
solver.add_includes(
    "iostream",
    "cmath",
    "algorithm",
    "cstdlib",
    "string",
    "tarch/NonCriticalAssertions.h"
)

ISOTHERMAL_MODEL = f"""
    static constexpr double scale_height    = 29e3;
    static constexpr double rho_0           = 0.4;
    static constexpr double p_0             = 113760.28571428574;
    static constexpr double E_0             = 284400.71428571444;
    static constexpr double cv              = 0.7177540243571513;
    static constexpr double cp              = 1.0048556341000117;
    static constexpr double gamma           = 1.4;
    static constexpr double mu              = 0.75e-4;
    static constexpr double gravity         = 9.831;
    static constexpr double domain_height   = {domain_height};
"""

SOURCE_INFORMATION = """
    static constexpr double PT_div_4         = 1.5;
    static constexpr double t_0         = 6;

    """

solver.add_declarations(SOURCE_INFORMATION)
solver.add_declarations(ISOTHERMAL_MODEL)

#TODO: Remove and add other constants

# the executable writes these into <outputPath>/run_config/ when a run starts
here = os.path.dirname(os.path.abspath(__file__))
embed_run_config(
    solver,
    name,
    source_files=[os.path.abspath(__file__), os.path.join(here, "atmosphere_navier_stokes_pertubations_isothermal.py")],
    settings={
        "project": name,
        "size": size,
        "end_time": end_time,
        "order": order,
        "unknowns": unknowns,
        "auxiliary_variables": aux_var,
    },
)



#add tracer 
probe_particles = project.add_tracer(name="Probe", particle_attributes=6)

probe_particles.insert_by_coordinates(
    [
        [25e3, 100.0e3],
        [25e3, 150.0e3],
        [25e3, 180.0e3],
        [25e3, 200.0e3],
        [25e3, 230.0e3],
        [25e3, 280.0e3],
    ]
)

probe_particles.dump_into_database(
    filename="Probes",
    time_delta_between_two_snapshots=0.05,
    output_precision=17,
)
solver.trace(
    probe_particles,
    exahype2.particles.Forces(
        interpolation_scheme=2,   # piecewise linear
        integration_scheme=0,     # static, the probes don't move
        force_method=0,
        fields_to_interpolate="0,1,2,3,4,5",   # first 6 solver values -> data(0..5); rho=0, j=2, background=5
    ),
    initialisation=False,
)


project.add_solver(solver)



project.generate_and_build()
