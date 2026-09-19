

def initial():
  return """

  //stresses
  Q[Shortcuts::sigma+0] = 0; //xx
  Q[Shortcuts::sigma+1] = 0; //yy
  Q[Shortcuts::sigma+2] = 0; //zz
  Q[Shortcuts::sigma+3] = 0; //xy
  Q[Shortcuts::sigma+4] = 0; //xz
  Q[Shortcuts::sigma+5] = 0; //yz

  // LOH1
  bool upperLayer = (x(1) <= 1.0);

  // HHS
  // bool upperLayer = false;

  Q[Shortcuts::v+0] = 0;
  Q[Shortcuts::v+1] = 0;
  Q[Shortcuts::v+2] = 0;

  //auxiliary variables
  Q[Shortcuts::rho] = (upperLayer ? 2.6 : 2.7);
  Q[Shortcuts::cp]  = (upperLayer ? 4.0 : 6.0);
  Q[Shortcuts::cs]  = (upperLayer ? 2.0 : 3.464);
"""


def boundary():
  return """
  switch(normal){
    case 0:
      Qoutside[Shortcuts::sigma+0] = 0; //xx
      Qoutside[Shortcuts::sigma+3] = 0; //xy
      Qoutside[Shortcuts::sigma+4] = 0; //xz
      Qoutside[Shortcuts::v+0] = 0;
      Qoutside[Shortcuts::v+1] = 0;
      Qoutside[Shortcuts::v+2] = 0;
      break;
    case 1:
      // free surface boundary condition
      Qoutside[Shortcuts::sigma+1] = -Qinside[Shortcuts::sigma+1]; //yy
      Qoutside[Shortcuts::sigma+3] = -Qinside[Shortcuts::sigma+3]; //xy
      Qoutside[Shortcuts::sigma+5] = -Qinside[Shortcuts::sigma+5]; //yz
      Qoutside[Shortcuts::v+0] =  Qinside[Shortcuts::v+0];
      Qoutside[Shortcuts::v+1] =  Qinside[Shortcuts::v+1];
      Qoutside[Shortcuts::v+2] =  Qinside[Shortcuts::v+2];
      break;
    case 2:
      Qoutside[Shortcuts::sigma+2] = 0; //zz
      Qoutside[Shortcuts::sigma+4] = 0; //xz
      Qoutside[Shortcuts::sigma+5] = 0; //yz
      Qoutside[Shortcuts::v+0] = 0;
      Qoutside[Shortcuts::v+1] = 0;
      Qoutside[Shortcuts::v+2] = 0;
  }

  //auxiliary variables
  Qoutside[Shortcuts::rho]  = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::cp]   = Qinside[Shortcuts::cp];
  Qoutside[Shortcuts::cs]   = Qinside[Shortcuts::cs];
"""

def eigenvalue():
  return """
  return (double) Q[Shortcuts::cp];
"""

def flux():
  return """
  //Lamé parameters
  auto mu(Q[Shortcuts::rho] * Q[Shortcuts::cs] * Q[Shortcuts::cs]); //rho*cs^2
  auto lambda(Q[Shortcuts::rho] * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * mu); //rho*cp^2 - 2 mu
  auto neg_irho(-1/Q[Shortcuts::rho]);

  switch(normal) {
    case 0:
      F[Shortcuts::v+0] = neg_irho*Q[Shortcuts::sigma+0]; //sigma_xx
      F[Shortcuts::v+1] = neg_irho*Q[Shortcuts::sigma+3]; //sigma_xy
      F[Shortcuts::v+2] = neg_irho*Q[Shortcuts::sigma+4]; //sigma_xz
      F[Shortcuts::sigma+0] = -(lambda + 2*mu) * Q[Shortcuts::v+0]; //xx
      F[Shortcuts::sigma+1] = -lambda * Q[Shortcuts::v+0];          //yy
      F[Shortcuts::sigma+2] = -lambda * Q[Shortcuts::v+0];          //zz
      F[Shortcuts::sigma+3] = -mu * Q[Shortcuts::v+1];              //xy
      F[Shortcuts::sigma+4] = -mu * Q[Shortcuts::v+2];              //xz
      F[Shortcuts::sigma+5] =  0;                        //yz
      break;
    case 1:
      F[Shortcuts::v+0] = neg_irho*Q[Shortcuts::sigma+3]; //sigma_xy
      F[Shortcuts::v+1] = neg_irho*Q[Shortcuts::sigma+1]; //sigma_yy
      F[Shortcuts::v+2] = neg_irho*Q[Shortcuts::sigma+5]; //sigma_yz
      F[Shortcuts::sigma+0] = -lambda * Q[Shortcuts::v+1];          //xx
      F[Shortcuts::sigma+1] = -(lambda + 2*mu) * Q[Shortcuts::v+1]; //yy
      F[Shortcuts::sigma+2] = -lambda * Q[Shortcuts::v+1];          //zz
      F[Shortcuts::sigma+3] = -mu * Q[Shortcuts::v+0];              //xy
      F[Shortcuts::sigma+4] =  0;                        //xz
      F[Shortcuts::sigma+5] = -mu * Q[Shortcuts::v+2];              //yz
      break;
    case 2:
      F[Shortcuts::v+0] = neg_irho*Q[Shortcuts::sigma+4]; //sigma_xz
      F[Shortcuts::v+1] = neg_irho*Q[Shortcuts::sigma+5]; //sigma_yz
      F[Shortcuts::v+2] = neg_irho*Q[Shortcuts::sigma+2]; //sigma_zz
      F[Shortcuts::sigma+0] = -lambda * Q[Shortcuts::v+2];          //xx
      F[Shortcuts::sigma+1] = -lambda * Q[Shortcuts::v+2];          //yy
      F[Shortcuts::sigma+2] = -(lambda + 2*mu) * Q[Shortcuts::v+2]; //zz
      F[Shortcuts::sigma+3] =  0;                        //xy
      F[Shortcuts::sigma+4] = -mu * Q[Shortcuts::v+0];              //xz
      F[Shortcuts::sigma+5] = -mu * Q[Shortcuts::v+1];              //yz
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
    forceVector[i] = 0;
  }

  constexpr auto t0 = 0.1;
  constexpr auto M0 = 10;//1000.0;
  auto           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[Shortcuts::sigma+4] = f; //sigma_xy
"""
def init_point_source():
  return """
  location[0][0] = 0;
  location[0][1] = 2;
  location[0][2] = 0;
"""

def riemann_solver():
  return """
  applications::exahype2::exaseis::riemannSolverIsotropic<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, DGOrder + 1, NumberOfUnknowns, NumberOfUnknowns+NumberOfAuxiliaryVariables, 1>(
    FL, FR, QL, QR, t, dt, direction, isBoundaryFace, faceIndex
  );
"""
