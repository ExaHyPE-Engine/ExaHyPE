import os
import sys

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2
from scenarios.radial_bathymetry_dam_break import  Radial_Bathymetry_Dam_Break

dimensions            = 2 # this is fixed to 2 in pde.py
width                 = [10, 10]
offset                = [0, 0] # this is fixed to [0, 0]
end_time              = 2
time_step_relaxation  = 0.5 # taken from tutorial
plot_interval         = 0.005 # taken from tutorial
patch_size            = 4 # 32
maximum_cell_size     = 0.1
amr_levels            = 0
auxiliary_variables   = 1 # bathymetry, this is fixed to 1 in pde.py
amr_tolerance         = 1e-3

solver = 1

my_pde = Radial_Bathymetry_Dam_Break(width=width, refinement_tolerance=amr_tolerance)
my_pde.substitute_expression(my_pde.g, 9.81)

custom_riemann_solver = """
/*
    std::cout << "Entered solveRiemannProblem for xR = " << xR(0) << ", yR = " << xR(1) << " in direction " << normal << std::endl;
    std::cout << "Checking arrays for garbage values" << std::endl;
    for(int i = 0; i < 3; i++)
    {
        std::cout << "QR[" << i << "] = " << QR[i] << std::endl
        << "QL[" << i << "] = " << QL[i] << std::endl
        << "FR[" << i << "] = " << FR[i] << std::endl
        << "FL[" << i << "] = " << FL[i] << std::endl
        << "LR[" << i << "] = " << LR[i] << std::endl
        << "LL[" << i << "] = " << LL[i] << std::endl;
    }
*/
    
    const double g = 9.81;
    double smax = 0.0;
    {
        double maxAbsLambda = 0.0;
        for(int i = 0; i < 3; i++)
        {
            maxAbsLambda = std::max(std::abs(LL[i]), std::abs(LR[i]));
            smax = std::max(smax, maxAbsLambda);
        }
    }

    double bm = std::max(QL[3], QR[3]);
    double hRoe = 0.5 * (QL[0] + QR[0]);
    double DEta = std::max(QR[0]+QR[3] - bm, 0.0) - std::max(QL[0]+QL[3] - bm, 0.0);

    double flux[3] = {0.0};
    flux[0] = 0.5 * (FL[0] + FR[0]) - 0.5 * smax * DEta;
    flux[1] = 0.5 * (FL[1] + FR[1]) - 0.5 * smax * (QR[1] - QL[1]);
    flux[2] = 0.5 * (FL[2] + FR[2]) - 0.5 * smax * (QR[2] - QL[2]);

    APDQ[0] = flux[0];
    AMDQ[0] = flux[0];
    APDQ[1] = flux[1];
    AMDQ[1] = flux[1];
    APDQ[2] = flux[2];
    AMDQ[2] = flux[2];

    APDQ[normal + 1] -= 0.5 * g * hRoe * DEta;
    AMDQ[normal + 1] += 0.5 * g * hRoe * DEta;
/*
    std::cout << "smax " << smax << std::endl
    << "APDQ[normal+1] " << APDQ[normal+1] << std::endl
    << "AMDQ[normal+1] " << AMDQ[normal+1] << std::endl;
*/
    return smax;
"""

custom_fwave_solver = """
double g = 9.81;
double m_h_l = QL[0];
double m_h_r = QR[0];
double m_hu_l = QL[normal+1];
double m_hu_r = QR[normal+1];
double bat_l = QL[3];
double bat_r = QR[3];

double u[2]{m_hu_l / m_h_l, m_hu_r / m_h_r};

double lambda_roe[2]{0};
double h_roe = (m_h_l + m_h_r) / 2;
double u_roe = (u[0] * std::sqrt(m_h_l) + u[1] * std::sqrt(m_h_r)) / (std::sqrt(m_h_l) + std::sqrt(m_h_r));
lambda_roe[0] = u_roe - std::sqrt(g * h_roe);
lambda_roe[1] = u_roe + std::sqrt(g * h_roe);

double delta_x_psi = -g * (bat_r - bat_l) * (m_h_l + m_h_r) / 2;
double delta_f[2]{FR[0] - FL[0], FR[normal+1] - FL[normal+1] - delta_x_psi};

double determinante{1 / (lambda_roe[1] - lambda_roe[0])};
double deter_mul_del_f_1 = determinante * delta_f[1];
double deter_mul_del_f_0 = determinante * delta_f[0];
double alpha[]{lambda_roe[1] * deter_mul_del_f_0 - deter_mul_del_f_1,
            -(lambda_roe[0]) * deter_mul_del_f_0 + deter_mul_del_f_1};

double z_1[2]{alpha[0], alpha[0] * lambda_roe[0]};
double z_2[2]{alpha[1], alpha[1] * lambda_roe[1]};

APDQ[0] = (lambda_roe[0] > 0 ? z_1[0] : 0) + (lambda_roe[1] > 0 ? z_2[0] : 0);
APDQ[1] = 0.0;
APDQ[2] = 0.0;
APDQ[normal+1] = (lambda_roe[0] > 0 ? z_1[1] : 0) + (lambda_roe[1] > 0 ? z_2[1] : 0);

AMDQ[0] = (lambda_roe[0] < 0 ? z_1[0] : 0) + (lambda_roe[1] < 0 ? z_2[0] : 0);
AMDQ[1] = 0.0;
AMDQ[2] = 0.0;
AMDQ[normal+1] = (lambda_roe[0] < 0 ? z_1[1] : 0) + (lambda_roe[1] < 0 ? z_2[1] : 0);

return std::max(std::abs(lambda_roe[0]), std::abs(lambda_roe[1]));
"""

custom_flux = """
double u_n = Q[1] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], 0.0001), 4));
double v_n = Q[2] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], 0.0001), 4));
switch(normal)
{
    case 0: 
        F[0] = Q[0] * u_n;
        F[1] = Q[0] * u_n * u_n;
        F[2] = Q[0] * u_n * v_n;
        break;
    case 1:
        F[0] = Q[0] * v_n;
        F[1] = Q[0] * u_n * v_n;
        F[2] = Q[0] * v_n * v_n;
        break;
}
return;
"""

custom_eigenvalues = """
double g = 9.81;
const double c = std::sqrt(g * Q[0]);
double u_n = Q[normal+1] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], 0.0001), 4));

lambda[0] = u_n + c;
lambda[1] = u_n - c;
lambda[2] = u_n;
return;
"""

solvers = [exahype2.solvers.fv.riemann.UserDefinedRiemann, exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep]
my_solver = solvers[solver](
    name                      = "%s" % "CUSTOMTEST" if solver == 0 else "RUSANOV",
    patch_size                = patch_size,
    unknowns                  = 3,
    auxiliary_variables       = 1,
    max_volume_h              = maximum_cell_size, # must be at least as fine as bathymetry file if netCDF is used
    min_volume_h              = maximum_cell_size * 3.0**(amr_levels),
    time_step_relaxation      = time_step_relaxation
)

if solver == 0:
    my_solver.set_implementation(
    initial_conditions    = my_pde.implementation_of_initial_conditions(),
    boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
    flux                  = custom_flux,
    eigenvalues           = custom_eigenvalues,
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
  dimensions            = dimensions,
  size                  = width,
  offset                = offset,
  min_end_time          = end_time,
  max_end_time          = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval,
  periodic_BC           = [False, False, False]
)

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.generate()
peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)