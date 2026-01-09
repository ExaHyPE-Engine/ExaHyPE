# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

max_eigenvalue = r"""
  // The eigenvalue represents the maximal velocity as a fraction of
  // the speed of light, as such it can never be greater than 1, and
  // returning 1 is safe as it an upper bound for the real eigenvalue
  // return 1.0;

  // PARAMETERS
  constexpr double gamma = 4.0/3.0;

  // Eigenvalues as given by Olindo
  double V[NumberOfUnknowns];
  //TODO: get whether this produced an error? make that an assertion?
  GRMHD::PDE::PDECons2Prim(Q, V);

  double rho    = V[Shortcuts::rho];
  double v_cov[3] = {
    V[Shortcuts::vel + 0], V[Shortcuts::vel + 1], V[Shortcuts::vel + 2]
  };
  double p = V[Shortcuts::E];
  double B_contr[3] = {
    V[Shortcuts::B + 0], V[Shortcuts::B + 1], V[Shortcuts::B + 2]
  };

  double shift[3] = {
    V[Shortcuts::shift + 0], V[Shortcuts::shift + 1], V[Shortcuts::shift + 2]
  };
  // double psi = V[Shortcuts::psi]; // unused variable
  double lapse = V[Shortcuts::lapse];

  double g_cov[3][3] = {
    {V[Shortcuts::gij + 0], V[Shortcuts::gij + 1], V[Shortcuts::gij + 2]},
    {V[Shortcuts::gij + 1], V[Shortcuts::gij + 3], V[Shortcuts::gij + 4]},
    {V[Shortcuts::gij + 2], V[Shortcuts::gij + 4], V[Shortcuts::gij + 5]}
  };

  double g_contr[3][3];
  double gp = GRMHD::PDE::matrixInverse3x3(g_cov, g_contr);
  gp = std::sqrt(gp);
  // double gm = 1./gp;

  double v_contr[3];
  GRMHD::PDE::matrixVectMult(g_contr, v_cov, v_contr);

  double B_cov[3];
  GRMHD::PDE::matrixVectMult(g_cov, B_contr, B_cov);

  // evaluate useful quantities
  double v2 = v_contr[0]*v_cov[0] + v_contr[1]*v_cov[1] + v_contr[2]*v_cov[2];
  // double lf = 1.0/sqrt(1.0 - v2);
  double lf2m = 1.0 - v2;
  double b2 = B_contr[0]*B_cov[0] + B_contr[1]*B_cov[1] + B_contr[2]*B_cov[2];

  double VdotB = v_contr[0]*B_cov[0] + v_contr[1]*B_cov[1] + v_contr[2]*B_cov[2];
  double b2_4 = b2*lf2m + VdotB;
  double gamma1 = gamma/(gamma-1.0);
  double w      = rho + gamma1*p + b2_4;
  double cs2    = (gamma*p + b2_4)/w;

  double u      = v_contr[normal];
  double sft    = shift[normal];
  double gg     = g_contr[normal][normal];
  double den    = 1.0/(1.0 - v2*cs2);

  double deltaU = sqrt( cs2*lf2m*( (1.0-v2*cs2)*gg - u*u*(1.0-cs2) ));

  double maxEigenvalue = std::abs(lapse*u -sft);

  maxEigenvalue = std::max( maxEigenvalue,
    std::abs(
    lapse*den*( u*(1.0-cs2) - deltaU ) - sft
  ));

  maxEigenvalue = std::max( maxEigenvalue,
    std::abs(
    lapse*den*( u*(1.0-cs2) + deltaU ) - sft
  ));

  maxEigenvalue = std::max( maxEigenvalue,
    1.0
  );

  return maxEigenvalue;

  //Safe mode: return also 'covariant' eigenvalues, should never be required
  // double shift_cov[3];
  // GRMHD::PDE::matrixVectMult(g_cov, shift, shift_cov);
  // double vn2 = v_cov[normal];
  // double sft2 = shift_cov[normal];
  // double gg2 = g_cov[normal][normal];
  // den = 1.0/(1.0 - v2*cs2);

  // u = vn2;
  // deltaU = sqrt( cs2*lf2m*( (1.0-v2*cs2)*gg2 - u*u*(1.0-cs2) ));

  // maxEigenvalue = std::max( maxEigenvalue,
  //   std::abs(
  //   lapse*den*( u*(1.0-cs2) - deltaU ) - sft2
  // ));

  // maxEigenvalue = std::max( maxEigenvalue,
  //   std::abs(
  //   lapse*den*( u*(1.0-cs2) - deltaU ) - sft2
  // ));

  // maxEigenvalue = std::max( maxEigenvalue,
  //   std::abs(
  //   lapse*den*( u*(1.0-cs2) - deltaU ) - sft2
  // ));
"""

