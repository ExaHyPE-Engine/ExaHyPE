#include "ElementMatrix.h"
#include "StencilFactory.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"



toolbox::finiteelements::ElementWiseAssemblyMatrix
toolbox::finiteelements::getElementWiseAssemblyMatrix( const toolbox::finiteelements::Stencil& stencil ) {
  /**
   * @todo Die Abbildung sollte man im Konstruktor einmal bauen und dann hier
   * nur noch anwenden. Deshalb ist das Ding ja eine Methode und kein
   * statisches Ding.
   */
  ElementWiseAssemblyMatrix result;

  dfor2(j)
  dfor2(i)
    tarch::la::Vector<Dimensions,int> stencilEntry;
    double    commonFacesPowerTwo = 1.0;
    for (int d=0; d<Dimensions; d++) {
      stencilEntry(d) = i(d)-j(d)+1;
      if (i(d)==j(d)) commonFacesPowerTwo *= 2.0;
    }
    result(jScalar,iScalar) = stencil(peano4::utils::dLinearised(stencilEntry,3)) / commonFacesPowerTwo;
  enddforx
  enddforx

  return result;
}


toolbox::finiteelements::ComplexElementWiseAssemblyMatrix
toolbox::finiteelements::getElementWiseAssemblyMatrix( const toolbox::finiteelements::ComplexStencil& complexStencil ) {
  /**
   * @todo Die Abbildung sollte man im Konstruktor einmal bauen und dann hier
   * nur noch anwenden. Deshalb ist das Ding ja eine Methode und kein
   * statisches Ding.
   */
  ComplexElementWiseAssemblyMatrix result;

  dfor2(j)
  dfor2(i)
    tarch::la::Vector<Dimensions,int> stencilEntry;
    double    commonFacesPowerTwo = 1.0;
    for (int d=0; d<Dimensions; d++) {
      stencilEntry(d) = i(d)-j(d)+1;
      if (i(d)==j(d)) commonFacesPowerTwo *= 2.0;
    }
    result(jScalar,iScalar) = complexStencil(peano4::utils::dLinearised(stencilEntry,3)) / commonFacesPowerTwo;
  enddforx
  enddforx

  return result;
}


double toolbox::finiteelements::getDiagonalElement( const ElementWiseAssemblyMatrix& matrix ) {
  return matrix(0,0) * TwoPowerD;
}


double toolbox::finiteelements::getDiagonalElement( const Stencil& stencil ) {
  return stencil(ThreePowerD / 2);
}


toolbox::finiteelements::Stencil
toolbox::finiteelements::reconstructUniformStencilFragments(const ElementWiseAssemblyMatrix& matrix ) {
  toolbox::finiteelements::Stencil result(0.0);

  dfor2(j)
    dfor2(k)
      const int stencilEntry = mapElementMatrixEntryOntoStencilEntry(jScalar,kScalar);
      result(stencilEntry) += matrix(jScalar,kScalar);
    enddforx
  enddforx

  return result;
}


tarch::la::Vector<TwoPowerD * ThreePowerD, double> toolbox::finiteelements::reconstructStencilFragments(const ElementWiseAssemblyMatrix& matrix ) {
  tarch::la::Vector<TwoPowerD * ThreePowerD, double> result(0.0);

  dfor2(j)
    dfor2(k)
      const int stencilEntry = mapElementMatrixEntryOntoStencilEntry(jScalar,kScalar);
      assertionEquals1(result(jScalar*ThreePowerD + stencilEntry),0.0,result);
      result(jScalar*ThreePowerD + stencilEntry) = matrix(jScalar,kScalar);
    enddforx
  enddforx

  return result;
}


