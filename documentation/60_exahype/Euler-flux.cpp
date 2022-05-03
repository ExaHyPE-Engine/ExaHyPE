void examples::exahype2::finitevolumes::Euler::flux(
  double                                       Q[5],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  const tarch::la::Vector<Dimensions,double>&  t,
  int                                          normal,
  double                                       F[5]
) {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]);
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]);
  #endif

  switch (normal) {
    case 0:
        {
          F[0] = Q[1];
          F[1] = irho*Q[1]*Q[1] + p;
          F[2] = irho*Q[2]*Q[1];
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[1] : 0.0;
          F[4] = irho*(Q[4]+p)*Q[1];
        }
        break;
    case 1:
        {
          F[0] = Q[2];
          F[1] = irho*Q[1]*Q[2];
          F[2] = irho*Q[2]*Q[2] + p;
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[2] : 0.0;
          F[4] = irho*(Q[4]+p)*Q[2];
        }
        break;
    case 2:
        {
          F[0] = Q[3];
          F[1] = irho*Q[1]*Q[3];
          F[2] = irho*Q[2]*Q[3];
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[3] + p : 0.0;
          F[4] = irho*(Q[4]+p)*Q[3];
        }
        break;
  }
}
