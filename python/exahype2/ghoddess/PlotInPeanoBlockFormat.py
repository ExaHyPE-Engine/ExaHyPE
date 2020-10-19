# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class PlotInPeanoBlockFormat(ActionSet):
  """
  Very simple plotter for Ghoddess
  """
  
  
  def __init__(self,filename,number_of_unknowns_within_cell,no_of_unknowns,dataset_name,description):
    self.d = {}
    self.d[ "FILENAME" ]          = filename
    self.d[ "UNKNOWNS" ]          = no_of_unknowns
    self.d[ "UNKNOWNS_PER_CELL" ] = number_of_unknowns_within_cell
    self.d[ "NAME" ]              = dataset_name
    self.d[ "DESCRIPTION" ]       = description
        

  __Template_Constructor = """
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  static bool calledBefore = false;
  tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode mode;
  if ( _treeNumber==0 and not calledBefore ) {{
    calledBefore = true;
    mode = tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::CreateNew;
  }}
  else if ( _treeNumber==0 ) {{
    mode = tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewDataSet;
  }}
  else {{
    mode = tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData;
  }}

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions,"{FILENAME}",mode
  );
  _dataWriter    = _writer->createCellDataWriter( "{NAME}", 1, {UNKNOWNS}*{UNKNOWNS_PER_CELL}, "{DESCRIPTION}" );
  _averageWriter = _writer->createCellDataWriter( "{NAME}Average", 1, {UNKNOWNS}, "{DESCRIPTION}" );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_Destructor = """
  static int rankLocalCounter = 0;
  static tarch::multicore::BooleanSemaphore booleanSemaphore;

  if (_dataWriter!=nullptr and _treeNumber>=0) {{
    _dataWriter->close();
    _averageWriter->close();

    int counter;
    {{
      tarch::multicore::Lock lock(booleanSemaphore);
      counter = rankLocalCounter;
      rankLocalCounter++;
    }}

    std::ostringstream filename;
    filename << "{FILENAME}" << "-tree-" << _treeNumber << "-" << counter;
    _writer->writeToFile( filename.str() );
  }}
  
  delete _dataWriter;
  delete _averageWriter;
  delete _writer;

  _dataWriter    = nullptr;
  _writer        = nullptr;
  _averageWriter = nullptr;
"""

    
  def get_destructor_body(self):
    return self.__Template_Destructor.format(**self.d)


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_TouchCellFirstTime = """ 
  int vertexIndices[TwoPowerD];
  
  const double PatchScaling = 0.95;

  assertion( _writer!=nullptr );
  assertion( _dataWriter!=nullptr );
  assertion( _averageWriter!=nullptr );
  
  const int patchIndex = _writer->plotPatch(
    marker.x() - marker.h() * PatchScaling * 0.5,
    marker.h() * PatchScaling
  );
 
  int cellIndex  = _dataWriter->getFirstCellWithinPatch(patchIndex);
  int currentDoF = 0;

  double averageValue[{UNKNOWNS}];
  for (int i=0; i<{UNKNOWNS}; i++) {{
    averageValue[i] = 0.0;
  }}

  for( int i=0; i<{UNKNOWNS_PER_CELL}; i++ ) {{  
    for (int unknown=0; unknown<{UNKNOWNS}; unknown++) {{
      averageValue[unknown] += *(fineGridCell{NAME}.value + i*{UNKNOWNS} + unknown );
    }}
  }}
  
  _dataWriter->plotCell( cellIndex, fineGridCell{NAME}.value );
  _averageWriter->plotCell( cellIndex, averageValue );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.format(**self.d) 
    return result


  def get_attributes(self):
    return """
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                  _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _dataWriter;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _averageWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/utils/Loop.h"
"""