flux = r"""
  std::fill_n(F, NumberOfUnknowns, 0.0);

  constexpr double gamma = 4.0/3.0;
  double gamma1 = gamma/(gamma-1.0);

  double V[NumberOfUnknowns];
  //TODO: get whether this produced an error? make that an assertion?
  GRMHD::PDE::PDECons2Prim(Q, V);

  // int iErr;
  // pdecons2prim_(V,Q,&iErr);

  double rho    = V[Shortcuts::rho];
  double v_cov[3] = {
    V[Shortcuts::vel + 0], V[Shortcuts::vel + 1], V[Shortcuts::vel + 2]
  };
  double p = V[Shortcuts::E];
  double B_contr[3] = {
    V[Shortcuts::B + 0], V[Shortcuts::B + 1], V[Shortcuts::B + 2]
  };
  double QB_contr[3] = {
    Q[Shortcuts::B + 0], Q[Shortcuts::B + 1], Q[Shortcuts::B + 2]
  };
  double shift[3] = {
    V[Shortcuts::shift + 0], V[Shortcuts::shift + 1], V[Shortcuts::shift + 2]
  };
  // double psi = V[Shortcuts::psi]; // unused variable
  double lapse = V[Shortcuts::lapse];

  double g_cov[3][3] = {
    {V[Shortcuts::gij + 0], V[Shortcuts::gij + 1], V[Shortcuts::gij + 2]},
    {V[Shortcuts::gij + 1], V[Shortcuts::gij + 3], V[Shortcuts::gij + 4]},
    {V[Shortcuts::gij + 2], V[Shortcuts::gij + 4], V[Shortcuts::gij + 5]}
  };

  double g_contr[3][3];
  double gp = GRMHD::PDE::matrixInverse3x3(g_cov, g_contr);
  gp = std::sqrt(gp);
  double gm = 1./gp;

  double v_contr[3];
  GRMHD::PDE::matrixVectMult(g_contr, v_cov, v_contr);

  double S_contr[3];
  GRMHD::PDE::matrixVectMult(g_contr, &Q[1], S_contr);

  double BQ[3];
  GRMHD::PDE::matrixVectMult(g_cov, QB_contr, BQ);

  double B_cov[3];
  GRMHD::PDE::matrixVectMult(g_cov, B_contr, B_cov);

  // COVARIANT =>
  double vxB_cov[3] = {
    gp*(v_contr[1]*B_contr[2] - v_contr[2]*B_contr[1]),
    gp*(v_contr[2]*B_contr[0] - v_contr[0]*B_contr[2]),
    gp*(v_contr[0]*B_contr[1] - v_contr[1]*B_contr[0])
  };

  // CONTRAVARIANT =>
  double vxB_contr[3] = {
    gm*(v_cov[1]*B_cov[2] - v_cov[2]*B_cov[1]),
    gm*(v_cov[2]*B_cov[0] - v_cov[0]*B_cov[2]),
    gm*(v_cov[0]*B_cov[1] - v_cov[1]*B_cov[0])
  };

  double v2 = v_contr[0]*v_cov[0] + v_contr[1]*v_cov[1] + v_contr[2]*v_cov[2];
  double e2 = vxB_contr[0]*vxB_cov[0] + vxB_contr[1]*vxB_cov[1] + vxB_contr[2]*vxB_cov[2];
  double b2 = B_contr[0]*B_cov[0] + B_contr[1]*B_cov[1] + B_contr[2]*B_cov[2];

  double uem = 0.5*(b2 + e2);
  double lf = 1.0/sqrt(1.0 - v2);

  double w   = rho + gamma1*p;
  double ww  = w*lf*lf;
  double wwx = ww*v_contr[0];
  double wwy = ww*v_contr[1];
  double wwz = ww*v_contr[2];

  double Vtr[3] = {
    lapse*v_contr[0]-shift[0], lapse*v_contr[1]-shift[1], lapse*v_contr[2]-shift[2]
  };

  double Fij[3][3];
  for(int i=0; i<3; i++){
    for(int j=0; j<3; j++){
      Fij[i][j] = Vtr[i]*QB_contr[j]-Vtr[j]*QB_contr[i];
    }
  }

  switch (normal) {
  case 0:
    F[0] = v_contr[0]*Q[0];
    F[1] = gp*(wwx*v_cov[0] - vxB_contr[0]*vxB_cov[0] - B_contr[0]*B_cov[0] + p + uem);
    F[2] = gp*(wwx*v_cov[1] - vxB_contr[0]*vxB_cov[1] - B_contr[0]*B_cov[1]);
    F[3] = gp*(wwx*v_cov[2] - vxB_contr[0]*vxB_cov[2] - B_contr[0]*B_cov[2]);
    F[4] = S_contr[0]-F[0];
    F[5] = Fij[0][0];
    F[6] = Fij[0][1];
    F[7] = Fij[0][2];
    break;
  case 1:
    F[0] = v_contr[1]*Q[0];
    F[1] = gp*(wwy*v_cov[0] - vxB_contr[1]*vxB_cov[0] - B_contr[1]*B_cov[0]);
    F[2] = gp*(wwy*v_cov[1] - vxB_contr[1]*vxB_cov[1] - B_contr[1]*B_cov[1] + p + uem);
    F[3] = gp*(wwy*v_cov[2] - vxB_contr[1]*vxB_cov[2] - B_contr[1]*B_cov[2]);
    F[4] = S_contr[1]-F[0];
    F[5] = Fij[1][0];
    F[6] = Fij[1][1];
    F[7] = Fij[1][2];
    break;
  case 2:
    F[0] = v_contr[2]*Q[0];
    F[1] = gp*(wwz*v_cov[0] - vxB_contr[2]*vxB_cov[0] - B_contr[2]*B_cov[0]);
    F[2] = gp*(wwz*v_cov[1] - vxB_contr[2]*vxB_cov[1] - B_contr[2]*B_cov[1]);
    F[3] = gp*(wwz*v_cov[2] - vxB_contr[2]*vxB_cov[2] - B_contr[2]*B_cov[2] + p + uem);
    F[4] = S_contr[2]-F[0];
    F[5] = Fij[2][0];
    F[6] = Fij[2][1];
    F[7] = Fij[2][2];
    break;
  }

  // Remember that Q(:) below contains already the factor gp, which is ok!
  for(int i=0; i<5; i++){
    F[i] = lapse*F[i] - shift[normal]*Q[i];
  }
"""

