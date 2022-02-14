#include "DGUtils.h"
#include "tarch/Assertions.h"
#include "tarch/la/la.h"


tarch::la::Vector<2,double>  exahype2::dg::getQuadraturePoint(
  const tarch::la::Vector<2,double>&  cellCentre,
  const tarch::la::Vector<2,double>&  cellSize,
  const tarch::la::Vector<2,int>&     index,
  int                                 polynomialOrder,
  const double* __restrict__          quadraturePoints
) {
  tarch::la::Vector<2,double> result;

  result(0) = quadraturePoints[index(0)];
  result(1) = quadraturePoints[index(1)];

  return tarch::la::multiplyComponents(result,cellSize) + cellCentre - 0.5*cellSize;
}


tarch::la::Vector<3,double>  exahype2::dg::getQuadraturePoint(
  const tarch::la::Vector<3,double>&  cellCentre,
  const tarch::la::Vector<3,double>&  cellSize,
  const tarch::la::Vector<3,int>&     index,
  int                                 polynomialOrder,
  const double* __restrict__          quadraturePoints
) {
  tarch::la::Vector<3,double> result;

  result(0) = quadraturePoints[index(0)];
  result(1) = quadraturePoints[index(1)];
  result(2) = quadraturePoints[index(2)];

  return tarch::la::multiplyComponents(result,cellSize) + cellCentre - 0.5*cellSize;
}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* __restrict__          BasisFunctionValuesLeft,
  const double* __restrict__          BasisFunctionValuesRight,
  const double* __restrict__          faceQLeft,
  const double* __restrict__          faceQRight,
  const double* __restrict__          faceQBottom,
  const double* __restrict__          faceQUp
) {
  // @todo implement
}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
  const double* __restrict__          BasisFunctionValuesLeft,
  const double* __restrict__          BasisFunctionValuesRight,
  const double* __restrict__          faceQLeft,
  const double* __restrict__          faceQRight,
  const double* __restrict__          faceQBottom,
  const double* __restrict__          faceQUp,
  const double* __restrict__          faceQFront,
  const double* __restrict__          faceQBack
) {
  // @todo implement
}
