#include "RusanovNonlinearAoS.h"

#include "KernelUtils.h"

GPUCallableMethod void exahype2::aderdg::rusanovNonlinear_maxEigenvalue_body_AoS(
    std::function< double(
      double * __restrict__                        Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t
    ) >                                         maxAbsoluteEigenvalue,
    double&                                     smax,
    const double * __restrict__                 QLR[2],
    const double                                t,
    const double                                dt,
    const tarch::la::Vector<Dimensions,double>& faceCentre,
    const double                                dx,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   direction,
    const int                                   linearisedIndexFace) {
  const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face) , (t,x,z,face), (t,x,y,face)
    delineariseIndex(linearisedIndexFace,getStrides(nodesPerAxis)); 
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double t = coords[0];

  const int lr = indexQFace[Dimensions];
 
  // hyperbolic eigenvalues
  *smax = std::max( smax, maxAbsoluteEigenvalue( QLR[lr][ linearisedIndexFace*strideQ ], x, t, direction ) );
}

GPUCallableMethod void exahype2::aderdg::rusanovNonlinear_riemannFlux_body_AoS(
    std::function< void(
      double * __restrict__                        Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) >                                         flux,
    double * __restrict__                       FLOut,
    double * __restrict__                       FROut,
    const double * __restrict__                 QLIn, 
    const double * __restrict__                 QRIn, 
    double * __restrict__                       FLAux,
    double * __restrict__                       FRAux,
    const double&                               smax,
    const double                                t,
    const double                                dt,
    const tarch::la::Vector<Dimensions,double>& faceCentre,
    const double                                dx,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   direction,
    const int                                   linearisedIndexFace) {
  const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face=0) , (t,x,z,face=0), (t,x,y,face=0)
    delineariseIndex(linearisedIndexFace,getStrides(nodesPerAxis)); 
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double t = coords[0];

  // time-averaged flux FL
  for (int a = 0; a < nodesPerAxis; a++) { // time integration
    const int offsetQ = (linearisedIndexFace*nodesPerAxis + a)*strideQ;
    const double coeff1 = weights[a] * 0.5;
    const double coeff2 = coeff1 * smax;
    
    flux( QL[ offsetQ + var ] x, t, direction, FLAux ); 
    flux( QR[ offsetQ + var ] x, t, direction, FRAux );
    for (int var = 0; var < unknowns; var++) {
      FL[ linearisedIndexFace*strideF + var ] += coeff1 * (FRAux[ var ] + FLAux[ var ]);
    }
    for (int var = 0; var < unknowns; var++) {
      FL[ linearisedIndexFace*strideF + var ] -= coeff2 * (QR[ offsetQ + var ] - QL[ offsetQ + var ]);
    }
  }
  // copy FL to FR
  for (int var = 0; var < unknowns; var++) {
    FR[ linearisedIndexFace*strideF + var ] = FL[ linearisedIndexFace*strideF + var ]; 
  }
}

GPUCallableMethod void exahype2::aderdg::rusanovNonlinear_addNcpContributionsToRiemannFlux_body_AoS(
    std::function< void(
      double * __restrict__                       Q,
      double                                      gradQ[][Dimensions],
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       BgradQ
    ) >                                         nonconservativeProduct,
    double * __restrict__                       FLOut,
    double * __restrict__                       FROut,
    double * __restrict__                       QAvgAux,
    double * __restrict__                       gradQAux,
    double * __restrict__                       SAux,
    const double * __restrict__                 QLIn, 
    const double * __restrict__                 QRIn, 
    const double                                smax,
    const double                                t,
    const double                                dt,
    const tarch::la::Vector<Dimensions,double>& faceCentre,
    const double                                dx,
    const int                                   nodesPerAxis,
    const int                                   unknowns,
    const int                                   strideQ,
    const int                                   direction,
    const int                                   linearisedIndexFace) {
  const tarch::la::Vector<Dimensions+1,int> indexQFace = // (t,y,z,face=0) , (t,x,z,face=0), (t,x,y,face=0)
    delineariseIndex(linearisedIndexFace,getStrides(nodesPerAxis)); 
  const tarch::la::Vector<Dimensions+1,double> coords = 
    getCoordinatesOnFace(indexQFace,faceCentre,direction,dx,t,dt,nodes);
  const tarch::la::Vector<Dimensions,double> x(&coords[1]);
  const double t = coords[0];

  for (int it = 0; it < nodesPerAxis; it++) { // time integration
    const int offsetQ  = (linearisedIndexFace*nodesPerAxis + it)*strideQ;
    
    for (int var=0; var < strideQ; var++) {
      gradQAux[ Dimensions*var + direction ] = QRIn[ offsetQ + var ] - QLIn[ offsetQ + var ];
    }
    
    for (int var=0; var < strideQ; var++) {
      QAvgAux[var] = 0.5 * (QRIn[ offsetQ + var ] + QLIn[ offsetQ + var ]);
    }
      
    nonConservativeProduct(QAvgAux, (double (*)[Dimensions]) gradQAux, x, t, coords, SAux);

    const double coeff = weights[it] * 0.5;
    for (int var = 0; var < unknowns; var++) {
      FROut[ linearisedIndexFace*strideQ + var ] -= coeff * SAux[var];
    }
    for (int var = 0; var < unknowns; var++) {
      FLOut[ linearisedIndexFace*strideQ + var ] += coeff * SAux[var];
    }
  }
}
