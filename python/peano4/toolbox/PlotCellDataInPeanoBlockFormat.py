# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PlotCellDataInPeanoBlockFormat(ActionSet):
  """!
  
  This writer writes a fixed number of unknown per cell
  
  Originally, I had considered this plotter to be used if you have a Finite 
  Volume-type discretisation with N unknowns associated to a cell. However, it
  turns out that there might be other use cases: People might want to add N
  unknowns but actually specify that these unknowns are distributed in a certain
  way. So you can alter the meta information, but per default it is set to one
  vector of entries per cell. If you want to visualise patch data, I still
  recommend to use the tailored patch data writer, which plays nicely together
  with the Python data model.
  
  """
  NoMetaFile     = "no-meta-file"
  CountTimeSteps = "count-time-steps"
  CastToDouble   = -1

  def __init__(self,
               filename,
               cell_unknown,
               getter,
               description,
               time_stamp_evaluation, 
               number_of_unknows_per_cell=1,
               guard_predicate="true", 
               additional_includes=""
               ):
    """
      Plot only the grid structure
      
      filename: String
        Name of the output file
        
      vertex_unknown: (DaStGen) object tied to a vertex 
        The object you have associated with a vertex and that you want to print.
        The code will access its name via 
        
             fineGridCell{CELL_UNKNOWN_NAME}
             
        in the generated source code 
        
      getter: String (C++ code)
        Getter acting on the vertex. Could be something alike getU() for example.
        If there's no getter but you want to directly access the data, remove 
        any brackets from the passed string.
        
      time_stamp_evaluation: String
        C++ expression returning a double. This is used to write the time series
        file for a series of snapshots. Pass in "0" if you solve a stationary 
        problem, e.g. 

      number_of_unknows_per_cell: Integer
        Has to match to the getter. You can hand in a CastToDouble which instructs the 
        plotter to explicitly cast the entry to double.
    """
    super( PlotCellDataInPeanoBlockFormat, self ).__init__()
    
    self.d = {}
    self.d[ "FILENAME" ]            = filename
    self.d[ "CELL_UNKNOWN_NAME" ]   = cell_unknown.name
    self.d[ "GETTER" ]              = getter
    self.d[ "NUMBER_OF_UNKNOWNS_PER_CELL" ] = number_of_unknows_per_cell
    self.d[ "DESCRIPTION" ]         = description
    self.d[ "TIMESTAMP" ]           = time_stamp_evaluation
    self.d[ "GUARD_PREDICATE" ]    = guard_predicate
    self.d[ "META_DATA" ]          = ""
    self.additional_includes       = additional_includes
        

  __Template_Constructor = """
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_EndTraversal = """
  assertion(_dataWriter!=nullptr);
  assertion1(_dataWriter!=nullptr,_treeNumber);
  
  _dataWriter->close();
  _writer->writeToFile();
  
  delete _dataWriter;
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;
"""


  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  Template_TouchCellFirstTime = """ 
if ( {{GUARD_PREDICATE}} ) {
  int cellIndex = _writer->plotPatch(
    marker.x()-0.5 * marker.h(),
    marker.h()
  );

  {% if NUMBER_OF_UNKNOWNS_PER_CELL<0 %}
  double data = static_cast<double>(fineGridCell{{CELL_UNKNOWN_NAME}}.{{GETTER}});
  {% else %}
  auto data = fineGridCell{{CELL_UNKNOWN_NAME}}.{{GETTER}};
  {% endif %}
  _dataWriter->plotCell( cellIndex, data );
}
"""


  __Template_BeginTraversal = """
  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "{{FILENAME}}-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  {% if TIMESTAMP==\"""" + NoMetaFile + """\" %}
  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{FILENAME}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::NoIndexFile,
    0.0
  );
  {% elif TIMESTAMP==\"""" + CountTimeSteps + """\" %}
  static int timeStep = -1;
  timeStep++;
  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{FILENAME}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    timeStep
  );
  {% else %}
  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{FILENAME}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    {{TIMESTAMP}}
  );    
  {% endif %}
    
  _dataWriter = _writer->createCellDataWriter( 
    "{{CELL_UNKNOWN_NAME}}", 
    1, // nodes per axis                                          
    {% if NUMBER_OF_UNKNOWNS_PER_CELL<0 %}
    1,                                          // records per cell
    {% else %}
    {{NUMBER_OF_UNKNOWNS_PER_CELL}},
    {% endif %}
    "{{DESCRIPTION}}",
    "{{META_DATA}}"
  );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template( self.Template_TouchCellFirstTime).render(**self.d) 
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = jinja2.Template( self.__Template_BeginTraversal).render(**self.d)             
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template( self.__Template_EndTraversal).render(**self.d)             
    return result


  def get_attributes(self):
    return """
    static tarch::mpi::BooleanSemaphore                                              _semaphore;
    
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                  _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _dataWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/utils/Loop.h"
#include "peano4/parallel/SpacetreeSet.h"
""" + self.additional_includes


  def get_static_initialisations(self,full_qualified_classname):
    return """
tarch::mpi::BooleanSemaphore  """ + full_qualified_classname + """::_semaphore;
"""
    
