import argparse
import sys
import os
import numpy as np

import exahype2
from atmosphere_navier_stokes_pertubations_isothermal import Atmosphere
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../functions")))
from run_config import embed_run_config




stratified_atmosphere = Atmosphere()


size = [50e3, 400e3, 1]
# size = [1200e3, 600e3, 1]
end_time = 6000
order = 5
# order = 2
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "gradients": 1}   # dTdz, heatflux: diagnostics written by heat_flux_postprocess



name = "Single_Wave"


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
    initial_condition          = stratified_atmosphere.initial_conditions(),
    boundary_conditions        = stratified_atmosphere.boundary_conditions(),
    diffusive_flux             = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver             = stratified_atmosphere.riemann_solver(),
    flux                       = False,
    source                     = stratified_atmosphere.source()

)
solver.add_includes(
    "tarch/reader/CSVReader.h",
    "tarch/reader/VTKSampler.h",
    "iostream",
    "cmath",
    "algorithm",
    "cstdlib"
)

ISOTHERMAL_MODEL = """
    static constexpr double scale_height    = 29e3;
    static constexpr double rho_0           = 0.4;
    static constexpr double p_0             = 113760.28571428574;
    static constexpr double E_0             = 284400.71428571444;
    static constexpr double cv              = 0.7177540243571513;
    static constexpr double cp              = 1.0048556341000117;
    static constexpr double gamma           = 1.4;
    static constexpr double mu              = 0.75e-4;
    static constexpr double gravity         = 9.831;
    static constexpr double domain_height   = 280e3;
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



# if args.use_probes:
probe_particles = project.add_tracer(name="Probe", particle_attributes=1)

project.add_action_set_to_initialisation(
    exahype2.particles.InsertTracersByCoordinates(
        particle_set=probe_particles,
        coordinates=[
            [25e3, 100.0e3],
            [25e3, 248.9e3],
        ],
    )
)

solver.trace(
    probe_particles,
    kernel="""[](const auto& marker, const int order, const auto* nodes,
                 const int nData, const auto* Q, auto& particle) {
        const auto x   = particle.getX();
        const auto rho = ::exahype2::dg::evaluatePolynomial(marker, order, nodes, nData, Q, x, 0);  // rho
        const auto jy  = ::exahype2::dg::evaluatePolynomial(marker, order, nodes, nData, Q, x, 2);  // j + 1
        const auto bg  = ::exahype2::dg::evaluatePolynomial(marker, order, nodes, nData, Q, x, 5);  // backgroundstate + 0
        particle.setData(0, jy / (rho + bg));
    }""",
    
        initialisation=True,
)

project.add_action_set_to_timestepping(
    exahype2.particles.DumpTracerIntoDatabase(
        particle_set=probe_particles,
        solver=solver,
        filename="Probes",
        time_delta_between_two_snapshots=0.05,
        output_precision=17,
    )
)


project.add_solver(solver)



project.generate_and_build()
