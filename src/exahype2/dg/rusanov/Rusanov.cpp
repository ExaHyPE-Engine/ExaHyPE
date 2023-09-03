#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"

#include "peano4/utils/Loop.h"
#include "Rusanov.h"

void exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ(
  [[maybe_unused]] ::exahype2::dg::Flux                         flux,
  [[maybe_unused]] ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
  [[maybe_unused]] ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  cellSize,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          order,
  [[maybe_unused]] int                                          unknowns,
  [[maybe_unused]] int                                          auxiliaryVariables,
  [[maybe_unused]] int                                          faceNumber,
  [[maybe_unused]] const double* __restrict__                   quadraturePoints,
  [[maybe_unused]] bool                                         evaluateFlux,
  [[maybe_unused]] bool                                         evaluateNonconservativeProduct,
  [[maybe_unused]] const double* __restrict__                   projectedValues,
  [[maybe_unused]] double* __restrict__                         solution
) {
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * cellSize(faceNumber%Dimensions);

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

    double leftEigenvalue = maxEigenvalue(
                    projectedValues + inputEnumerator(leftDoF,0),
                    x,
                    t,
                    dt,
                    faceNumber%Dimensions );

    double rightEigenvalue = maxEigenvalue(
                    projectedValues + inputEnumerator(rightDoF,0),
                    x,
                    t,
                    dt,
                    faceNumber%Dimensions );

    double maxEigenvalue = std::max(leftEigenvalue,rightEigenvalue);

    for(int var=0; var<unknowns; var++){
      solution[ outputEnumerator(leftDoF,var)  ] -= 0.5 * maxEigenvalue * deltaQ[var];
      solution[ outputEnumerator(rightDoF,var) ] -= 0.5 * maxEigenvalue * deltaQ[var];
    }

    if(evaluateFlux){
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
        solution[ outputEnumerator(leftDoF,var)  ] += 0.5 * (fluxValuesLeft[var]+fluxValuesRight[var]);
        solution[ outputEnumerator(rightDoF,var) ] += 0.5 * (fluxValuesLeft[var]+fluxValuesRight[var]);
      }
    }

    if(evaluateNonconservativeProduct){
      nonConservativeProduct(
          averageQ,
          deltaQ,
          x,
          t,
          dt,
          faceNumber%Dimensions, //normal
          ncpValues);

      for (int var=0; var<unknowns; var++) {
        solution[ outputEnumerator(leftDoF,var)  ] += 0.5 * ncpValues[var];
        solution[ outputEnumerator(rightDoF,var) ] -= 0.5 * ncpValues[var];
      }
    }
  }

  delete[] deltaQ;
  delete[] averageQ;
  delete[] fluxValuesLeft;
  delete[] fluxValuesRight;
  delete[] ncpValues;
}

void exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
  [[maybe_unused]] ::exahype2::dg::Flux                         flux,
  [[maybe_unused]] ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
  [[maybe_unused]] ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  cellSize,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          order,
  [[maybe_unused]] int                                          unknowns,
  [[maybe_unused]] int                                          auxiliaryVariables,
  [[maybe_unused]] int                                          faceNumber,
  [[maybe_unused]] const double* __restrict__                   quadraturePoints,
  [[maybe_unused]] bool                                         evaluateFlux,
  [[maybe_unused]] bool                                         evaluateNonconservativeProduct,
  [[maybe_unused]] const double* __restrict__                   projectedValues,
  [[maybe_unused]] double* __restrict__                         solution
) {
  assertionMsg(false, "not implemented yet");
}
