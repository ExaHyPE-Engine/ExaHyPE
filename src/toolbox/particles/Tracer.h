// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLES_TRACER_
#define _TOOLBOX_PARTICLES_TRACER_


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


namespace toolbox {
  namespace particles {
    /**
     * Assume that we have a particle suspended in a cell. The cell hosts a
     * regular Cartesian mesh. The routine computes the correct voxel.
     */
    tarch::la::Vector<Dimensions,int> mapParticleOntoVoxel(
      const peano4::datamanagement::CellMarker& marker,
      int voxelsPerAxis,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

    /**
     * Assume that we have a particle suspended in a cell. The cell hosts a
     * regular Cartesian mesh. The routine computes the correct voxel and
     * provides the flags indicating the cloest 8 cells.
     */
    tarch::la::Vector<Dimensions*2,int> mapParticleOntoVoxelwithFlags(
      const peano4::datamanagement::CellMarker& marker,
      int voxelsPerAxis,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

    tarch::la::Vector<Dimensions,double> explicitEulerWithoutInterpolation(
      const peano4::datamanagement::CellMarker&   marker,
      int                                         voxelsPerAxis,
      int                                         unknownsPerVoxel,
      const tarch::la::Vector<Dimensions,int>&    velocityIndices,
      double                                      timeStepSize,
      double*                                     voxelField,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

    tarch::la::Vector<Dimensions,double> LinearInterp(
      const peano4::datamanagement::CellMarker&   marker,
      int                                         voxelsPerAxis,
      int                                         unknownsPerVoxel,
      const tarch::la::Vector<Dimensions,int>&    velocityIndices,
      double                                      timeStepSize,
      double*                                     voxelField,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

	// use this function to fix the position 
    tarch::la::Vector<Dimensions,double> StaticPosition(
      const peano4::datamanagement::CellMarker&   marker,
      int                                         voxelsPerAxis,
      int                                         unknownsPerVoxel,
      const tarch::la::Vector<Dimensions,int>&    velocityIndices,
      double                                      timeStepSize,
      double*                                     voxelField,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

    double ObLinearInterp(
      const peano4::datamanagement::CellMarker&   marker,
      int                                         voxelsPerAxis,
      int                                         unknownsPerVoxel,
      const int					  ObIndex,
      double*                                     voxelField,
      const tarch::la::Vector<Dimensions,double>& particleX
    );
  }
}

inline double linearInter1D(double x1, double f1, double x2, double f2, double target){
	if ((x1-x2)<10e-8){
		return (f1+f2)/2;
	} else {
		return f2*((target-x1)/(x2-x1))+f1*((x2-target)/(x2-x1));
	}
}

inline void FindInterIndex( //without the halo
	tarch::la::Vector<Dimensions,int>* InterIndex, 
	tarch::la::Vector<Dimensions*2,int> IndexOfCell,
	int voxelsPerAxis
	)
{	
	int x[2],y[2],z[2];
	
	if (IndexOfCell(Dimensions)==1) {
		if (IndexOfCell(0)<(voxelsPerAxis-1)) { x[0]=IndexOfCell(0);x[1]=IndexOfCell(0)+1;}
		else {x[0]=IndexOfCell(0)-1;x[1]=IndexOfCell(0);}
	} 
	if (IndexOfCell(Dimensions)==-1) {
		if (IndexOfCell(0)>0) { x[0]=IndexOfCell(0)-1;x[1]=IndexOfCell(0);}
		else {x[0]=IndexOfCell(0);x[1]=IndexOfCell(0)+1;}
	} 
		
	if (IndexOfCell(Dimensions+1)==1) {
		if (IndexOfCell(1)<(voxelsPerAxis-1)) { y[0]=IndexOfCell(1);y[1]=IndexOfCell(1)+1;}
		else {y[0]=IndexOfCell(1)-1;y[1]=IndexOfCell(1);}
	} 
	if (IndexOfCell(Dimensions+1)==-1) {
		if (IndexOfCell(1)>0) { y[0]=IndexOfCell(1)-1;y[1]=IndexOfCell(1);}
		else {y[0]=IndexOfCell(1);y[1]=IndexOfCell(1)+1;}
	} 
	
	#if Dimensions==2
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++){
		InterIndex[i*2+j]={x[i],y[j]};
	}
	#else 		
	if (IndexOfCell(Dimensions+2)==1) {
		if (IndexOfCell(2)<(voxelsPerAxis-1)) { z[0]=IndexOfCell(2);z[1]=IndexOfCell(2)+1;}
		else {z[0]=IndexOfCell(2)-1;z[1]=IndexOfCell(2);}
	} 
	if (IndexOfCell(Dimensions+2)==-1) {
		if (IndexOfCell(2)>0) { z[0]=IndexOfCell(2)-1;z[1]=IndexOfCell(2);}
		else {z[0]=IndexOfCell(2);z[1]=IndexOfCell(2)+1;}
	} 
	
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++)
	for(int k=0;k<2;k++){
		InterIndex[i*4+j*2+k]={x[i],y[j],z[k]};
	}
	#endif
}

inline tarch::la::Vector<Dimensions,double> Interpolation(
  tarch::la::Vector<Dimensions,int>* IndexForInter,
  double* raw,
  const tarch::la::Vector<Dimensions,double>& coor,
  const peano4::datamanagement::CellMarker& marker,
  int patchSize,
  int NumberofInterQ
){

  tarch::la::Vector<Dimensions,double> Offset=marker.getOffset();
  double volumeH=marker.h()(0)/patchSize;
  #if Dimensions==2
  int NumberofNeighbor=4;
  #else
  int NumberofNeighbor=8;
  #endif
  
	//calculate the actual coordinates
	#if Dimensions==3
	double CoorsForInter1[NumberofNeighbor][Dimensions];
	double raw1[NumberofNeighbor][NumberofInterQ];
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++)
	for(int k=0;k<2;k++){
		for (int m=0;m<Dimensions;m++){
			CoorsForInter1[i*4+j*2+k][m]=Offset(m)+(IndexForInter[i*4+j*2+k](m)+0.5)*volumeH;
		}
		for (int m=0;m<NumberofInterQ;m++){
			raw1[i*4+j*2+k][m]=raw[(i*4+j*2+k)*NumberofInterQ+m];
		}
	}
	//z direction interpolation
	double CoorsForInter2[4][Dimensions];
	double raw2[4][NumberofInterQ];
	for (int n=0;n<4;n++){
		CoorsForInter2[n][0]=CoorsForInter1[n][0];
		CoorsForInter2[n][1]=CoorsForInter1[n][1];
		CoorsForInter2[n][2]=coor(2);
		for (int m=0;m<NumberofInterQ;m++){
			raw2[n][m]=linearInter1D(CoorsForInter1[2*n][2],raw1[2*n][m],CoorsForInter1[2*n+1][2],raw1[2*n+1][m],coor(2));
		} 
	}
	#else //2d
	double CoorsForInter2[NumberofNeighbor][Dimensions];
	double raw2[NumberofNeighbor][NumberofInterQ];
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++){
		for (int m=0;m<Dimensions;m++){
			CoorsForInter2[i*2+j][m]=Offset(m)+(IndexForInter[i*2+j](m)+0.5)*volumeH;
		}
		for (int m=0;m<NumberofInterQ;m++){
			raw2[i*2+j][m]=raw[(i*2+j)*NumberofInterQ+m];
		}
	}
	#endif
	
