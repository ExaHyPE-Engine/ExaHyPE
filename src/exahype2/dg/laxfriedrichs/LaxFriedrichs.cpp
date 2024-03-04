#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"
#include "LaxFriedrichs.h"

#include "peano4/utils/Loop.h"


void exahype2::dg::laxfriedrichs::solveRiemannProblem_pointwise_in_situ(
  ::exahype2::dg::Flux                         flux,
  ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  cellSize,
  double                                       t,
  double                                       dt,
  int                                          order,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  int                                          faceNumber,
  const double* __restrict__                   quadraturePoints,
  bool                                         useFlux,
  bool                                         useNcp,
  const double* __restrict__                   projectedValues,
  double* __restrict__                         solution
) {
  int normal = faceNumber%Dimensions;
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(normal) += 0.5 * cellSize(normal);

  exahype2::enumerator::FaceAoSLexicographicEnumerator inputEnumerator (faceNumber,order+1,1,unknowns,auxiliaryVariables);
  exahype2::enumerator::FaceAoSLexicographicEnumerator outputEnumerator(faceNumber,order+1,1,unknowns,0);

  double* deltaQ          = new double[unknowns+auxiliaryVariables];
  double* averageQ        = new double[unknowns+auxiliaryVariables];
  double* fluxValuesLeft  = new double[unknowns];
  double* fluxValuesRight = new double[unknowns];
  double* ncpValues       = new double[unknowns];

  dfore(dof,order+1,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,double> x;
    for (int d=0; d<Dimensions; d++) {
      x(d)  = faceOffset(d);
      x(d) += (d==faceNumber % Dimensions) ? 0.0 : quadraturePoints[dof(d)] * cellSize(d);
    }

    tarch::la::Vector<Dimensions,int>    leftDoF  = dof;
    tarch::la::Vector<Dimensions,int>    rightDoF = dof;

    leftDoF(faceNumber % Dimensions)  = 0;
    rightDoF(faceNumber % Dimensions) = 1;

    //initialize solution to 0
    for(int var=0; var<unknowns; var++){
      solution[ outputEnumerator(leftDoF,var)  ] = 0.0;
      solution[ outputEnumerator(rightDoF,var) ] = 0.0;
    }

    for(int var=0; var<unknowns+auxiliaryVariables; var++){
      deltaQ[ var ] = projectedValues[ inputEnumerator(rightDoF,var) ]
                    - projectedValues[ inputEnumerator(leftDoF,var) ];
      averageQ[ var ] = 0.5 * projectedValues[ inputEnumerator(rightDoF,var) ]
                      + 0.5 * projectedValues[ inputEnumerator(leftDoF,var) ];
    }

    for(int var=0; var<unknowns; var++){
      solution[ outputEnumerator(leftDoF,var)  ] = + cellSize(normal)/2.0/dt * deltaQ[var];
      solution[ outputEnumerator(rightDoF,var) ] = - cellSize(normal)/2.0/dt * deltaQ[var];
    }

    if(useFlux){
      flux( projectedValues + inputEnumerator( leftDoF, 0 ),
            x,
            t,
            dt,
            faceNumber%Dimensions, //normal
            fluxValuesLeft);
      flux( projectedValues + inputEnumerator (rightDoF, 0),
            x,
            t,
            dt,
            faceNumber%Dimensions, //normal
            fluxValuesRight);

      for(int var=0; var<unknowns; var++){
        solution[ outputEnumerator(leftDoF,var)  ] -= 0.5 * (fluxValuesLeft[var]+fluxValuesRight[var]);
        solution[ outputEnumerator(rightDoF,var) ] += 0.5 * (fluxValuesLeft[var]+fluxValuesRight[var]);
      }
    }

    if(useNcp){
      nonConservativeProduct(
          averageQ,
          deltaQ,
          x, //TODO: add positioning
          t,
          dt,
          faceNumber%Dimensions, //normal
          ncpValues);

      for(int var=0; var<unknowns; var++){
        solution[ outputEnumerator(leftDoF,var)  ] += 0.5 * deltaQ[var] * ncpValues[var];
        solution[ outputEnumerator(rightDoF,var) ] += 0.5 * deltaQ[var] * ncpValues[var];
      }
    }


  }

  delete[] deltaQ;
  delete[] averageQ;
  delete[] fluxValuesLeft;
  delete[] fluxValuesRight;
  delete[] ncpValues;
}
