import os, sys
import peano4, exahype2

from exahype2.solvers.PDETerms import PDETerms

project = exahype2.Project(["exahype", "limiting", "euler"], ".", executable="Euler_Airfoil")

dimensions          = 2
min_level           = 4
max_depth           = 0
order               = 5

e_t     =   10 #end_time
offset  = [-10, -60]
size    = [120, 120]

max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)


init = """

  //Parameters which define a symmetric NACA airfoil
  constexpr double c = 100.;
  constexpr double t = 0.12;

  Q[0] = 1.0;
  Q[1] = 0.1;
  Q[2] = 0.0;
  Q[3] = 1.0;

  double yl = x[1] - h[1]/2;
  double yu = x[1] + h[1]/2;
  double volumePercentage = 1.0;

  constexpr int nI = 50; //numberOfIntegrationMeasurementPoints
  const double hI = h[0]/nI;

  for(int i=0; i<nI; i++){
    double xI = x[0] + i * hI;
    double yI = 5.0 * t * c * ( 
      0.2969 * sqrt ( xI / c ) 
      + (((( 
      - 0.1015 ) * ( xI / c ) 
      + 0.2843 ) * ( xI / c ) 
      - 0.3516 ) * ( xI / c ) 
      - 0.1260 ) * ( xI / c ) ); //Specific function for a symmetric NACA airfoil

    if( -yI<yl && yI>yu ){ //fully within the airfoil
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
  if( normal==0 && x[0]==DomainOffset[0] ){
    Qoutside[0] = 1.0;
    Qoutside[1] = 0.1;
    Qoutside[2] = 0.0;
    Qoutside[3] = 1.0;
    Qoutside[4] = 1.0;
  }
  else{
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
    Qoutside[4] = 1.0;
  }
"""

flux = """
  const double irho = 1.0 / Q[0];
  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*irho*(Q[3]+p);

  F[normal+1] += p;"""

eigenvalues = """
  const double irho = 1.0 / Q[0];
  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c = std::sqrt(gamma * p * irho);
  const double u = Q[normal+1]*irho;"""

aderSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name  = "aderEuler", order = order,
  min_cell_h = min_h, max_cell_h = max_h,
  time_step_relaxation  = 0.5,
  unknowns              = 4,
  auxiliary_variables   = 1,
  initial_conditions    = init,
  boundary_conditions   = boundaryCondition,
  flux        = flux,
  eigenvalues = eigenvalues + """
    return std::max(std::abs(u-c), std::abs(u+c));"""
)
aderSolver.add_kernel_optimisations(polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto)

project.add_solver(aderSolver)

fvSolver = exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep(
  name = "fvEuler",
  patch_size = 2*order+1,
  min_volume_h          = min_h,
  max_volume_h          = max_h,
  time_step_relaxation  = 0.5,
  unknowns              = 4,
  auxiliary_variables   = 1,
  flux                  = flux,
  boundary_conditions   = """
    tarch::la::Vector<Dimensions, double> x = faceCentre;\n""" + boundaryCondition,
  initial_conditions    = """
    tarch::la::Vector<Dimensions, double> x = volumeCentre;
    tarch::la::Vector<Dimensions, double> h = volumeH;\n""" + init,
  eigenvalues = eigenvalues + """
    L[0] = u-c;
    L[1] = -u+c;
    L[2] = u+c;
    L[3] = -u+c;""",
  riemann_solver = """
    const double sMax = std::max(*std::max_element(LL, LL+4), *std::max_element(LR, LR+4));

    const double vL = QL[4];
    const double vR = QR[4];

    /*
      Assuming anything that can't get into the other is reflected back to me.
      own + what's reflected from what I send + what was sent to me and not reflected
        Rl = vl*Fl + vl*Fl*(1-vr) + vr*Fr*vl
        Rr = vr*Fr + vr*Fr*(1-vl) + vl*Fl*vr
    */
    for(int i=0; i<4; i++){
      const double FLe = 0.5*vL*(FL[i]+sMax*QL[i]);
      const double FRe = 0.5*vR*(FR[i]-sMax*QR[i]);

      //left going update
      AMDQ[i] = FLe*(2-vR) + vL*FRe;
      //right going update
      APDQ[i] = FRe*(2-vL) + vR*FLe;
    }

    return sMax;"""
)
project.add_solver(fvSolver)

# actual limiting solver
limiter = exahype2.solvers.limiting.StaticLimiting(
  name            = "limitingSolver",
  regular_solver   = aderSolver,
  limiting_solver  = fvSolver,
  limiting_criterion_implementation = """return (Q[4]==1.0);"""
)
project.add_solver(limiter)

project.set_output_path("eulerAirfoilSolutions")

dimensions = 2
build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
    dimensions = dimensions,
    offset = offset[0:dimensions],
    size = size[0:dimensions],
    min_end_time = e_t,
    max_end_time = e_t,
    first_plot_time_stamp = 0.0,
    time_in_between_plots = 0.5,
    periodic_BC=[False, False]
)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
    "new ::exahype2::LoadBalancingConfiguration()",
)
project.set_Peano4_installation("../../../", build_mode)
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=16)
