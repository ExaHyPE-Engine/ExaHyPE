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
tarch::la::Vector<Dimensions,int>* FindInterIndex(tarch::la::Vector<Dimensions*2,int> IndexOfCell)
{	


}

//do the real interpolation
double* Interpolation(
  tarch::la::Vector<Dimensions,int>* IndexForInterpolate,
  double** raw,
  const double* coor,
  tarch::la::Vector<Dimensions,double> Offset,
  double volumeH,
  int patchSize
){


}


















