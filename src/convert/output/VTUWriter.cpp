#include <string>
#include <unordered_map>
#include <vector>

#include "../config.h"
#include "convert/input/PeanoTextPatchFileReader.h"

#include "VTUWriter.h"

#include "tarch/Assertions.h"


#ifdef UseVTK
#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkAppendFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkCellArray.h"
#include "vtkVoxel.h"
#include "vtkQuad.h"

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkMergeCells.h>
#endif


tarch::logging::Log  convert::output::VTUWriter::_log( "PeanoConverter" );


convert::output::VTUWriter::VTUWriter(const std::string&  directory, const std::string& outputFileWithoutExtension):
  _outputFileWithoutExtension(outputFileWithoutExtension),
  _directory(directory) {

/*
	  _pvdFile(directory + "/" + outputFileWithoutExtension+".pvd")
	_pvdFile << "<?xml version=\"1.0\"?>" << std::endl
	       << "<VTKFile type=\"Collection\" version=\"0.1\" >" << std::endl
	 	   << "<Collection>" << std::endl;
*/
}


convert::output::VTUWriter::~VTUWriter() {
/*
  _pvdFile << "</Collection>" << std::endl
	       << "</VTKFile>" << std::endl;
  _pvdFile.close();
*/
}


#ifdef UseVTK
vtkSmartPointer<vtkDoubleArray> convert::output::VTUWriter::getMetaDataForOnePatch(const convert::data::Variable& variable, const convert::data::PatchData& data) {
  vtkSmartPointer<vtkDoubleArray> variableArray = vtkSmartPointer<vtkDoubleArray>::New();
  variableArray->SetNumberOfComponents( 1 );
  variableArray->SetName("source-file");

  for(int i = 0; i < variable.getTotalNumberOfDofsPerPatch(); i++) {
    double array[] = { static_cast<double>(data.originTree) };
    variableArray->InsertNextTuple(array);
  }
  return variableArray;
}


vtkSmartPointer<vtkDoubleArray> convert::output::VTUWriter::getVTUDataForOnePatch(const convert::data::Variable& variable, const convert::data::PatchData& data) {
  vtkSmartPointer<vtkDoubleArray> variableArray = vtkSmartPointer<vtkDoubleArray>::New();
  variableArray->SetNumberOfComponents( variable.unknowns );
  variableArray->SetName(variable.name.c_str());

  for(int i = 0; i < variable.getTotalNumberOfDofsPerPatch(); i++) {
    variableArray->InsertNextTuple(&(data.data[i*variable.unknowns]));
    assertion1(variable.unknowns>0,variable.unknowns);
    for (int j=0; j<variable.unknowns; j++) {
      assertion( data.data[i*variable.unknowns+j]==data.data[i*variable.unknowns+j] );
    }
  }
  return variableArray;
}


vtkSmartPointer<vtkImageData> convert::output::VTUWriter::toImageData(const convert::data::Variable& variable, const convert::data::PatchData& patchData) {
  vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();

  //get the data from the patch in to arrays
  double spacing[3] = {1, 1, 1};
  double offSets[3] = {0, 0, 0};
  int dimensions[3] = {1, 1, 1};

  for(int i = 0; i < variable.dimensions; i++) {
    dimensions[i] = variable.getVerticesPerAxisInCartesianMesh();
    offSets[i]    = patchData.offset[i];
    spacing[i]    = patchData.size[i]/(variable.getVerticesPerAxisInCartesianMesh()-1);
  }

  imageData->SetDimensions(dimensions);
  imageData->SetOrigin(offSets);
  imageData->SetSpacing(spacing);

  vtkSmartPointer<vtkDoubleArray> variableArray = getVTUDataForOnePatch(variable,patchData);
  vtkSmartPointer<vtkDoubleArray> metaDataArray = getMetaDataForOnePatch(variable,patchData);

  if(variable.type == convert::data::PeanoDataType::Cell_Values) {
    imageData->GetCellData()->AddArray(variableArray);
    imageData->GetCellData()->AddArray(metaDataArray);
  }
  else if(variable.type == convert::data::PeanoDataType::Vertex_Values) {
    imageData->GetPointData()->AddArray(variableArray);
    imageData->GetPointData()->AddArray(metaDataArray);
  }

  return imageData;
}


