

def initial():
  return """

  //stresses
  Q[3+0] = 0.0; //xx
  Q[3+1] = 0.0; //yy
  Q[3+2] = 0.0; //zz
  Q[3+3] = 0.0; //xy
  Q[3+4] = 0.0; //xz
  Q[3+5] = 0.0; //yz

  // LOH1
  double layerWidth = 1.0;
  bool upperLayer = x(1) <= layerWidth;

  // HHS
  // bool upperLayer = false;

  Q[ 0 + 0 ] = 0.0;
  Q[ 0 + 1 ] = Q[ 0 + 0 ];
  Q[ 0 + 2 ] = Q[ 0 + 1 ];

  //auxiliary variables
  Q[9]  = upperLayer ? 2.6 : 2.7;
  Q[10] = upperLayer ? 4.0 : 6.0;
  Q[11] = upperLayer ? 2.0 : 3.464;
"""


def boundary():
  return """
  switch(normal){
    case 0:
      Qoutside[3+0] = 0.; //xx
      Qoutside[3+3] = 0.; //xy
      Qoutside[3+4] = 0.; //xz
      Qoutside[0+0] = 0.;
      Qoutside[0+1] = 0.;
      Qoutside[0+2] = 0.;
      break;
    case 1:
      // free surface boundary condition
      Qoutside[3+1] = -Qinside[3+1]; //yy
      Qoutside[3+3] = -Qinside[3+3]; //xy
      Qoutside[3+5] = -Qinside[3+5]; //yz
      Qoutside[0+0] =  Qinside[0+0];
      Qoutside[0+1] =  Qinside[0+1];
      Qoutside[0+2] =  Qinside[0+2];
      break;
    case 2:
      Qoutside[3+2] = 0.; //zz
      Qoutside[3+4] = 0.; //xz
      Qoutside[3+5] = 0.; //yz
      Qoutside[0+0] = 0.;
      Qoutside[0+1] = 0.;
      Qoutside[0+2] = 0.;
  }

  //auxiliary variables
  Qoutside[9]  = Qinside[9];
  Qoutside[10] = Qinside[10];
  Qoutside[11] = Qinside[11];
  
"""

def eigenvalue():
  return """
  return std::max(std::abs(Q[10]), std::abs(Q[11]));
"""

def flux():
  return """
  //Lamé parameters
  auto mu(Q[9] * Q[11] * Q[11]); //rho*cs^2
  auto lambda(Q[9] * Q[10] * Q[10] - 2.0 * mu); //rho*cp^2 - 2 mu
  auto neg_irho(-1.0/Q[9]);

  switch(normal) {
    case 0:
      F[0+0] = neg_irho*Q[3+0]; //sigma_xx
      F[0+1] = neg_irho*Q[3+3]; //sigma_xy
      F[0+2] = neg_irho*Q[3+4]; //sigma_xz
      F[3+0] = -(lambda + 2*mu) * Q[0+0]; //xx
      F[3+1] = -lambda * Q[0+0];          //yy
      F[3+2] = -lambda * Q[0+0];          //zz
      F[3+3] = -mu * Q[0+1];              //xy
      F[3+4] = -mu * Q[0+2];              //xz
      F[3+5] =  0.0;                        //yz
      break;
    case 1:
      F[0+0] = neg_irho*Q[3+3]; //sigma_xy
      F[0+1] = neg_irho*Q[3+1]; //sigma_yy
      F[0+2] = neg_irho*Q[3+5]; //sigma_yz
      F[3+0] = -lambda * Q[0+1];          //xx
      F[3+1] = -(lambda + 2*mu) * Q[0+1]; //yy
      F[3+2] = -  lambda * Q[0+1];          //zz
      F[3+3] = -mu * Q[0+0];              //xy
      F[3+4] =  0.0;                        //xz
      F[3+5] = -mu * Q[0+2];              //yz
      break;
    case 2:
      F[0+0] = neg_irho*Q[3+4]; //sigma_xz
      F[0+1] = neg_irho*Q[3+5]; //sigma_yz
      F[0+2] = neg_irho*Q[3+2]; //sigma_zz
      F[3+0] = -lambda * Q[0+2];          //xx
      F[3+1] = -lambda * Q[0+2];          //yy
      F[3+2] = -(lambda + 2*mu) * Q[0+2]; //zz
      F[3+3] =  0.0;                        //xy
      F[3+4] = -mu * Q[0+0];              //xz
      F[3+5] = -mu * Q[0+1];              //yz
  }
"""

def refinement_criterion():
  return """
  auto result = ::exahype2::RefinementCommand::Keep;

  tarch::la::Vector<DIMENSIONS, double> sourceLocation = {
    pointSourceLocation[0][0],
    pointSourceLocation[0][1],
    #if DIMENSIONS == 3
    pointSourceLocation[0][2]
    #endif
  };

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - sourceLocation) < 1.0) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
"""

def point_source():
  return """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  constexpr double t0 = 0.1;
  constexpr double M0 = 10.;//1000.0;
  double           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[3+4] = f; //sigma_xy
"""
def init_point_source():
  return """
  location[0][0] = 0.0;
  location[0][1] = 2.0;
  location[0][2] = 0.0;
"""

def riemann_solver():
  return """
  Numerics::riemannSolver<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, Order+1, NumberOfUnknowns, NumberOfUnknowns+NumberOfAuxiliaryVariables, 1>(
    FL, FR, QL, QR, dt, direction, isBoundaryFace, faceIndex
  );
"""