// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

// The matrices are stored in column major order. A Transposed suffix names the transpose of the
// matrix that carries the same name without the suffix.
template <typename T>
class DGMatrices final {
public:
  static void initDGMatrices();
  static void freeDGMatrices();

  // Reference element stiffness matrix. Kxi[i*n+j] holds
  // sum_k quadratureWeight[k] * dphi_k(x_i)/dx * phi_k(x_j), where phi are the Lagrange polynomials
  // of the quadrature points.
  inline static T* Kxi = nullptr;
  inline static T* KxiTransposed = nullptr;

  // Inverse of the difference between the element mass operator at x=1.0, phi_i(1.0)*phi_k(1.0),
  // and the reference element stiffness matrix. Equation (1.12) of DOI 10.1007/s10915-010-9426-6
  // defines the operator.
  inline static T* iK1Transposed = nullptr;

  // Derivatives of the basis functions at the quadrature points. dudx[i*n+j] holds the derivative
  // of basis function i at quadrature point j.
  inline static T* dudx = nullptr;

  // dudxTransposed[i*n+j] holds the derivative at quadrature point i of basis function j, so the
  // total derivative of the values a_j at quadrature point i is sum_j a_j * dudxTransposed[i*n+j].
  inline static T* dudxTransposed = nullptr;

  // Coefficients that project the basis polynomials onto the left face of the cell, so the values
  // a_i at the quadrature points x_i give the value sum_i a_i * FLCoeff[i] at x=0.
  inline static T* FLCoeff = nullptr;
  // Coefficients that project the basis polynomials onto the right face of the cell at x=1.
  inline static T* FRCoeff = nullptr;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
