// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

// All matrices are stored as column major array, _T denote a transposed.
template <typename T>
class DGMatrices {
public:
  static void initDGMatrices();
  static void freeDGMatrices();

  // Reference element stiffness matrix, Kxi[i*n+j] contains:
  // sum_k quadWeight[k] * \partial_x phi_k(x_i)* phi_k(x_j),
  // where phi are the lagrange-polynomials for the given quadrature points
  inline static T* Kxi = nullptr;
  inline static T* Kxi_T = nullptr; // Transpose of Kxi

  // Inverse of the difference between the element mass operator evaluated at x=1.0
  // ( phi_i[x=1.0]*phi_k[x=1.0] and the reference element stiffness matrix
  // used in nonlinear FSTPVI to integrate the predictor in time in each Picard iteration
  // stems e.g. from (1.12) in DOI 10.1007/s10915-010-9426-6
  inline static T* iK1_T = nullptr;

  // Contains the derivatives of the basis functions at the quadrature points
  // i.e. dudx[i*n+j] contains the derivative of basis function i at quad point j
  inline static T* dudx = nullptr;

  // Contains the derivatives at the quadrature points of the basis functions
  // i.e. dudx_T[i*n+j] contains the derivative at quad point i of basis function j
  // Therefore in order to compute the total derivative at quadpoint i, given values
  // a_j at the different quadrature points requires sum_j{ a_j * dudx_T[i*n+j] }
  inline static T* dudx_T = nullptr;

  // Contains the coefficients for the projection of the basis polynomials to the "left"
  // (i.e. negative) edge of the cell. Thus given a_i the values at the quadrature points
  // x=x_i, then the value at x=0 would be sum_i{ a_i * FLCoeff[i] }
  inline static T* FLCoeff = nullptr;
  // Same thing, but on the positive edge of the cell (so x=1 for the example above)
  inline static T* FRCoeff = nullptr;

  // Contains the coefficients for the projection of the basis polynomials to nodes of
  // subcells of the reference element. Thus given a_i the values at the quadrature points
  // x=x_i, then the value at point i/3 + x_j/3 (i.e. a Peano subcell, with 1/3 the size),
  // would be sum_k{ a_k * FineGridProjector1d[i][j*n + k] }
  inline static T** fineGridProjector1d = nullptr;
  // Equivalent operator for restriction
  inline static T** fineGridProjector1d_T_weighted = nullptr; // [k][i*nDof+j] = fineGridProjector1d[k][j*nDof+i] * weight[j] / weight[i] / 3.0
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
