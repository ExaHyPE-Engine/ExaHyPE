# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=10.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

dimensions = 2
order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="StaticLimitingEulerAirfoil",
    dimensions=dimensions,
    size=[120, 120, 120],
    offset=[-10, -60, 0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(
        f"Static limiting Euler airfoil is a {dimensions}D testcase, but the "
        f"project was configured for {project.dimensions}D."
    )

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
  const double hI = 1. / nI;

  for(int i=0; i<nI; i++){
    double xI = x[0] + h[0] * i * hI;
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

  return std::fmax(std::fabs(u - c), std::fabs(u + c));
"""

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    time_step_relaxation=0.5,
    unknowns={"rho": 1, "v": 2, "E": 1},
    auxiliary_variables={"alpha": 1}
)
aderdg_solver.set_implementation(
    initial_conditions=init,
    boundary_conditions=boundaryCondition,
    flux=flux,
    max_eigenvalue=eigenvalues
)
aderdg_solver.add_kernel_optimisations(
    polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto
)
project.add_solver(aderdg_solver)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * order + 1,
    time_step_relaxation=0.5,
    unknowns={"rho": 1, "v": 2, "E": 1},
    auxiliary_variables={"alpha": 1},
)
fv_solver.set_implementation(
    flux=flux,
    boundary_conditions=boundaryCondition,
    initial_conditions=init,
    riemann_solver="""
    auto eigenvalues = [](
      const double* const                           Q,
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      double                                        t,
      double                                        dt,
      int                                           normal
    ) -> double {""" + eigenvalues +"""
    };

    const double sMax = std::max(
      eigenvalues(QL, x, h, t, dt, normal),
      eigenvalues(QR, x, h, t, dt, normal)
    );

    double fluxLeft[4];
    double fluxRight[4];
    flux(QL, x, h, t, dt, normal, fluxLeft);
    flux(QR, x, h, t, dt, normal, fluxRight);

    const double vL = QL[4];
    const double vR = QR[4];

  /*
    Assuming anything that can't get into the other is reflected back to me.
    i.e. the other side has a percentage equal to (1-alphaX) which is a wall boundary condition
    Wall boundary conditions cause a flux of 0 in all parameters except the normal velocity,
    where they cause a flux equal to the flux inside of the boundary
  */
    for(int i=0; i<4; i++){
      const double rusanovFlux = 0.5*(fluxLeft[i]+fluxRight[i]+sMax*(QL[i]-QR[i]));

      FL[i] = vR*rusanovFlux;
      FR[i] = vL*rusanovFlux;
    }
    FL[normal+1] += (1-vR)*fluxLeft[normal+1];
    FR[normal+1] += (1-vL)*fluxRight[normal+1];

    return sMax;""",
)
project.add_solver(fv_solver)

limiter = exahype2.solvers.limiting.StaticLimiting(
    name="LimitingSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    physical_admissibility_criterion="""return (Q[4] == 1.0);""",
)
project.add_solver(limiter)

project.generate_and_build()
