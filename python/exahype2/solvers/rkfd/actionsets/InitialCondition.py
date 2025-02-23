# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRKFDActionSet import AbstractRKFDActionSet

import peano4
import jinja2

class InitialCondition(AbstractRKFDActionSet):
  def __init__(self,solver,guard,grid_is_constructed, restart_from_checkpoint=False, loading_scheme="global-loading"):
    AbstractRKFDActionSet.__init__(self,solver)
    self.guard                    = guard
    self.grid_is_constructed      = grid_is_constructed
    self._restart_from_checkpoint = restart_from_checkpoint
    self._loading_scheme          = loading_scheme


  def get_body_of_operation(self,operation_name):
    result = ""

    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard      
      d[ "GRID_IS_CONSTRUCTED" ] = self.grid_is_constructed
      d[ "CHECKPOINTFILELIST"] = "CheckpointFilesOf" + self._solver._name
      if self._restart_from_checkpoint:
        result = jinja2.Template(self.get_template_for_restarting()).render(**d)
      else:
        result = jinja2.Template( self.TemplateInitialCondition ).render(**d)

    return result

  def get_body_of_prepareTraversal(self):
    result = "\n"

    if self._restart_from_checkpoint:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard      
      d[ "GRID_IS_CONSTRUCTED" ] = self.grid_is_constructed
      d[ "CHECKPOINTFILELIST"] = "CheckpointFilesOf" + self._solver._name
      result = jinja2.Template( self.get_template_for_restart_preprocessing() ).render(**d)

    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return """
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>

#include "exahype2/fd/PatchUtils.h"
#include "tarch/reader/PeanoTextPatchFileReader.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes 


  def get_static_initialisations(self,full_qualified_classname):
    if self._restart_from_checkpoint and self._loading_scheme=="global-loading":
      return """
std::vector<double> """ + full_qualified_classname + """::domainDataFromFiles;
std::vector<double> """ + full_qualified_classname + """::offsetIndex;
int """ + full_qualified_classname + """::domainPatchCount=0;
"""
    elif self._restart_from_checkpoint and self._loading_scheme=="local-loading":
      return """
//std::unordered_map<std::tuple<double, double, double>, std::string, tarch::reader::offsetHash, tarch::reader::offsetEqual> """ + full_qualified_classname + """::OffsetIndexMap;
//tarch::reader::CheckpointFilesManager """ + full_qualified_classname + """::subFilesManager(100*1024*1024*1024);
"""
    else:
      return """\n"""


  def get_attributes(self):
    if self._restart_from_checkpoint and self._loading_scheme=="global-loading":
      return """
    static std::vector<double> domainDataFromFiles;
    static std::vector<double> offsetIndex;
    static int domainPatchCount;
"""
    elif self._restart_from_checkpoint and self._loading_scheme=="local-loading":
      return """
    static inline std::map< tarch::la::Vector<Dimensions,double> , std::string, tarch::la::VectorCompare<Dimensions> > OffsetIndexMap{ tarch::la::VectorCompare<Dimensions>(1e-5) };
    static constexpr size_t memoryLimit = 100ULL * 1024 * 1024 * 1024;
    static inline tarch::reader::CheckpointFilesManager subFilesManager{memoryLimit};
"""
    else:
      return """\n"""

  TemplateInitialCondition = """
  if ({{PREDICATE}}) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );
    parallelDfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
      int index = peano4::utils::dLinearised(volume,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}) * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
        ::exahype2::fd::getGridCellCentre( marker.x(), marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, volume), 
        ::exahype2::fd::getGridCellSize( marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ),
        {{GRID_IS_CONSTRUCTED}}
      );
      //index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    } endParallelDfor
    
    // relevant for tracing et al which kicks in if and only if cell has been
    // updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  } 
"""

  def get_template_for_restarting(self):
    if self._loading_scheme=="global-loading":
      return """
