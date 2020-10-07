# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class PlotGridInPeanoBlockFormat(ActionSet):
  def __init__(self,filename,cell_unknown):
    """
      Plot only the grid structure
      
      :Attibutes:
      
      filename: String
         Name of the output file.
         
      cell_unknown: None or cell data   
         If you use cell unknowns, pass any unknown in. As we do not dump
         any semantic information about unknowns, it does not matter which 
         one you choose. If you don't have cell unknowns at all, pass in 
         None.
          
    """
    self.d = {}
    self.d[ "FILENAME" ]     = filename
    self.d[ "CELL_WRAPPER" ] = "Cell"
    if cell_unknown!=None:
      self.d[ "CELL_WRAPPER" ] += cell_unknown.name
    

  __Template_Constructor = """
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_Destructor = """
  static int rankLocalCounter = 0;
  static tarch::multicore::BooleanSemaphore booleanSemaphore;
  
  if (_dataWriter!=nullptr and _treeNumber>=0) {{
    _dataWriter->close();

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
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;
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

  int indices = _writer->plotPatch(
    marker.x() - marker.h() * 0.5,
    marker.h()
  );

  assertion( _dataWriter!=nullptr );
  
  double markerData[] = {{
    marker.isRefined() ? 1.0 : 0.0,
    marker.isLocal() ? 1.0 : 0.0,
    marker.isEnclaveCell() ? 1.0 : 0.0
  }};
 _dataWriter->plotCell(indices,markerData);
"""


  __Template_BeginTraversal = """
  static bool calledBefore = false;
  if ( _treeNumber==0 and not calledBefore ) {{
    calledBefore = true;
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions,"{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::CreateNew
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
  }}
  else if ( _treeNumber==0 ) {{
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions,"{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewDataSet
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
  }}
  else {{
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions,"{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::DontChange
    );
  }}

  _dataWriter = _writer->createCellDataWriter( "cell-marker(refined,local,enclave)", 1, 3 );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.format(**self.d) 
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginTraversal.format(**self.d)             
    return result


  def get_attributes(self):
    return """
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                  _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _dataWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/mpi/Lock.h"
#include "peano4/parallel/SpacetreeSet.h"
"""
