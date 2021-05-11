#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <cstring>

#include "tarch/la/Vector.h"

std::istream& ignoreline(std::fstream& in, std::ifstream::pos_type& pos)
{
    pos = in.tellg();
    return in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getLastLine(std::fstream& in)
{
    std::ifstream::pos_type pos = in.tellg();

    std::ifstream::pos_type lastPos;
    while (in >> std::ws && ignoreline(in, lastPos))
        pos = lastPos;

    in.clear();
    in.seekg(pos);

    std::string line;
    std::getline(in, line);
    return line;
}

double linearInter(double x1, double f1, double x2, double f2, double target){
	if ((x1-x2)<10e-8){
		return (f1+f2)/2;
	} else {
		return f2*((target-x1)/(x2-x1))+f1*((x2-target)/(x2-x1));
	}
}


void CoorReadIn(double* coor, std::string line)
{
	int index=0;
	std::string var[3]={"","",""};
	for (auto x : line){
		if (x == ' '){
			coor[index]=std::stod(var[index]);
			index++;
		} else {
			var[index]=var[index]+x;
		}
	}
	coor[index]=std::stod(var[index]);
}

tarch::la::Vector<Dimensions*2,int> FindCellIndex(
  const double* coor,
  tarch::la::Vector<Dimensions,double> Offset,
  double volumeH,
  int patchSize
) {
	//std::cout<<Offset(0)<<" "<<Offset(1)<<" "<<Offset(2)<<" "<< volumeH <<std::endl;
	tarch::la::Vector<Dimensions*2,int> index= {0,0,0,0,0,0};
	for (int i=0;i<patchSize;i++){
		if (coor[0]>(Offset(0)+i*volumeH) and coor[0]<(Offset(0)+(i+1)*volumeH))
			{index(0)=i;
			if (coor[0]<(Offset(0)+(i+0.5)*volumeH))
				{index(3)=-1;}
			else if (coor[0]>(Offset(0)+(i+0.5)*volumeH))
				{index(3)=1;}
		}
	}
	index(0)++; //to include the extra layer
	//std::cout<<coor[0]<<" "<<(Offset(0)+index(0)*volumeH)<<" "<<(Offset(0)+(index(0)+1)*volumeH)<<" "<< (Offset(0)+(index(0)+0.5)*volumeH) <<std::endl;
	for (int i=0;i<patchSize;i++){
		if (coor[1]>(Offset(1)+i*volumeH) and coor[1]<(Offset(1)+(i+1)*volumeH))
			{index(1)=i;
			if (coor[1]<(Offset(1)+(i+0.5)*volumeH))
				{index(4)=-1;}
			else if (coor[1]>(Offset(1)+(i+0.5)*volumeH))
				{index(4)=1;}
		}
	}
	index(1)++; //to include the extra layer
	//std::cout<<coor[1]<<" "<<(Offset(1)+index(1)*volumeH)<<" "<<(Offset(1)+(index(1)+1)*volumeH)<<" "<< (Offset(1)+(index(1)+0.5)*volumeH) <<std::endl;
	for (int i=0;i<patchSize;i++){
		if (coor[2]>(Offset(2)+i*volumeH) and coor[2]<(Offset(2)+(i+1)*volumeH))
			{index(2)=i;
			if (coor[2]<(Offset(2)+(i+0.5)*volumeH))
				{index(5)=-1;}
			else if (coor[2]>(Offset(2)+(i+0.5)*volumeH))
				{index(5)=1;}
		}
	}
	index(2)++; //to include the extra layer
	return index;
}

//output index for 8 nearest cells
void FindInterIndex(tarch::la::Vector<Dimensions,int>* InterIndex, tarch::la::Vector<Dimensions*2,int> IndexOfCell)
{	
	int x[2],y[2],z[2];
	
	if (IndexOfCell(3)==1) {
		x[0]=IndexOfCell(0);x[1]=IndexOfCell(0)+1;
	} else if (IndexOfCell(3)==-1) {
		x[0]=IndexOfCell(0)-1;x[1]=IndexOfCell(0);
	} else {
		x[0]=IndexOfCell(0);x[1]=IndexOfCell(0);}
		
	if (IndexOfCell(4)==1) {
		y[0]=IndexOfCell(1);y[1]=IndexOfCell(1)+1;
	} else if (IndexOfCell(4)==-1) {
		y[0]=IndexOfCell(1)-1;y[1]=IndexOfCell(1);
	} else {
		y[0]=IndexOfCell(1);y[1]=IndexOfCell(1);}
		
	if (IndexOfCell(5)==1) {
		z[0]=IndexOfCell(2);z[1]=IndexOfCell(2)+1;
	} else if (IndexOfCell(5)==-1) {
		z[0]=IndexOfCell(2)-1;z[1]=IndexOfCell(2);
	} else {
		z[0]=IndexOfCell(2);z[1]=IndexOfCell(2);}	
	
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++)
	for(int k=0;k<2;k++){
		InterIndex[i*4+j*2+k]={x[i],y[j],z[k]};
	}
	
}


//do the real interpolation
void Interpolation(
  double* result,
  tarch::la::Vector<Dimensions,int>* IndexForInter,
  double* raw,
  const double* coor,
  tarch::la::Vector<Dimensions,double> Offset,
  double volumeH,
  int patchSize
){
	//calculate the actual coordinates
	double CoorsForInter1[8][3];
	double raw1[8][3];
	for(int i=0;i<2;i++)
	for(int j=0;j<2;j++)
	for(int k=0;k<2;k++){
		for (int m=0;m<3;m++){
			CoorsForInter1[i*4+j*2+k][m]=Offset(m)+(IndexForInter[i*4+j*2+k](m)-0.5)*volumeH;
			raw1[i*4+j*2+k][m]=raw[(i*4+j*2+k)*3+m];
		}
	}
	
	//first interpolate along x axis
	double CoorsForInter2[4][3];
	double raw2[4][3];
	for (int n=0;n<4;n++){
		CoorsForInter2[n][0]=coor[0];
		CoorsForInter2[n][1]=CoorsForInter1[n][1];
		CoorsForInter2[n][2]=CoorsForInter1[n][2];
		for (int m=0;m<3;m++){
			raw2[n][m]=linearInter(CoorsForInter1[n][0],raw1[n][m],CoorsForInter1[n+4][0],raw1[n+4][m],coor[0]);
		} 
	}
	
	//second interpolate along y axis
	double CoorsForInter3[2][3];
	double raw3[2][3];
	for (int n=0;n<2;n++){
		CoorsForInter3[n][0]=coor[0];
		CoorsForInter3[n][1]=coor[1];
		CoorsForInter3[n][2]=CoorsForInter1[n][2];
		for (int m=0;m<3;m++){
			raw3[n][m]=linearInter(CoorsForInter2[n][1],raw2[n][m],CoorsForInter2[n+2][1],raw2[n+2][m],coor[1]);
		} 
	}
	
	//finally interpolate along z axis
	double CoorsForInter4[1][3];
	double raw4[1][3];
	for (int n=0;n<1;n++){
		CoorsForInter4[n][0]=coor[0];
		CoorsForInter4[n][1]=coor[1];
		CoorsForInter4[n][2]=coor[2];
		for (int m=0;m<3;m++){
			raw4[n][m]=linearInter(CoorsForInter3[n][2],raw3[n][m],CoorsForInter3[n+1][2],raw3[n+1][m],coor[2]);
		} 
	}	
	
	result[0]=raw4[0][0]; result[1]=raw4[0][1]; result[2]=raw4[0][2];
}
















