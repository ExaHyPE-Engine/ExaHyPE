#include "Tracer.h"
#include "particles.h"


#include "peano4/utils/Loop.h"


tarch::la::Vector<Dimensions,int> toolbox::particles::mapParticleOntoVoxel(
  const peano4::datamanagement::CellMarker& marker,
  int voxelsPerAxis,
  const tarch::la::Vector<Dimensions,double>& particleX
) {
  toolbox::particles::addBibTeXEntry();

  tarch::la::Vector<Dimensions,int> result;

  assertion(voxelsPerAxis>0);
  double cartesianMeshH = marker.h()(0) / voxelsPerAxis;

  for (int d=0; d<Dimensions; d++) {
    double positionWithinPatch = particleX(d) - marker.getOffset()(d);
    result(d) = static_cast<int>( std::floor( positionWithinPatch / cartesianMeshH ));
    result(d) = std::max(0, result(d));
    result(d) = std::min(voxelsPerAxis-1, result(d));
  }

  return result;
}

tarch::la::Vector<Dimensions*2,int> toolbox::particles::mapParticleOntoVoxelwithFlags(
  const peano4::datamanagement::CellMarker& marker,
  int voxelsPerAxis,
  const tarch::la::Vector<Dimensions,double>& particleX
) {
  toolbox::particles::addBibTeXEntry();

  tarch::la::Vector<Dimensions*2,int> result;

  assertion(voxelsPerAxis>0);
  double cartesianMeshH = marker.h()(0) / voxelsPerAxis;

  for (int d=0; d<Dimensions; d++) {
    double positionWithinPatch = particleX(d) - marker.getOffset()(d);
    result(d) = static_cast<int>( std::floor( 2*positionWithinPatch / cartesianMeshH ));
    result(d+Dimensions)= result(d) % 2 == 0 ? -1.0 : 1.0;
    result(d) = result(d)/2;
    result(d) = std::max(0, result(d));
    result(d) = std::min(voxelsPerAxis-1, result(d));
  }

  return result;
}


tarch::la::Vector<Dimensions,double> toolbox::particles::explicitEulerWithoutInterpolation(
  const peano4::datamanagement::CellMarker&   marker,
  int                                         voxelsPerAxis,
  int                                         unknownsPerVoxel,
  const tarch::la::Vector<Dimensions,int>&    velocityIndices,
  double                                      timeStepSize,
  double*                                     voxelField,
  const tarch::la::Vector<Dimensions,double>& particleX
) {
  tarch::la::Vector<Dimensions,int> voxel = toolbox::particles::mapParticleOntoVoxel(marker,voxelsPerAxis,particleX);

  int voxelIndex = peano4::utils::dLinearised(voxel,voxelsPerAxis);
  double* Q      = voxelField + voxelIndex * unknownsPerVoxel;

  tarch::la::Vector<Dimensions,double> result = particleX;
  for (int d=0; d<Dimensions; d++) {
    result(d) += timeStepSize * Q[ velocityIndices(d) ];
    //result(d) += timeStepSize * Q[ 17 ];
  }

  return result;
}

tarch::la::Vector<Dimensions,double> toolbox::particles::LinearInterp(
  const peano4::datamanagement::CellMarker&   marker,
  int                                         voxelsPerAxis,
  int                                         unknownsPerVoxel,
  const tarch::la::Vector<Dimensions,int>&    velocityIndices,
  double                                      timeStepSize,
  double*                                     voxelField,
  const tarch::la::Vector<Dimensions,double>& particleX
) {
  tarch::la::Vector<Dimensions*2,int> voxel = toolbox::particles::mapParticleOntoVoxelwithFlags(marker,voxelsPerAxis,particleX);
  #if Dimensions==2
  int NumberofNeighbor=4;
  int interq=2;
  #else
  int NumberofNeighbor=8;
  int interq=3;
  #endif
  tarch::la::Vector<Dimensions,int> IndexForInterpolate[NumberofNeighbor];
  FindInterIndex(IndexForInterpolate,voxel,voxelsPerAxis);  
  double raw[NumberofNeighbor*Dimensions];
  for (int i=0;i<NumberofNeighbor;i++){
    int voxelIndex = peano4::utils::dLinearised(IndexForInterpolate[i],voxelsPerAxis); 
    for (int j=0;j<Dimensions;j++) {raw[i*Dimensions+j]=voxelField[voxelIndex * unknownsPerVoxel+velocityIndices(j)];}
  }

  tarch::la::Vector<Dimensions,double> velocity;
  velocity=Interpolation(IndexForInterpolate, raw, particleX, marker, voxelsPerAxis, interq);

  tarch::la::Vector<Dimensions,double> result = particleX;
  for (int d=0; d<Dimensions; d++) {
    //std::cout << velocity(d) << std::endl;
    result(d) += timeStepSize * velocity(d);
    //result(d) += timeStepSize * 0.02;
  }

  return result;
}

tarch::la::Vector<Dimensions,double> toolbox::particles::StaticPosition(
  const peano4::datamanagement::CellMarker&   marker,
  int                                         voxelsPerAxis,
  int                                         unknownsPerVoxel,
  const tarch::la::Vector<Dimensions,int>&    velocityIndices,
  double                                      timeStepSize,
  double*                                     voxelField,
  const tarch::la::Vector<Dimensions,double>& particleX
) {
  tarch::la::Vector<Dimensions,double> result = particleX;
  return result;
}

double toolbox::particles::ObLinearInterp(
  const peano4::datamanagement::CellMarker&   marker,
  int                                         voxelsPerAxis,
  int                                         unknownsPerVoxel,
  const int				      ObIndex,
  double*                                     voxelField,
  const tarch::la::Vector<Dimensions,double>& particleX
){
  tarch::la::Vector<Dimensions*2,int> voxel = toolbox::particles::mapParticleOntoVoxelwithFlags(marker,voxelsPerAxis,particleX);
  #if Dimensions==2
  int NumberofNeighbor=4;
  #else
  int NumberofNeighbor=8;
  #endif
  tarch::la::Vector<Dimensions,int> IndexForInterpolate[NumberofNeighbor];
  FindInterIndex(IndexForInterpolate,voxel,voxelsPerAxis);

  double raw[NumberofNeighbor*1];
  for (int i=0;i<NumberofNeighbor;i++){
    int voxelIndex = peano4::utils::dLinearised(IndexForInterpolate[i],voxelsPerAxis); 
    raw[i]=voxelField[voxelIndex * unknownsPerVoxel+ObIndex];
  }

  tarch::la::Vector<Dimensions,double> result;
  result=Interpolation(IndexForInterpolate, raw, particleX, marker, voxelsPerAxis, 1);

  return result(0);
}


