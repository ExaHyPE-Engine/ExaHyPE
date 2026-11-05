# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import CurvilinearElastic


def initial():
    return """
  // The damping grows as the distance into the layer to the power PmlPower. The lower face of the
  // axis PmlTop is a free surface without a layer.
  {
    constexpr int    NodesPerAxis       = DGOrder + 1;
    constexpr double AmplitudeNumerator = 6.0;
    const auto&      quadraturePoints   = Quadrature<{{STORAGE_PRECISION}}>::getQuadraturePoints1D();
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const auto   width      = PmlCellWidth * h[axis];
      const auto   upperBound = DomainOffset[axis] + DomainSize[axis] - width;
      const auto   lowerBound = DomainOffset[axis] + width;
      const auto   amplitude  = (PmlPower + 1) * AmplitudeNumerator / (2 * width) * log(1 / PmlRelativeError);
      const double lowerEdge  = x[axis] - h[axis] * 0.5;
      for (int node = 0; node < NodesPerAxis * NodesPerAxis * NodesPerAxis; node++) {
        int nodeIndexAlongAxis = node;
        for (int lowerAxis = 0; lowerAxis < axis; lowerAxis++) {
          nodeIndexAlongAxis /= NodesPerAxis;
        }
        nodeIndexAlongAxis %= NodesPerAxis;
        const double position = lowerEdge + h[axis] * quadraturePoints[nodeIndexAlongAxis];
        auto&        damping  = Q[node].pmlDamping(axis);
        if (PmlTop != axis and position < lowerBound) {
          damping = amplitude * std::pow((lowerBound - position) / width, PmlPower);
        }
        if (position >= upperBound) {
          damping = amplitude * std::pow((position - upperBound) / width, PmlPower);
        }
      }
    }
  }
"""


def flux():
    return CurvilinearElastic.flux() + """
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    F.pml(normal, axis) = F.v(axis);
  }
"""


def ncp():
    return """
  // The first three of the nine entries per axis contain the velocity and the last six the stress.
  const auto& metric              = Q.metricDerivative;
  const auto& velocityJump        = deltaQ.v;
  const auto  density             = Q.rho;
  const auto  damping             = Q.pmlDamping(normal);
  const auto  shearModulus        = density * Q.cs * Q.cs;
  const auto  firstLameParameter  = density * Q.cp * Q.cp - 2 * shearModulus;
  const auto  lameTimesDivergence = firstLameParameter
    * (-metric(normal, 0) * velocityJump(0) - metric(normal, 1) * velocityJump(1) - metric(normal, 2) * velocityJump(2));
  BTimesDeltaQ = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    BTimesDeltaQ.sigma(axis, axis) =
      -2 * shearModulus * metric(normal, axis) * velocityJump(axis) + lameTimesDivergence;
    BTimesDeltaQ.pml(normal, 3 + axis) = -damping * metric(normal, axis) * velocityJump(axis);
  }
  BTimesDeltaQ.sigma(0, 1) = -shearModulus * (metric(normal, 1) * velocityJump(0) + metric(normal, 0) * velocityJump(1));
  BTimesDeltaQ.sigma(0, 2) = -shearModulus * (metric(normal, 2) * velocityJump(0) + metric(normal, 0) * velocityJump(2));
  BTimesDeltaQ.sigma(1, 2) = -shearModulus * (metric(normal, 2) * velocityJump(1) + metric(normal, 1) * velocityJump(2));
  BTimesDeltaQ.pml(normal, 6) = -damping * (metric(normal, 1) * velocityJump(0) + metric(normal, 0) * velocityJump(1));
  BTimesDeltaQ.pml(normal, 7) = -damping * (metric(normal, 2) * velocityJump(0) + metric(normal, 0) * velocityJump(2));
  BTimesDeltaQ.pml(normal, 8) = -damping * (metric(normal, 2) * velocityJump(1) + metric(normal, 1) * velocityJump(2));
"""


def algebraic_source():
    return """
  const auto density             = Q.rho;
  const auto shearModulus        = density * Q.cs * Q.cs;
  const auto firstLameParameter  = density * Q.cp * Q.cp - 2 * shearModulus;
  const auto longitudinalModulus = 2 * shearModulus + firstLameParameter;
  const auto inverseDensity      = 1 / density;
  // The first three of the nine entries per axis contain the velocity and the last six the stress.
  const auto& layer = Q.pml;

  for (int component = 0; component < DIMENSIONS; component++) {
    S.v(component) = -inverseDensity * (layer(0, component) + layer(1, component) + layer(2, component));
  }
  S.sigma(0, 0) = -(longitudinalModulus * layer(0, 3) + firstLameParameter * (layer(0, 4) + layer(0, 5))
                  + longitudinalModulus * layer(1, 3) + firstLameParameter * (layer(1, 4) + layer(1, 5))
                  + longitudinalModulus * layer(2, 3) + firstLameParameter * (layer(2, 4) + layer(2, 5)));
  S.sigma(1, 1) = -(longitudinalModulus * layer(0, 4) + firstLameParameter * (layer(0, 3) + layer(0, 5))
                  + longitudinalModulus * layer(1, 4) + firstLameParameter * (layer(1, 3) + layer(1, 5))
                  + longitudinalModulus * layer(2, 4) + firstLameParameter * (layer(2, 3) + layer(2, 5)));
  S.sigma(2, 2) = -(longitudinalModulus * layer(0, 5) + firstLameParameter * (layer(0, 3) + layer(0, 4))
                  + longitudinalModulus * layer(1, 5) + firstLameParameter * (layer(1, 3) + layer(1, 4))
                  + longitudinalModulus * layer(2, 5) + firstLameParameter * (layer(2, 3) + layer(2, 4)));
  S.sigma(0, 1) = -shearModulus * (layer(0, 6) + layer(1, 6) + layer(2, 6));
  S.sigma(0, 2) = -shearModulus * (layer(0, 7) + layer(1, 7) + layer(2, 7));
  S.sigma(1, 2) = -shearModulus * (layer(0, 8) + layer(1, 8) + layer(2, 8));
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto damping    = Q.pmlDamping(axis);
    const auto relaxation = PmlAlphaConstant + PmlAlphaScalar * damping;
    for (int entry = 0; entry < 9; entry++) {
      S.pml(axis, entry) = -(damping + relaxation) * layer(axis, entry);
    }
  }
"""


def multiplyMaterialParameterMatrix():
    return """
  const auto jacobian                    = Q.jacobian;
  const auto inverseDensityTimesJacobian = 1 / (Q.rho * jacobian);
  rhs.v                                  = inverseDensityTimesJacobian * rhs.v;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto damping = Q.pmlDamping(axis);
    for (int component = 0; component < DIMENSIONS; component++) {
      rhs.pml(axis, component) = damping / jacobian * rhs.pml(axis, component);
    }
  }
"""


def riemann_solver():
    return CurvilinearElastic.riemann_solver()


def userIncludes():
    return CurvilinearElastic.userIncludes()


def add_constants(project):
    project.add_constant("PmlCellWidth", 1, "int")
    project.add_constant("PmlAlphaConstant", 1.5, "double")
    project.add_constant("PmlAlphaScalar", 0.0, "double")
    project.add_constant("PmlRelativeError", 0.001, "double")
    project.add_constant("PmlPower", 1, "int")