vtkSmartPointer<vtkUnstructuredGrid> convert::output::VTUWriter::toUnstructuredGrid(const convert::data::Variable& variable, const convert::data::PatchData& patchData) {
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

/*
  double spacing[3] = {1, 1, 1};
  double offSets[3] = {0, 0, 0};
  int dimensions[3] = {1, 1, 1};

	double* offsets = patch->offsets;
	double* sizes = patch->sizes;
	double* mapping = structure->mapping;
	int mappings = structure->mappings;
	int dimensions = patch->dimensions;
	int numberOfPoints = mappings/patch->dimensions;
*/

  double spacing[3] = {1, 1, 1};
  double offSets[3] = {0, 0, 0};

  for(int i = 0; i < variable.dimensions; i++) {
    spacing[i]    = patchData.size[i]/(variable.getVerticesPerAxisInCartesianMesh()-1);
    offSets[i]    = patchData.offset[i];
  }

  points->SetNumberOfPoints( variable.getTotalNumberOfVerticesInCartesianMesh() );

  double position[3] = {0, 0, 0};
  int i =0;
  if (variable.dimensions==2) {
    for (int xx=0; xx<variable.getVerticesPerAxisInCartesianMesh(); xx++)
    for (int yy=0; yy<variable.getVerticesPerAxisInCartesianMesh(); yy++) {
      position[0] = offSets[0] + xx * spacing[0];
      position[1] = offSets[1] + yy * spacing[1];
      points->SetPoint(i, position);
      i++;
    }
  }
  else {
    for (int xx=0; xx<variable.getVerticesPerAxisInCartesianMesh(); xx++)
    for (int yy=0; yy<variable.getVerticesPerAxisInCartesianMesh(); yy++)
    for (int zz=0; zz<variable.getVerticesPerAxisInCartesianMesh(); zz++) {
      position[0] = offSets[0] + xx * spacing[0];
      position[1] = offSets[1] + yy * spacing[1];
      position[2] = offSets[2] + zz * spacing[2];
      points->SetPoint(i, position);
      i++;
    }
  }

  vtkSmartPointer<vtkCellArray> connectivity = vtkSmartPointer<vtkCellArray>::New();
  connectivity->Allocate(VTK_VOXEL,variable.getTotalNumberOfCellsInCartesianMesh());

  vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  if (variable.dimensions==3) {
    for (int zz=0; zz<variable.getVerticesPerAxisInCartesianMesh()-1; zz++)
    for (int yy=0; yy<variable.getVerticesPerAxisInCartesianMesh()-1; yy++)
    for (int xx=0; xx<variable.getVerticesPerAxisInCartesianMesh()-1; xx++) {
      vtkSmartPointer<vtkVoxel> voxel = vtkSmartPointer<vtkVoxel>::New();
      vtkIdList* points = voxel->GetPointIds();
      points->SetId(0, xyzToIndex(xx,  yy,  zz,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(1, xyzToIndex(xx+1,yy,  zz,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(2, xyzToIndex(xx,  yy+1,zz,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(3, xyzToIndex(xx+1,yy+1,zz,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(4, xyzToIndex(xx,  yy,  zz+1, variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(5, xyzToIndex(xx+1,yy,  zz+1, variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(6, xyzToIndex(xx,  yy+1,zz+1, variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(7, xyzToIndex(xx+1,yy+1,zz+1, variable.getVerticesPerAxisInCartesianMesh()));
      connectivity->InsertNextCell(voxel);
    }
    grid->SetPoints(points);
    grid->SetCells(VTK_VOXEL, connectivity);
  }
  else {
    for (int yy=0; yy<variable.getVerticesPerAxisInCartesianMesh()-1; yy++)
    for (int xx=0; xx<variable.getVerticesPerAxisInCartesianMesh()-1; xx++) {
      vtkSmartPointer<vtkQuad> voxel = vtkSmartPointer<vtkQuad>::New();
      vtkIdList* points = voxel->GetPointIds();
      points->SetId(0, xyzToIndex(xx,  yy,  0,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(1, xyzToIndex(xx+1,yy,  0,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(2, xyzToIndex(xx,  yy+1,0,   variable.getVerticesPerAxisInCartesianMesh()));
      points->SetId(3, xyzToIndex(xx+1,yy+1,0,   variable.getVerticesPerAxisInCartesianMesh()));
      connectivity->InsertNextCell(voxel);
    }
    grid->SetPoints(points);
    grid->SetCells(VTK_QUAD, connectivity);
  }

  vtkSmartPointer<vtkDoubleArray> variableArray = getVTUDataForOnePatch(variable,patchData);
  if (variable.type == convert::data::PeanoDataType::Cell_Values) {
    grid->GetCellData()->AddArray(variableArray);
  }
  else if(variable.type == convert::data::PeanoDataType::Vertex_Values) {
    grid->GetPointData()->AddArray(variableArray);
  }

  return grid;
}
#endif


int convert::output::VTUWriter::xyzToIndex(int x, int y, int z, int verticesPerAxis) {
  return x + y*verticesPerAxis + z*verticesPerAxis*verticesPerAxis;
}


void convert::output::VTUWriter::writeFile(const convert::data::Variable& variable, const std::vector<convert::data::PatchData>& patchData) {
  #ifdef UseVTK
  vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
  for (auto p: patchData) {
/*
	if(variable.hasMappings()) {
      appendFilter->AddInputData(toUnstructuredGrid(variable,p));
	}
	else {
*/
      appendFilter->AddInputData(toImageData(variable,p));
//	}
  }

  appendFilter->Update();
  vtkSmartPointer<vtkUnstructuredGrid> combined = vtkSmartPointer<vtkUnstructuredGrid>::New();
  combined->ShallowCopy(appendFilter->GetOutput());

  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  std::string outputFileName = _directory + "/" + _outputFileWithoutExtension + "." + writer->GetDefaultFileExtension();
  writer->SetFileName(outputFileName.c_str());
//  writer->SetCompressorTypeToZLib();
  writer->AddInputDataObject( combined );
  writer->Write();

  logInfo( "writeFile(...)", "write to file " << outputFileName );
  #endif
}



/*

void convert::output::VTUWriter::writeFile(const std::string& outputFileWithoutExtention, const std::vector<PeanoPatch*>& patches) {
  #ifdef UseVTK
  if (!patches.empty()) {
     std::string outFile = outputFileWithoutExtention + "." + writer->GetDefaultFileExtension();

    vtkSmartPointer<vtkUnstructuredGrid> outputGrid = convert::output::VTUWriter::combine( patches );

    writer->SetFileName(outFile.c_str());
    writer->SetCompressorTypeToZLib();
    writer->AddInputDataObject( outputGrid );
    writer->Write();
  }
  else {
    logError( "combineAndWriteToFile(...)", "Output is empty" );
  }
  #else
  logError( "combineAndWriteToFile(...)", "Can't build VTK files as code is setup without VTK (rerun ./configure with --with-vtk)" );
  return "";
  #endif
}


#ifdef UseVTK
vtkSmartPointer<vtkUnstructuredGrid> convert::output::VTUWriter::combine(const std::vector<PeanoPatch*>& patches){
  vtkSmartPointer<vtkAppendFilter> appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
  for(uint i = 0; i < patches.size(); i++) {
    PeanoPatch* patch = patches.at(i);
   	if(patch->hasMappings()) {
	  appendFilter->AddInputData(toUnstructuredGrid(patches.at(i)));
	} else {
      appendFilter->AddInputData(toImageData(patches.at(i)));
	}
  }

  appendFilter->Update();
  vtkSmartPointer<vtkUnstructuredGrid> combined = vtkSmartPointer<vtkUnstructuredGrid>::New();
  combined->ShallowCopy(appendFilter->GetOutput());

  return combined;
}
*/



/*
PeanoPatch* convert::output::VTUWriter::subSample(std::vector<PeanoReader*> &readers, int x, int y, int z) {
	std::vector<PeanoPatch*> patches;

	double xMax, yMax, zMax, xMin, yMin, zMin;
	xMax = yMax = zMax = std::numeric_limits<double>::min();
	xMin = yMin = zMin = std::numeric_limits<double>::max();

	//calculate the minimum & maximum sizes and add all the patches to the patches vector
	for(uint i = 0; i < readers.size(); i++) {
		//add all the reader's patches to the patch vector
		std::vector<PeanoPatch*> currentPatches = readers[i]->patches;
		for(uint j = 0; j < currentPatches.size(); j++) {
			PeanoPatch* patch = currentPatches[j];
			patches.push_back(patch);
			if(patch->offsets[0] < xMin) xMin = patch->offsets[0];
			if(patch->offsets[1] < yMin) yMin = patch->offsets[1];
			if(patch->offsets[2] < zMin) zMin = patch->offsets[2];
			if(patch->offsets[0] + patch->sizes[0] > xMax) xMax = patch->offsets[0] + patch->sizes[0];
			if(patch->offsets[1] + patch->sizes[1] > yMax) yMax = patch->offsets[1] + patch->sizes[1];
			if(patch->offsets[2] + patch->sizes[2] > zMax) zMax = patch->offsets[2] + patch->sizes[2];
		}
	}

	std::vector<int> patchSize;
	patchSize.push_back(x);
	patchSize.push_back(y);
	patchSize.push_back(z);

	int* resolution = new int[3];
	resolution[0] = x;
	resolution[1] = y;
	resolution[2] = z;

	double* offsets = new double[3];
	offsets[0] = xMin;
	offsets[1] = yMin;
	offsets[2] = zMin;

	double* sizes = new double[3];
	sizes[0] = xMax - xMin;
	sizes[1] = yMax - yMin;
	sizes[2] = zMax - zMin;


	//put the pieces together
	PeanoPatch* outPatch = new PeanoPatch();
	outPatch->dimensions = 3;
	outPatch->offsets = offsets;
	outPatch->resolution = resolution;
	outPatch->sizes = sizes;

	int outputCells = x*y*z;
	int outputVertices = (x+1)*(y+1)*(z+1);


	//std::unordered_map<std::string, PeanoPatchData*> patchData;


	//create a list of patchData for the new patch
	std::vector<PeanoPatchData*> datas;
	PeanoPatch* patch1 = patches[0];
	for (auto it : patch1->patchData) {
		PeanoVariable* var = it.second->structure;
		PeanoVariable* newVar = new PeanoVariable(var->name, var->unknowns, var->type,
				(var->type==Cell_Values?outputCells:outputVertices), nullptr, -1);

		PeanoPatchData* newData = new PeanoPatchData(newVar);
		//set all values to 0
		for(int j = 0; j < newVar->totalValues; j++) {
			newData->values[j] = 0;
		}

		//add the data to the patch and to our list of data objects
		outPatch->patchData[newData->structure->name] = newData;
		datas.push_back(newData);
	}

	for(uint i = 0; i < patches.size(); i++) {
		PeanoPatch* patch = patches[i];
		for (uint dataIndex = 0; dataIndex < datas.size(); dataIndex++) {
			PeanoPatchData* newData = datas[dataIndex];
			PeanoVariable* newVar = newData->structure;
			PeanoPatchData* oldData = patch->patchData[newData->structure->name];
			PeanoVariable* oldVar = oldData->structure;

			int unknowns = newVar->unknowns;
			double* data = oldData->values;

			int x = 0;
			int y = 0;
			int z = 0;

			//std::cout << "total values = " << oldVar->totalValues << "\n";

			if(newData->structure->type == Cell_Values) {
				for(int j = 0; j < oldVar->totalValues; j+= oldVar->unknowns) {
					double* position = patch->getPositionCellCenter(x,y,z);

					//see my notes for this equation
					int xCell = (position[0] - offsets[0])*resolution[0]/sizes[0];
					int yCell = (position[1] - offsets[1])*resolution[1]/sizes[1];
					int zCell = (position[2] - offsets[2])*resolution[2]/sizes[2];

					delete position;

					int index = outPatch->getIndexCellData(xCell, yCell, zCell);
					newData->setData(index, data +j);

					//increment the x,y,z values
					z++;
					if(z == patch->resolution[2]) {
						z = 0;
						y++;
						if(y == patch->resolution[1]) {
							y = 0;
							x++;
						}
					}
				}
				int breakpoint = 0;
			} else {//Vertex_Values
				for(int j = 0; j < oldVar->totalValues; j+= oldVar->unknowns) {
					double* position = patch->getPositionVertex(x,y,z);

					//see my notes for this equation
					int xVert = ((position[0] - offsets[0])*resolution[0]/sizes[0])+0.5;
					int yVert = ((position[1] - offsets[1])*resolution[1]/sizes[1])+0.5;
					int zVert = ((position[2] - offsets[2])*resolution[2]/sizes[2])+0.5;

					delete position;

					int index = outPatch->getIndexVertexData(xVert, yVert, zVert);
					newData->setData(index, data +j);

					//increment the x,y,z values
					z++;
					if(z == patch->resolution[2] +1) {
						z = 0;
						y++;
						if(y == patch->resolution[1] +1) {
							y = 0;
							x++;
						}
					}
				}
				int breakpoint = 0;
			}
		}
	}
	return outPatch;
}
*/
