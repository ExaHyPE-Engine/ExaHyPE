# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import CurvilinearElastic


def initial():
    return """
  // The damping of an axis grows as the power PmlPower of the distance into the layer of PmlCellWidth
  // cells at either domain face of the axis; the lower face of the axis PmlTop is a free surface
  // without layer.
  {
    constexpr int    NodesPerAxis       = DGOrder + 1;
    constexpr int    NumberOfData       = NumberOfUnknowns + NumberOfAuxiliaryVariables;
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
        auto&        damping  = Q[node * NumberOfData + Shortcuts::pmlDamping + axis];
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
    F[Shortcuts::pml + 9 * normal + axis] = F[Shortcuts::v + axis];
  }
"""


def ncp():
    return """
  const auto* const metric             = Q + Shortcuts::metricDerivative + normal * DIMENSIONS;
  const auto* const velocityJump       = deltaQ + Shortcuts::v;
  const auto        density            = Q[Shortcuts::rho];
  const auto        damping            = Q[Shortcuts::pmlDamping + normal];
  const auto        shearModulus       = density * Q[Shortcuts::cs] * Q[Shortcuts::cs];
  const auto        firstLameParameter = density * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * shearModulus;
  const auto        lameTimesDivergence = firstLameParameter
    * (-metric[0] * velocityJump[0] - metric[1] * velocityJump[1] - metric[2] * velocityJump[2]);
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    BTimesDeltaQ[Shortcuts::sigma + axis] = -2 * shearModulus * metric[axis] * velocityJump[axis] + lameTimesDivergence;
  }
  BTimesDeltaQ[Shortcuts::sigma + 3] = -shearModulus * (metric[1] * velocityJump[0] + metric[0] * velocityJump[1]);
  BTimesDeltaQ[Shortcuts::sigma + 4] = -shearModulus * (metric[2] * velocityJump[0] + metric[0] * velocityJump[2]);
  BTimesDeltaQ[Shortcuts::sigma + 5] = -shearModulus * (metric[2] * velocityJump[1] + metric[1] * velocityJump[2]);

  auto* const layer = BTimesDeltaQ + Shortcuts::pml + 9 * normal + Shortcuts::sigma;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    layer[axis] = -damping * metric[axis] * velocityJump[axis];
  }
  layer[3] = -damping * (metric[1] * velocityJump[0] + metric[0] * velocityJump[1]);
  layer[4] = -damping * (metric[2] * velocityJump[0] + metric[0] * velocityJump[2]);
  layer[5] = -damping * (metric[2] * velocityJump[1] + metric[1] * velocityJump[2]);
"""


def algebraic_source():
    return """
  const auto density             = Q[Shortcuts::rho];
  const auto shearModulus        = density * Q[Shortcuts::cs] * Q[Shortcuts::cs];
  const auto firstLameParameter  = density * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * shearModulus;
  const auto longitudinalModulus = 2 * shearModulus + firstLameParameter;
  const auto inverseDensity      = 1 / density;
  // The auxiliary variables of the layer hold nine entries per axis, the velocity entries first.
  const auto layer = [Q](const int axis, const int entry) { return Q[Shortcuts::pml + 9 * axis + entry]; };

  for (int component = 0; component < DIMENSIONS; component++) {
    S[Shortcuts::v + component] = -inverseDensity * (layer(0, component) + layer(1, component) + layer(2, component));
  }
  S[Shortcuts::sigma + 0] = -(longitudinalModulus * layer(0, 3) + firstLameParameter * (layer(0, 4) + layer(0, 5))
                            + longitudinalModulus * layer(1, 3) + firstLameParameter * (layer(1, 4) + layer(1, 5))
                            + longitudinalModulus * layer(2, 3) + firstLameParameter * (layer(2, 4) + layer(2, 5)));
  S[Shortcuts::sigma + 1] = -(longitudinalModulus * layer(0, 4) + firstLameParameter * (layer(0, 3) + layer(0, 5))
                            + longitudinalModulus * layer(1, 4) + firstLameParameter * (layer(1, 3) + layer(1, 5))
                            + longitudinalModulus * layer(2, 4) + firstLameParameter * (layer(2, 3) + layer(2, 5)));
  S[Shortcuts::sigma + 2] = -(longitudinalModulus * layer(0, 5) + firstLameParameter * (layer(0, 3) + layer(0, 4))
                            + longitudinalModulus * layer(1, 5) + firstLameParameter * (layer(1, 3) + layer(1, 4))
                            + longitudinalModulus * layer(2, 5) + firstLameParameter * (layer(2, 3) + layer(2, 4)));
  for (int shearComponent = 3; shearComponent < 6; shearComponent++) {
    S[Shortcuts::sigma + shearComponent] =
      -shearModulus * (layer(0, 3 + shearComponent) + layer(1, 3 + shearComponent) + layer(2, 3 + shearComponent));
  }
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto damping    = Q[Shortcuts::pmlDamping + axis];
    const auto relaxation = PmlAlphaConstant + PmlAlphaScalar * damping;
    for (int entry = 0; entry < 9; entry++) {
      S[Shortcuts::pml + 9 * axis + entry] = -(damping + relaxation) * layer(axis, entry);
    }
  }
"""


def multiplyMaterialParameterMatrix():
    return """
  const auto jacobian                    = Q[Shortcuts::jacobian];
  const auto inverseDensityTimesJacobian = 1 / (Q[Shortcuts::rho] * jacobian);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    rhs[Shortcuts::v + axis] = inverseDensityTimesJacobian * rhs[Shortcuts::v + axis];
  }
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const auto damping = Q[Shortcuts::pmlDamping + axis];
    for (int component = 0; component < DIMENSIONS; component++) {
      rhs[Shortcuts::pml + 9 * axis + component] = damping / jacobian * rhs[Shortcuts::pml + 9 * axis + component];
    }
  }
"""


def riemann_solver():
    return CurvilinearElastic.riemann_solver()


def userIncludes():
    return CurvilinearElastic.userIncludes()


def abstractDeclarations():
    return """
protected:
  static constexpr int    PmlCellWidth     = 1;
  static constexpr double PmlAlphaConstant = 1.5;
  static constexpr double PmlAlphaScalar   = 0.0;
  static constexpr double PmlRelativeError = 0.001;
  static constexpr int    PmlPower         = 1;
"""
