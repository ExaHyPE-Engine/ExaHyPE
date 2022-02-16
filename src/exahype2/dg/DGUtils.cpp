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
	const double* const __restrict__    BasisFunctionValuesLeft,
	const double* const __restrict__    BasisFunctionValuesRight,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp
) {

	int nodesPerAxis = order+1;

	for(int row=0; row<nodesPerAxis; row++){

		//unrolled from rest of loop to initialize elements of faces
		for(int var=0; var<unknowns; var++){
			faceQLeft[(2*row+1)*unknowns+var] 	= BasisFunctionValuesLeft[0] * cellQ[(row*nodesPerAxis+0)*unknowns + var];
			faceQRight[2*row*unknowns+var]  		= BasisFunctionValuesRight[0] * cellQ[(row*nodesPerAxis+0)*unknowns + var];

			faceQBottom[(2*row+1)*unknowns+var] = BasisFunctionValuesLeft[0] * cellQ[(0*nodesPerAxis+row)*unknowns + var];
			faceQUp[2*row*unknowns+var] 				= BasisFunctionValuesLeft[0] * cellQ[(0*nodesPerAxis+row)*unknowns + var];
		} // var

		for(int node=1; node<nodesPerAxis; node++){
			for(int var=0; var<unknowns; var++){

				/*iterate over nodes from left to right; remember that in face elements are in order left1, then right1,
				 then left 2, then right 2 etc, and left face here is the face this cell is to the right of*/
				faceQLeft[(2*row+1)*unknowns+var] 	+= BasisFunctionValuesLeft[node] * cellQ[(row*nodesPerAxis+node)*unknowns + var];
				faceQRight[2*row*unknowns+var]  		+= BasisFunctionValuesRight[node] * cellQ[(row*nodesPerAxis+node)*unknowns + var];

				//iterate over nodes from lowest to highest
				faceQBottom[(2*row+1)*unknowns+var] += BasisFunctionValuesLeft[node] * cellQ[(node*nodesPerAxis+row)*unknowns + var];
				faceQUp[2*row*unknowns+var] 				+= BasisFunctionValuesLeft[node] * cellQ[(node*nodesPerAxis+row)*unknowns + var];
			} // var

		} // node
	} // row

}


void exahype2::dg::projectVolumetricDataOntoFaces(
  const double* __restrict__          cellQ,
  int                                 order,
  int                                 unknowns,
	const double* const __restrict__    BasisFunctionValuesLeft,
	const double* const __restrict__    BasisFunctionValuesRight,
  double* const __restrict__          faceQLeft,
  double* const __restrict__          faceQRight,
  double* const __restrict__          faceQBottom,
  double* const __restrict__          faceQUp,
  double* const __restrict__          faceQFront,
  double* const __restrict__          faceQBack
) {
  // @todo implement

	int nodesPerAxis = order+1;
	int nodesPerFace = nodesPerAxis*nodesPerAxis;

	for(int face=0; face<nodesPerAxis; face++){
		for(int row=0; row<nodesPerAxis; row++){

			//unrolled from rest of loop to initialize elements of faces
			for(int var=0; var<unknowns; var++){
				faceQLeft[(2*(face*nodesPerAxis+row)+1)*unknowns+var]   = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerFace+row*nodesPerAxis+0)*unknowns + var];
				faceQRight[2*(face*nodesPerAxis+row)*unknowns+var] 	 		= BasisFunctionValuesRight[0] * cellQ[(face*nodesPerFace+row*nodesPerAxis+0)*unknowns + var];

				faceQBottom[(2*(face*nodesPerAxis+row)+1)*unknowns+var] = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerFace+row+0*nodesPerAxis)*unknowns + var];
				faceQUp[2*(face*nodesPerAxis+row)*unknowns+var] 		 		= BasisFunctionValuesRight[0] * cellQ[(face*nodesPerFace+row+0*nodesPerAxis)*unknowns + var];

				faceQFront[(2*(face*nodesPerAxis+row)+1)*unknowns+var]  = BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerAxis+row+0*nodesPerFace)*unknowns + var];
				faceQBack[2*(face*nodesPerAxis+row)*unknowns+var]	   		= BasisFunctionValuesLeft[0] * cellQ[(face*nodesPerAxis+row+0*nodesPerFace)*unknowns + var];
			} // var

			for(int node=1; node<nodesPerAxis; node++){
				for(int var=0; var<unknowns; var++){

					/*iterate over nodes from left to right; remember that in face elements are in order left1, then right1,
					 then left 2, then right 2 etc, and left face here is the face this cell is to the right of*/
					faceQLeft[(2*(face*nodesPerAxis+row)+1)*unknowns+var]   += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerFace+row*nodesPerAxis+node)*unknowns + var];
					faceQRight[2*(face*nodesPerAxis+row)*unknowns+var] 	 		+= BasisFunctionValuesRight[node] * cellQ[(face*nodesPerFace+row*nodesPerAxis+node)*unknowns + var];

					//iterate over nodes from lowest to highest
					faceQBottom[(2*(face*nodesPerAxis+row)+1)*unknowns+var] += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerFace+row+node*nodesPerAxis)*unknowns + var];
					faceQUp[2*(face*nodesPerAxis+row)*unknowns+var] 		 		+= BasisFunctionValuesRight[node] * cellQ[(face*nodesPerFace+row+node*nodesPerAxis)*unknowns + var];

					//iterate over nodes from front to back
					faceQFront[(2*(face*nodesPerAxis+row)+1)*unknowns+var]  += BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerAxis+row+node*nodesPerFace)*unknowns + var];
					faceQBack[2*(face*nodesPerAxis+row)*unknowns+var]	   		+= BasisFunctionValuesLeft[node] * cellQ[(face*nodesPerAxis+row+node*nodesPerFace)*unknowns + var];

				} // var

			} // node
		} // row
	} // face

}
