#include "tarch/la/Scalar.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <cmath>
#include <algorithm>


double tarch::la::max( double a, double b, double c) {
  return std::max( a, std::max(b,c) );
}


double tarch::la::relativeEps( double valueA, double valueB, double eps ) {
  return std::max(valueA,valueB) <= 1.0 ? eps : eps * std::max(valueA,valueB);
}


#ifdef CompilerICC
#include <mathimf.h>


double tarch::la::pow( double base, double exponent ) {
  return pow(base,exponent);
}
#else
double tarch::la::pow( double base, double exponent ) {
  return std::pow(base,exponent);
}
#endif


double tarch::la::convertAbsoluteIntoRelativeValue(
  double referenceValue,
  double value
) {
  const double weight = std::max(
    1.0, std::abs(referenceValue)
  );
  return value / weight;
}

