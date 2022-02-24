#include "Euler.h"

void exahype2::dg::explicitEulerOnCell(
    const double* __restrict__          					cellQin,
    const int                           					order,
    const int                           					unknowns,
    const int                           					auxiliaryVariables,
    const tarch::la::Vector<Dimensions,double>&    x,
    const double                                   dx,
    const double                                   t,
    const double                                   dt,
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F
    ) >   flux,
    std::function< void(
      const double * __restrict__                  Q,
      const double * __restrict__                  dQdx,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F
    ) >   nonconservativeProduct,
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double * __restrict__                        S
    ) >   source,
    std::function< void(
      const double * __restrict__                  Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,int>&     index,
      double                                       t,
      double * __restrict__                        S
    ) >   pointSource,
    const double* __restrict__	                   quadNodes,
    const double* __restrict__                     quadWeights,
    const double* __restrict__                     DerivativeOperator,
    const double* __restrict__                     MassMatrix,
    const double* __restrict__                     StiffnessMatrix,
    const double* __restrict__                     InvertedAndLumpedMassMatrix,
    double* __restrict__                           cellQout
) {

	const int nodesPerAxis = order+1;
	const int strideQ=order+unknowns;
	const int nodesPerCell = getNodesPerCell(nodesPerAxis);
	tarch::la::Vector<Dimensions,int> strides = getStrides(nodesPerAxis);

	const double coeff0 = dt/dx; //We assume that the cells are cubes, e.g. A/V = 1/dx

	double* fluxValues 	 = new double[unknowns]{0.0};
	double* sourceValues = new double[unknowns]{0.0};

	//which node's value is being computed
	for(int node=0; node<nodesPerCell; node++){

		//initialize to zero
		for(int var=0; var<strideQ; var++){
				cellQout[node*strideQ+var] = cellQin[node*strideQ+var];
		}

		tarch::la::Vector<Dimensions,int> index = getIndex(node, strides);


		// direction in which flux contributions are being computed
		for(int dim=0; dim<Dimensions; dim++){

			double coeff1 = coeff0 / quadWeights[index[dim]];

			// contributing node along this direction
			for(int i=0; i<nodesPerAxis; i++){

				// compute contributing node real position
				tarch::la::Vector<Dimensions,int> contributingNodeIndex = index;
				contributingNodeIndex[dim] = i;
				tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
					  dx,
					  contributingNodeIndex,
					  order,
					  quadNodes);

				flux(	&cellQin[node*strideQ],
							x,
							t,
							dim, //normal
							fluxValues);
				double coeff = coeff1 * StiffnessMatrix[i*nodesPerAxis+index[dim]]; //TODO: is this correct? I assume matrix is [receiving node, contributing node] but could be opposite
				for(int var=0; var<unknowns; var++){
						cellQout[node*strideQ+var] += coeff * fluxValues[var];
				} //var

			} //i
		} //dim


		//compute real node position
		tarch::la::Vector<Dimensions,double> position = getQuadraturePoint( x,
			  dx,
			  index,
			  order,
			  quadNodes);

		//compute source term contributions
		source( &cellQin[node*strideQ],
						position,
						t,
						sourceValues);

		for(int var=0; var<unknowns; var++){
			cellQout[node*strideQ+var] += dt * sourceValues[var];
		} //var

	} //node

	delete [] fluxValues;
	delete [] sourceValues;

}
