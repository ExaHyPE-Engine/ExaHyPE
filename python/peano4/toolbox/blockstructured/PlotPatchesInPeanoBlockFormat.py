# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class PlotPatchesInPeanoBlockFormat(ActionSet):
  """
  Very simple plotter that should be used in combination with Patches.
  At the moment, I only support plain double patches, i.e. you have to 
  use the default source code generator for your stuff. This plotter 
  works only for cell patches, i.e. patches associated with faces or 
  vertices are not supported.
  
  At the moment, I can only plot one dataset (patch type) per plotter.
  In theory, the underlying Peano plotter however could dump multiple
  patches at once.
  
  Also, this dump expects incoming data to be AoS.
  """
  
  
  def __init__(self,filename,patch,dataset_name):
    self.d = {}
    self.d[ "FILENAME" ]           = filename
    self.d[ "UNKNOWNS" ]           = str(patch.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]      = str(patch.dim[0])
    self.d[ "NAME" ]               = dataset_name
        
    for i in patch.dim:
      if i!=patch.dim[0]:
        print( "Error: patch plotter requires patch to have same dimension along all coordinate axes")

    #self.dim       = dim
    #self.no_of_unknowns = no_of_unknowns

  __Template_Constructor = """
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_Destructor = """
  _dataWriter->close();
  _writer->writeToFile();
  
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
  
  const double PatchScaling = 0.95;

  assertion( _writer!=nullptr );
  assertion( _dataWriter!=nullptr );
  
  const int patchIndex = _writer->plotPatch(
    marker.x() - marker.h() * PatchScaling * 0.5,
    marker.h() * PatchScaling
  );
 
  int cellIndex  = _dataWriter->getFirstCellWithinPatch(patchIndex);
  int currentDoF = 0;
  
  dfor(k,{DOFS_PER_AXIS}) {{
    double* data = fineGridCell{NAME}.value + currentDoF;
    _dataWriter->plotCell( cellIndex, data );
    cellIndex++;
    currentDoF += {UNKNOWNS};
  }}
"""


  __Template_BeginTraversal = """
  static int rankLocalCounter = 0;
  static tarch::mpi::BooleanSemaphore booleanSemaphore("{FILENAME}");
  static bool calledBefore = false;
  
  int counter;
  {{
    tarch::mpi::Lock lock(booleanSemaphore);
    counter = rankLocalCounter;
    rankLocalCounter++;
  }}
  std::ostringstream snapshotFileName;
  snapshotFileName << "{FILENAME}" << "-tree-" << _treeNumber << "-" << counter;

  if ( _treeNumber==0 and not calledBefore ) {{
    calledBefore = true;
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::CreateNew
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
  }}
  else if ( _treeNumber==0 ) {{
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewDataSet
    );
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
  }}
  else {{
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{FILENAME}");
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData
    );
  }}

  _dataWriter = _writer->createCellDataWriter( "{NAME}", {DOFS_PER_AXIS}, {UNKNOWNS} );
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
#include "peano4/utils/Loop.h"
#include "peano4/parallel/SpacetreeSet.h"
"""