	//second interpolate along y axis
	double CoorsForInter3[2][Dimensions];
	double raw3[2][NumberofInterQ];
	for (int n=0;n<2;n++){
		CoorsForInter3[n][0]=CoorsForInter2[n][0];
		CoorsForInter3[n][1]=coor(1);
		#if Dimensions==3
		CoorsForInter3[n][2]=coor(2);
		#endif
		for (int m=0;m<NumberofInterQ;m++){
			raw3[n][m]=linearInter1D(CoorsForInter2[2*n][1],raw2[2*n][m],CoorsForInter2[2*n+1][1],raw2[2*n+1][m],coor(1));
		} 
	}
	
	//finally interpolate along x axis
	double CoorsForInter4[1][Dimensions];
	double raw4[1][NumberofInterQ];
	for (int n=0;n<1;n++){
		CoorsForInter4[n][0]=coor(0);
		CoorsForInter4[n][1]=coor(1);
		#if Dimensions==3
		CoorsForInter4[n][2]=coor(2);
		#endif
		for (int m=0;m<NumberofInterQ;m++){
			raw4[n][m]=linearInter1D(CoorsForInter3[n][0],raw3[n][m],CoorsForInter3[n+1][0],raw3[n+1][m],coor(0));
		} 
	}

	tarch::la::Vector<Dimensions,double> result={0,0,0};
	for (int m=0;m<NumberofInterQ;m++){
		result(m)=raw4[0][m];
		//std::cout << result(m) << std::endl;
	}

	return result;
}


#endif
