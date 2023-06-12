#include "tarch/la/Scalar.h"

#include <algorithm>
#include <cmath>

#include "tarch/compiler/CompilerSpecificSettings.h"

double tarch::la::max(double a, double b, double c) { return std::max(a, std::max(b, c)); }

double tarch::la::relativeEpsNormaledAgainstValueGreaterOne(double valueA, double valueB, double eps) {
  return std::max(std::abs(valueA), std::abs(valueB)) <= 1.0 ? eps : eps * std::max(std::abs(valueA), std::abs(valueB));
}

double tarch::la::relativeEps(double valueA, double valueB, double eps) {
  return eps * std::max(std::abs(valueA), std::abs(valueB));
}

#ifdef CompilerICC
#include <mathimf.h>

double tarch::la::pow(double base, double exponent) { return pow(base, exponent); }
#else
double tarch::la::pow(double base, double exponent) { return std::pow(base, exponent); }
#endif

double tarch::la::convertAbsoluteIntoRelativeValue(double referenceValue, double value) {
  const double weight = std::max(1.0, std::abs(referenceValue));
  return value / weight;
}
