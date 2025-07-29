# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"], ".", executable="ADERDG-Limiting"
)

parser = exahype2.ArgumentParser("ExaHyPE 2 - ADER-DG Limiting Testing Script")
parser.set_defaults(
    min_depth=4,
    degrees_of_freedom=6,
)
args = parser.parse_args()

dimensions = 2
order = args.degrees_of_freedom - 1
end_time = 10
plot_interval = 0.0  # 0.5

size = [120, 120]
offset = [-10, -60]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h / (3.0**args.amr_levels)

init = """
  // Parameters which define a symmetric NACA airfoil
  constexpr double c = 100.;
  constexpr double t = 0.12;

  Q[0] = 1.0;
  Q[1] = 0.1;
  Q[2] = 0.0;
  Q[3] = 1.0;

  double yl = x[1] - h[1]/2;
  double yu = x[1] + h[1]/2;
  double volumePercentage = 1.0;

  constexpr int nI = 50; // NumberOfIntegrationMeasurementPoints
  const double hI = h[0] / nI;

  for(int i=0; i<nI; i++){
    double xI = x[0] + i * hI;
    double yI = 5.0 * t * c * (
      0.2969 * sqrt ( xI / c )
      + ((((
      - 0.1015 ) * ( xI / c )
      + 0.2843 ) * ( xI / c )
      - 0.3516 ) * ( xI / c )
      - 0.1260 ) * ( xI / c ) ); // Specific function for a symmetric NACA airfoil

    if( -yI<yl && yI>yu ){ // fully within the airfoil
      volumePercentage -= hI ;
    }
    else if ( yI>yl && yI<yu && -yI<yl ){ // y in cell, -y below
      volumePercentage -= hI*(yI - yl) / (yu - yl);
    }
    else if ( -yI<yu && -yI>yl && yI>yu){ // -y in cell, y above
      volumePercentage -= hI*(yu + yI) / (yu - yl);
    }
    else if ( yI>yl && yI<yu && -yI<yu && -yI>yl ){ // -y and y both in cell
      volumePercentage -= hI*( 2*yI ) / (yu - yl);
    }
  }

  Q[4] = std::max(0.0, std::min(1.0, volumePercentage));
"""

boundaryCondition = """
  if (normal == 0 && x[0] == DomainOffset[0]) {
    Qoutside[0] = 1.0;
    Qoutside[1] = 0.1;
    Qoutside[2] = 0.0;
    Qoutside[3] = 1.0;
    Qoutside[4] = 1.0;
  } else {
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
    Qoutside[4] = 1.0;
  }
"""

flux = """
  const double irho = 1.0 / Q[0];
  constexpr double Gamma = 1.4;
  const double p = (Gamma - 1) * (Q[3] - 0.5 * irho * (Q[1]*Q[1]+Q[2]*Q[2]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1] * Q[1] * irho;
  F[2] = Q[normal+1] * Q[2] * irho;
  F[3] = Q[normal+1] * irho * (Q[3] + p);

  F[normal+1] += p;
"""

eigenvalues = """
  const double irho = 1.0 / Q[0];
  constexpr double Gamma = 1.4;
  const double p = (Gamma - 1) * (Q[3] - 0.5 * irho * (Q[1]*Q[1]+Q[2]*Q[2]));

  const double c = std::sqrt(Gamma * p * irho);
  const double u = Q[normal + 1] * irho;
"""

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.5,
    unknowns=4,
    auxiliary_variables=1,
)
aderdg_solver.set_implementation(
    initial_conditions=init,
    boundary_conditions=boundaryCondition,
    flux=flux,
    max_eigenvalue=eigenvalues
    + """
    return std::max(std::abs(u-c), std::abs(u+c));""",
)
aderdg_solver.add_kernel_optimisations(
    polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto
)
project.add_solver(aderdg_solver)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * order + 1,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
    unknowns=4,
    auxiliary_variables=1
)
fv_solver.set_implementation(
    flux=flux,
    boundary_conditions=boundaryCondition,
    initial_conditions=init,
    eigenvalues=eigenvalues
    + """
    L[0] = u-c;
    L[1] = -u+c;
    L[2] = u+c;
    L[3] = -u+c;""",
    riemann_solver="""
    double LL[4];
    double LR[4];
    eigenvalues(QL, x, h, t, dt, normal, LL);
    eigenvalues(QR, x, h, t, dt, normal, LR);
    const double sMax = std::max(*std::max_element(LL, LL+4), *std::max_element(LR, LR+4));

    double fluxLeft[4];
    double fluxRight[4];
    flux(QL, x, h, t, dt, normal, fluxLeft);
    flux(QR, x, h, t, dt, normal, fluxRight);

    const double vL = QL[4];
    const double vR = QR[4];

    /*
      Assuming anything that can't get into the other is reflected back to me.
      own + what's reflected from what I send + what was sent to me and not reflected
        Rl = vl*Fl + vl*Fl*(1-vr) + vr*Fr*vl
        Rr = vr*Fr + vr*Fr*(1-vl) + vl*Fl*vr
    */
    for(int i=0; i<4; i++){
      const double FLe = 0.5*vL*(fluxLeft[i]+sMax*QL[i]);
      const double FRe = 0.5*vR*(fluxRight[i]-sMax*QR[i]);

      //left going update
      FL[i] = FLe*(2-vR) + vL*FRe;
      //right going update
      FR[i] = FRe*(2-vL) + vR*FLe;
    }

    return sMax;""",
)
project.add_solver(fv_solver)

limiter = exahype2.solvers.limiting.StaticLimiting(
    name="LimitingSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    limiting_criterion_implementation="""return (Q[4] == 1.0);""",
)
project.add_solver(limiter)

project.set_output_path("solutions")

project.set_global_simulation_parameters(
    dimensions=dimensions,
    offset=offset[0:dimensions],
    size=size[0:dimensions],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_interval,
    periodic_BC=[False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation("../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(False)
project.build()