toolbox::finiteelements::ElementWiseAssemblyMatrix
toolbox::finiteelements::getElementWiseAssemblyMatrix( const VectorOfStencils& vectorOfStencils ) {
  ElementWiseAssemblyMatrix result;

  dfor2(j)
  dfor2(i)
    const int                         stencilOffset = jScalar * ThreePowerD;
    tarch::la::Vector<Dimensions,int> stencilEntry;
    double                            commonFacesPowerTwo = 1.0;
    for (int d=0; d<Dimensions; d++) {
      if (i(d)==j(d)) {
        stencilEntry(d)      = 1;
        commonFacesPowerTwo *= 2.0;
      }
      else if (i(d)<j(d)) {
        stencilEntry(d)      = 0;
      }
      else {
        stencilEntry(d)      = 2;
      }
    }
    const int vectorOfStencilIndex = stencilOffset+peano4::utils::dLinearised(stencilEntry,3);
    result(jScalar,iScalar) = vectorOfStencils( vectorOfStencilIndex ) / commonFacesPowerTwo;
  enddforx
  enddforx

  #ifdef Dim2
  assertionNumericalEquals2( result(0,0), vectorOfStencils(0*ThreePowerD+4)/4.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(0,1), vectorOfStencils(0*ThreePowerD+5)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(0,2), vectorOfStencils(0*ThreePowerD+7)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(0,3), vectorOfStencils(0*ThreePowerD+8)/1.0, result, vectorOfStencils );

  assertionNumericalEquals2( result(1,0), vectorOfStencils(1*ThreePowerD+3)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(1,1), vectorOfStencils(1*ThreePowerD+4)/4.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(1,2), vectorOfStencils(1*ThreePowerD+6)/1.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(1,3), vectorOfStencils(1*ThreePowerD+7)/2.0, result, vectorOfStencils );

  assertionNumericalEquals2( result(2,0), vectorOfStencils(2*ThreePowerD+1)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(2,1), vectorOfStencils(2*ThreePowerD+2)/1.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(2,2), vectorOfStencils(2*ThreePowerD+4)/4.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(2,3), vectorOfStencils(2*ThreePowerD+5)/2.0, result, vectorOfStencils );

  assertionNumericalEquals2( result(3,0), vectorOfStencils(3*ThreePowerD+0)/1.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(3,1), vectorOfStencils(3*ThreePowerD+1)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(3,2), vectorOfStencils(3*ThreePowerD+3)/2.0, result, vectorOfStencils );
  assertionNumericalEquals2( result(3,3), vectorOfStencils(3*ThreePowerD+4)/4.0, result, vectorOfStencils );
  #endif

  return result;
}


toolbox::finiteelements::ElementWiseAssemblyMatrix toolbox::finiteelements::getElementWiseAssemblyMatrix( const VectorOfStencils& vectorOfStencils, const std::bitset<ThreePowerD>& cellIsInside ) {
  assertion2( cellIsInside[ThreePowerD/2], vectorOfStencils, cellIsInside );

  ElementWiseAssemblyMatrix result;

  dfor2(j)
  dfor2(i)
    const int                         stencilOffset = jScalar * ThreePowerD;
    tarch::la::Vector<Dimensions,int> stencilEntry;
    int                               adjacentInnerCells = 0;

    dfor2(jj)
    dfor2(ii)
      if (jj+j==ii+i) {
        adjacentInnerCells += ( cellIsInside[peano4::utils::dLinearised(jj+j,3)] ) ? 1 : 0;
      }
    enddforx
    enddforx

    for (int d=0; d<Dimensions; d++) {
      if (i(d)==j(d)) {
        stencilEntry(d)      = 1;
      }
      else if (i(d)<j(d)) {
        stencilEntry(d)      = 0;
      }
      else {
        stencilEntry(d)      = 2;
      }
    }
    const int vectorOfStencilIndex = stencilOffset+peano4::utils::dLinearised(stencilEntry,3);
    result(jScalar,iScalar) = vectorOfStencils( vectorOfStencilIndex ) / static_cast<double>(adjacentInnerCells);
  enddforx
  enddforx

  assertion4( !cellIsInside.all() || tarch::la::equals(result,getElementWiseAssemblyMatrix(vectorOfStencils) ), vectorOfStencils, cellIsInside, result, getElementWiseAssemblyMatrix(vectorOfStencils) );

  return result;
}


toolbox::finiteelements::ComplexElementWiseAssemblyMatrix toolbox::finiteelements::getElementWiseAssemblyMatrix( const VectorOfComplexStencils& vectorOfStencils, const std::bitset<ThreePowerD>& cellIsInside ) {
  assertionMsg(false, "not implemented yet" );
  return toolbox::finiteelements::ComplexElementWiseAssemblyMatrix();
}


int toolbox::finiteelements::mapElementMatrixEntryOntoStencilEntry(
  const tarch::la::Vector<Dimensions,int>&  row,
  const tarch::la::Vector<Dimensions,int>&  col
) {
  tarch::la::Vector<Dimensions,int>  stencilEntry;

  for (int d=0; d<Dimensions; d++) {
    if (col(d)>row(d)) {
      stencilEntry(d)=2;
    }
    else if (col(d)<row(d)) {
      stencilEntry(d)=0;
    }
    else {
      stencilEntry(d)=1;
    }
  }

  return getStencilEntryLinearisedIndex(stencilEntry);
}


int toolbox::finiteelements::mapElementMatrixEntryOntoStencilEntry(int row, int col) {
  assertion( row>=0 );
  assertion( row<TwoPowerD );
  assertion( col>=0 );
  assertion( col<TwoPowerD );
  #if defined(Dim2)
  static const int result[] = {4,5,7,8,3,4,6,7,1,2,4,5,0,1,3,4};
  int thisResult = result[row*4+col];

  #ifdef Asserts
  tarch::la::Vector<Dimensions,int>  toIndex   = peano::utils::dDelinearised(row,2);
  tarch::la::Vector<Dimensions,int>  fromIndex = peano::utils::dDelinearised(col,2);
  assertionEquals2( thisResult, mapElementMatrixEntryOntoStencilEntry(toIndex,fromIndex), row, col );
  #endif

  return thisResult;
  #else
  tarch::la::Vector<Dimensions,int>  toIndex   = peano4::utils::dDelinearised(row,2);
  tarch::la::Vector<Dimensions,int>  fromIndex = peano4::utils::dDelinearised(col,2);
  return mapElementMatrixEntryOntoStencilEntry(toIndex,fromIndex);
  #endif
}