//loading-scheme: global-loading
if ({{PREDICATE}}) { 
  logTraceIn( "touchCellFirstTime(...)---RestartingFromCheckpoint" );
  
  #if Dimensions==2
    tarch::la::Vector<2,double> offset;
    const int totalCellCount={{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
  #elif Dimensions==3
    tarch::la::Vector<3,double> offset;
    const int totalCellCount={{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
  #endif

  for (int i=0;i<domainPatchCount;i++) {
    bool FoundCell = false;
    #if Dimensions==2
    offset[0]=offsetIndex[i*2+0];
    offset[1]=offsetIndex[i*2+1];
    #elif Dimensions==3
    offset[0]=offsetIndex[i*3+0];
    offset[1]=offsetIndex[i*3+1];
    offset[2]=offsetIndex[i*3+2];
    #endif

    if ( tarch::la::equals( marker.x() - 0.5*marker.h(), offset, 1e-5) ) { FoundCell=true; }
    
    if (FoundCell){
      parallelDfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
        int index = peano4::utils::dLinearised(volume,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}) * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
        for (int k=0; k<=({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}); k++){
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value[index+k]=domainDataFromFiles[i*({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})*totalCellCount+index+k];
        }
      } endParallelDfor
      break;
    }
  }

  fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(CheckpointTimeStamp);
  fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  logTraceOut( "touchCellFirstTime(...)---RestartingFromCheckpoint" );
}
"""
    elif self._loading_scheme=="local-loading":
      return """
//loading-scheme: local-loading
//warning: it only support 3D checkpointing file currently!
if ({{PREDICATE}}) {
  logTraceIn( "touchCellFirstTime(...)---RestartingFromCheckpoint" );
  
  #if Dimensions==2
    tarch::la::Vector<2,double> offset;
    const int totalCellCount={{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
  #elif Dimensions==3
    tarch::la::Vector<3,double> offset;
    const int totalCellCount={{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}*{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
  #endif
  double patchDataFromFile[totalCellCount*({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})];

  std::string targetCheckpointFileName=OffsetIndexMap[( marker.x() - 0.5*marker.h() )];
  if (targetCheckpointFileName.empty()) {std::cout<<"Patch "<< marker.x() - 0.5*marker.h() << " do not see a correct file " << targetCheckpointFileName <<std::endl;}
  //else {std::cout<<"Patch "<< marker.x() - 0.5*marker.h() << " should be in " << targetCheckpointFileName <<std::endl;}

  const auto& lines = subFilesManager.getCheckpointFileData(targetCheckpointFileName);
  bool PatchFound=false;

  for (const auto& line : lines) {
    if (line.find("  offset") == 0) { 
      std::istringstream iss(line);
      std::string token; int index=0;

      while (iss >> token) { try {offset[index]=std::stod(token); index++; } catch (...){ } }
      if ( tarch::la::equals( marker.x() - 0.5*marker.h(), offset, 1e-6) ){ PatchFound=true; }
    }
    if ( PatchFound and (line.find("     ") == 0) ){
      std::istringstream iss(line);
      std::string token; int index=0;

      while (iss >> token) { patchDataFromFile[index]=std::stod(token); index++; }
      //std::cout<<"Found patch with "<< marker.x() - 0.5*marker.h() << " in " << targetCheckpointFileName <<std::endl;
      break;
    }
  }

  parallelDfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
      int index = peano4::utils::dLinearised(volume,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}) * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}) ;
      for (int i=0; i<=({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}); i++){
            fineGridCell{{UNKNOWN_IDENTIFIER}}.value[index+i]=patchDataFromFile[index+i];
      }
  } endParallelDfor

  fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(CheckpointTimeStamp);
  fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  logTraceOut( "touchCellFirstTime(...)---RestartingFromCheckpoint" );
}
"""
  
  def get_template_for_restart_preprocessing(self):
    if self._loading_scheme=="global-loading":
      return """        
      tarch::reader::readInCheckpointFiles(CheckpointFilesOf{{SOLVER_NAME}}, domainDataFromFiles, offsetIndex,
                                             {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, Dimensions, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}});
      #if Dimensions==2
      domainPatchCount=offsetIndex.size()/2;
      #elif Dimensions==3
      domainPatchCount=offsetIndex.size()/3;
      #endif

"""
    elif self._loading_scheme=="local-loading":
      return """
      tarch::reader::readInCheckpointOffsetIndex(OffsetIndexMap, CheckpointFilesOf{{SOLVER_NAME}});

"""