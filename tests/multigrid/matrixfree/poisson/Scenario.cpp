#include "Scenario.h"


double tests::multigrid::matrixfree::poisson::getPointWiseRhs(
  const tarch::la::Vector<Dimensions, double>&  x
) {

  // Reference solution type: sinProduct or twoPeaks
  const std::string refSolutionType = "sinProduct";

  if (refSolutionType == "sinProduct") {
    // RHS = 4 * d * pi^2 * sin(2*pi*x) * sin(2*pi*y) * sin(2*pi*z) * ...
    // that corresponds to the reference solution 
    // u = sin(2*pi*x) * sin(2*pi*y) * sin(2*pi*z) * ...

    static constexpr double fourDPiSquared = 4 * Dimensions * tarch::la::PI * tarch::la::PI;

    double sinSquared = 1;
    for (int i=0; i<Dimensions; i++) {
      sinSquared *= std::sin( 2 * tarch::la::PI * x(i) );
    }
    
    return fourDPiSquared * sinSquared;

  } else if (refSolutionType == "twoPeaks") {
    // RHS corresponding to the two-gaussian-peaks solution (only for 2D)
    // u(x,y) = x*(1-x)*y*(1-y) * ( 2 * exp(-((x - x_01) ** 2 + (y - y_01) ** 2) / (2 * sigma1**2)) 
    //                                - exp(-((x - x_02) ** 2 + (y - y_02) ** 2) / (2 * sigma2**2)) )
    
    const double x_01 = 0.3;
    const double y_01 = 0.4;
    const double sigma1 = 0.2;
    const double x_02 = 0.8;
    const double y_02 = 0.6;
    const double sigma2 = 0.1;

    return -( x(0)*(x(0) - 1.0)*
    (
      x(1)*(x(1) - 1.0)*
        ( 
              std::pow(sigma1, 4)*std::pow(sigma2, 2)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
             -std::pow(sigma1, 4)*std::pow(x(1) - y_02, 2)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
        - 2.0*std::pow(sigma1, 2)*std::pow(sigma2, 4)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2)) 
        + 2.0*std::pow(sigma2, 4)*std::pow(x(1) - y_01, 2)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )
        
      - std::pow(sigma1, 4)*std::pow(sigma2, 4)*
        (
          2.0*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
        - 4.0*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )
      
      + 2.0*std::pow(sigma1, 2)*std::pow(sigma2, 2)*(2.0*x(1) - 1.0)*
        (
              std::pow(sigma1, 2)*(x(1) - y_02)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
        - 2.0*std::pow(sigma2, 2)*(x(1) - y_01)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )
    )
    
    + x(1)*(x(1) - 1.0)*
    (
      x(0)*(x(0) - 1.0)*
        (
              std::pow(sigma1, 4)*std::pow(sigma2, 2)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
             -std::pow(sigma1, 4)*std::pow(x(0) - x_02, 2)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2))
        - 2.0*std::pow(sigma1, 2)*std::pow(sigma2, 4)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2)) 
        + 2.0*std::pow(sigma2, 4)*std::pow(x(0) - x_01, 2)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )

      - std::pow(sigma1, 4)*std::pow(sigma2, 4)*
        (
          2.0*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
        - 4.0*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )

      + 2.0*std::pow(sigma1, 2)*std::pow(sigma2, 2)*(2*x(0) - 1.0)*
        (
              std::pow(sigma1, 2)*(x(0) - x_02)*std::exp(-0.5*(std::pow(x(0) - x_02, 2) + std::pow(x(1) - y_02, 2))/std::pow(sigma2, 2)) 
        - 2.0*std::pow(sigma2, 2)*(x(0) - x_01)*std::exp(-0.5*(std::pow(x(0) - x_01, 2) + std::pow(x(1) - y_01, 2))/std::pow(sigma1, 2))
        )
    )
    )/(std::pow(sigma1, 4)*std::pow(sigma2, 4));

  } else {
    std::cerr << "ERROR: Unknown reference solution type '" << refSolutionType << "'" << std::endl;
    exit(-1);
  }

}