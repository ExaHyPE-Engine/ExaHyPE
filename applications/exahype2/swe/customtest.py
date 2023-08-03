import os
import sys

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2
from scenarios.radial_bathymetry_dam_break import Radial_Bathymetry_Dam_Break

dimensions            = 2 # this is fixed to 2 in pde.py
width                 = [10, 10]
offset                = [0, 0] # this is fixed to [0, 0]
end_time              = 1
time_step_relaxation  = 0.5 # taken from tutorial
plot_interval         = 0.005 # taken from tutorial
patch_size            = 4 # 32
maximum_cell_size     = 0.1
amr_levels            = 3 
auxiliary_variables   = 1 # bathymetry, this is fixed to 1 in pde.py
amr_tolerance         = 1e-3

my_pde = Radial_Bathymetry_Dam_Break(width=width, refinement_tolerance=amr_tolerance)
my_pde.substitute_expression(my_pde.g, 9.81)

custom_riemann_solver = """
    double grav = 9.81;
    double smax = 0.0;
    for(int i = 0; i < 3; i++)
    {
        smax = std::max(smax, std::max(std::abs(LL[i]), std::abs(LR[i])));
    }
    double bm = std::max(QL[3], QR[3]);
    double hRoe = 0.5 * (QL[0] + QR[0]);
    double DEta = std::max(QR[0]+QR[3] - bm, 0.0) - std::max(QL[0]+QL[3] - bm, 0.0);

    double flux[3] = {0.0};
    flux[0] = 0.5 * (FL[0] + FR[0]) - 0.5 * smax * DEta;
    flux[1] = 0.5 * (FL[1] + FR[1]) - 0.5 * smax * (QR[1] - QL[1]);
    flux[2] = 0.5 * (FL[2] + FR[2]) - 0.5 * smax * (QR[2] - QL[2]);

    switch(normal)
    {
        case 0:
            QSFlux[0] = flux[0];
            QSFlux[1] = flux[1] - 0.5*grav*hRoe*DEta;
            QSFlux[2] = flux[2];
            break;
        
        case 2:
            QSFlux[0] = flux[0];
            QSFlux[1] = flux[1];
            QSFlux[2] = flux[2] - 0.5*grav*hRoe*DEta;
            break;
    }
    return smax;
"""

custom_flux = """
switch(normal)
{
    case 0:
        F[0] = Q[1];
        F[1] = Q[1] * Q[1] / Q[0];
        F[2] = Q[1] * Q[2] / Q[0];
        break;
    case 1:
        F[0] = Q[2];
        F[1] = Q[1] * Q[2] / Q[0];
        F[2] = Q[2] * Q[2] / Q[0];
        break;
}
return;
"""

solvers = [exahype2.solvers.fv.riemann.UserDefinedRiemann, exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep]
solver = 0
my_solver = solvers[solver](
    name                      = "%s" % "CUSTOMTEST" if solver == 0 else "RUSANOV",
    patch_size                = 4,
    unknowns                  = 3,
    auxiliary_variables       = 1,
    max_volume_h              = 0.1, # must be at least as fine as bathymetry file if netCDF is used
    min_volume_h              = 0.1 * 3.0**(0),
    time_step_relaxation      = 0.005
)

if solver == 0:
    my_solver.set_implementation(
    initial_conditions    = my_pde.implementation_of_initial_conditions(),
    boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
    flux                  = custom_flux,
    eigenvalues           = my_pde.implementation_of_eigenvalues(),
    refinement_criterion  = my_pde.implementation_of_refinement_criterion(),
    riemann_solver        = custom_riemann_solver
    )
else:
    my_solver.set_implementation(
        initial_conditions    = my_pde.implementation_of_initial_conditions(),
        boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
        flux                  = my_pde.implementation_of_flux(),
        eigenvalues           = my_pde.implementation_of_max_eigenvalue(),
        ncp                   = my_pde.implementation_of_ncp(),
        refinement_criterion  = my_pde.implementation_of_refinement_criterion(),
        source_term           = my_pde.implementation_of_sources()
)

my_project = exahype2.Project( ["applications", "exahype2", "swe"], "SWEApplication", ".", executable="CUSTOMTEST")
my_project.add_solver( my_solver )
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release)

my_project.set_global_simulation_parameters(
  dimensions            = 2,
  size                  = [10, 10],
  offset                = [0, 0],
  min_end_time          = 0.1,
  max_end_time          = 0.1,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005,
  periodic_BC           = [False, False, False]
)

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.generate()
peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)