ncp = r"""
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  // PARAMETERS
  constexpr double gamma = 4.0/3.0;
  constexpr double DivCleaning_a = 1.0;

  double lapse = Q[9];
  double shift[3] = {
    Q[Shortcuts::shift+0], Q[Shortcuts::shift+1], Q[Shortcuts::shift+2]
  };

  double g_cov[3][3] = {
    {Q[13], Q[14], Q[15]},
    {Q[14], Q[16], Q[17]},
    {Q[15], Q[17], Q[18]}
  };

  double g_contr[3][3];
  double gp = GRMHD::PDE::matrixInverse3x3(g_cov, g_contr);

  gp = std::sqrt(gp);
  double gm = 1./gp;

  double Vc[NumberOfUnknowns];
  //TODO: get whether this produced an error? make that an assertion?
  GRMHD::PDE::PDECons2Prim(Q, Vc);

  double gamma1 = gamma/(gamma-1.);

  double rho = Vc[0];
  double v_cov[3] = {
    Vc[1], Vc[2], Vc[3]
  };
  double p = Vc[4];

  double B_contr[3] = {
    Vc[Shortcuts::B + 0], Vc[Shortcuts::B + 1], Vc[Shortcuts::B + 2]
  };
  // double QB_contr[3] = {
  //   Q[Shortcuts::B + 0], Q[Shortcuts::B + 1], Q[Shortcuts::B + 2]
  // }; //unused variable

  double B_cov[3];
  GRMHD::PDE::matrixVectMult(g_cov, B_contr, B_cov);

  // double psi = Vc[Shortcuts::psi];

  double v_contr[3];
  GRMHD::PDE::matrixVectMult(g_contr, v_cov, v_contr);

  double S_contr[3];
  GRMHD::PDE::matrixVectMult(g_contr, &Q[Shortcuts::vel], S_contr);

  // COVARIANT =>
  double vxB_cov[3] = {
    gp*(v_contr[1]*B_contr[2] - v_contr[2]*B_contr[1]),
    gp*(v_contr[2]*B_contr[0] - v_contr[0]*B_contr[2]),
    gp*(v_contr[0]*B_contr[1] - v_contr[1]*B_contr[0])
  };

  // CONTRAVARIANT =>
  double vxB_contr[3] = {
    gm*(v_cov[1]*B_cov[2] - v_cov[2]*B_cov[1]),
    gm*(v_cov[2]*B_cov[0] - v_cov[0]*B_cov[2]),
    gm*(v_cov[0]*B_cov[1] - v_cov[1]*B_cov[0])
  };

  double v2 = v_contr[0]*v_cov[0] + v_contr[1]*v_cov[1] + v_contr[2]*v_cov[2];
  double e2 = vxB_contr[0]*vxB_cov[0] + vxB_contr[1]*vxB_cov[1] + vxB_contr[2]*vxB_cov[2];
  double b2 = B_contr[0]*B_cov[0] + B_contr[1]*B_cov[1] + B_contr[2]*B_cov[2];

  double uem = 0.5*(b2 + e2);
  double lf = 1.0/sqrt(1.0 - v2);

  double w   = rho + gamma1*p;
  double ww  = w*lf*lf;

  double delta[3][3] = {
    {1., 0., 0.}, {0., 1., 0.}, {0., 0., 1.}
  };

  //Consider unrolling these loops
  int ccount = 0;
  for(int i=0; i<3; i++){
    double W_ij = ww*v_cov[i]*v_contr[normal]-vxB_cov[i]*vxB_contr[normal]-B_cov[i]*B_contr[normal]+(p+uem)*delta[i][normal];
    BTimesDeltaQ[Shortcuts::vel + normal] -= Q[Shortcuts::vel+i]*deltaQ[Shortcuts::shift+i];
    BTimesDeltaQ[Shortcuts::E] -= gp*W_ij*deltaQ[Shortcuts::shift+i];

    for(int j=0; j<3; j++){
      if(j>=i){
        double Wim = ww*v_contr[i]*v_contr[j]-vxB_contr[i]*vxB_contr[j]-B_contr[i]*B_contr[j]+(p+uem)*g_contr[i][j];
        if(i!=j){
          Wim = 2.0*Wim;
        }
        BTimesDeltaQ[Shortcuts::vel+normal] -= 0.5*gp*lapse*Wim*deltaQ[Shortcuts::gij+ccount];
        BTimesDeltaQ[Shortcuts::E] -= 0.5*gp*Wim*shift[normal]*deltaQ[Shortcuts::gij+ccount];
        ccount += 1;
      }
    }
  }

  BTimesDeltaQ[Shortcuts::vel+normal] += (Q[Shortcuts::E]+Q[Shortcuts::rho])*deltaQ[Shortcuts::lapse];
  BTimesDeltaQ[Shortcuts::E]   += S_contr[normal]*deltaQ[Shortcuts::lapse];
  BTimesDeltaQ[Shortcuts::B+0] = lapse*gp*g_contr[0][normal]*deltaQ[Shortcuts::psi]
    - Q[Shortcuts::shift+0]*deltaQ[Shortcuts::B+normal];
  BTimesDeltaQ[Shortcuts::B+1] = lapse*gp*g_contr[1][normal]*deltaQ[Shortcuts::psi]
    - Q[Shortcuts::shift+1]*deltaQ[Shortcuts::B+normal];
  BTimesDeltaQ[Shortcuts::B+2] = lapse*gp*g_contr[2][normal]*deltaQ[Shortcuts::psi]
    - Q[Shortcuts::shift+2]*deltaQ[Shortcuts::B+normal];
  BTimesDeltaQ[Shortcuts::psi] = gm*lapse*DivCleaning_a*DivCleaning_a*deltaQ[Shortcuts::B+normal]
    - Q[Shortcuts::shift+normal]*deltaQ[Shortcuts::psi];
"""