toolbox::finiteelements::ComplexElementWiseAssemblyMatrix
toolbox::finiteelements::getElementWiseAssemblyMatrix( const VectorOfComplexStencils& vectorOfComplexStencils ) {
  ComplexElementWiseAssemblyMatrix result;

  dfor2(j)
  dfor2(i)
    const int                         stencilOffset = jScalar * ThreePowerD;
    tarch::la::Vector<Dimensions,int> stencilEntry;
    double                            commonFacesPowerTwo = 1.0;
    for (int d=0; d<Dimensions; d++) {
      if (i(d)==j(d)) {
        stencilEntry(d)      = 1;
        commonFacesPowerTwo *= 2.0;
      }
      else if (i(d)<j(d)) {
        stencilEntry(d)      = 0;
      }
      else {
        stencilEntry(d)      = 2;
      }
    }
    const int vectorOfComplexStencilIndex = stencilOffset+peano4::utils::dLinearised(stencilEntry,3);
    result(jScalar,iScalar) = vectorOfComplexStencils( vectorOfComplexStencilIndex ) / commonFacesPowerTwo;
  enddforx
  enddforx

  #ifdef Dim2
  assertionNumericalEquals2( result(0,0), vectorOfComplexStencils(0*ThreePowerD+4)/4.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(0,1), vectorOfComplexStencils(0*ThreePowerD+5)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(0,2), vectorOfComplexStencils(0*ThreePowerD+7)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(0,3), vectorOfComplexStencils(0*ThreePowerD+8)/1.0, result, vectorOfComplexStencils );

  assertionNumericalEquals2( result(1,0), vectorOfComplexStencils(1*ThreePowerD+3)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(1,1), vectorOfComplexStencils(1*ThreePowerD+4)/4.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(1,2), vectorOfComplexStencils(1*ThreePowerD+6)/1.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(1,3), vectorOfComplexStencils(1*ThreePowerD+7)/2.0, result, vectorOfComplexStencils );

  assertionNumericalEquals2( result(2,0), vectorOfComplexStencils(2*ThreePowerD+1)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(2,1), vectorOfComplexStencils(2*ThreePowerD+2)/1.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(2,2), vectorOfComplexStencils(2*ThreePowerD+4)/4.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(2,3), vectorOfComplexStencils(2*ThreePowerD+5)/2.0, result, vectorOfComplexStencils );

  assertionNumericalEquals2( result(3,0), vectorOfComplexStencils(3*ThreePowerD+0)/1.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(3,1), vectorOfComplexStencils(3*ThreePowerD+1)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(3,2), vectorOfComplexStencils(3*ThreePowerD+3)/2.0, result, vectorOfComplexStencils );
  assertionNumericalEquals2( result(3,3), vectorOfComplexStencils(3*ThreePowerD+4)/4.0, result, vectorOfComplexStencils );
/*
  result = stencil0[4]/4.0, stencil0[5]/2.0, stencil0[7]/2.0, stencil0[8]/1.0,
           stencil1[3]/2.0, stencil1[4]/4.0, stencil1[6]/1.0, stencil1[7]/2.0,
           stencil2[1]/2.0, stencil2[2]/1.0, stencil2[4]/4.0, stencil2[5]/2.0,
           stencil3[0]/1.0, stencil3[1]/2.0, stencil3[3]/2.0, stencil3[4]/4.0;
*/
  #endif

  return result;
}


toolbox::finiteelements::ElementWiseAssemblyMatrix toolbox::finiteelements::hierarchicalTransform(
  const ElementWiseAssemblyMatrix&              matrix,
  const tarch::la::Vector<Dimensions,double>&   h,
  double                                        weight
) {
  static toolbox::finiteelements::ElementWiseAssemblyMatrix referenceStiffnessMatrix =
    toolbox::finiteelements::getElementWiseAssemblyMatrix(
      toolbox::finiteelements::getLaplacian(h)
    );

  toolbox::finiteelements::ElementWiseAssemblyMatrix result;
  for (int row=0; row<TwoPowerD; row++)
  for (int col=0; col<TwoPowerD; col++) {
    result(row,col) = matrix(row,col) - weight * referenceStiffnessMatrix(row,col);
  }

  return result;
}


toolbox::finiteelements::ElementWiseAssemblyMatrix toolbox::finiteelements::inverseHierarchicalTransform(
  const ElementWiseAssemblyMatrix&              matrix,
  const tarch::la::Vector<Dimensions,double>&   h,
  double weight
) {
  return hierarchicalTransform(matrix,h,-weight);
}
