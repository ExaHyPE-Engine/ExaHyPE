void examples::exahype2::finitevolumes::Euler::eigenvalues(
  double                                       Q[5],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  const tarch::la::Vector<Dimensions,double>&  t,
  int                                          normal,
  double                                       lambda[5]
) {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]);
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]);
  #endif

  const double u_n = Q[normal + 1] * irho;
  const double c   = std::sqrt(gamma * p * irho);

  lambda[0]  = u_n;
  lambda[1]  = u_n;
  lambda[2]  = u_n;
  lambda[3]  = u_n + c;
  lambda[4]  = u_n - c;
}
