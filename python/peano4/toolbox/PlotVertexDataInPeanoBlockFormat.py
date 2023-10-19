# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PlotVertexDataInPeanoBlockFormat(ActionSet):
  NoMetaFile     = "no-meta-file"
  CountTimeSteps = "count-time-steps"

  def __init__(self,
               filename,
               vertex_unknown,
               getter,description,
               time_stamp_evaluation, 
               number_of_unknows_per_vertex=1,
               guard_predicate="true", 
               additional_includes=""
               ):
    """!
    
      Plot grid and assume all data are associated with vertices
      
      filename: String
        Name of the output file
        
      vertex_unknown: (DaStGen) object tied to a vertex 
        The object you have associated with a vertex and that you want to print.
        The code will access its name via 
        
             fineGridVertices{VERTEX_UNKNOWN_NAME}
             
        in the generated source code 
        
      getter: String (C++ code)
        Getter acting on the vertex. Could be something alike getU() for example.
        If there's no getter but you want to directly access the data, remove 
        any brackets from the passed string. Please note that Peano's plotter
        routines usually require plain value (double) pointers. So if a string
        like
        
              getU()
              
        returns a tarch vector, you have to pass in 
        
              getU().data()
              
        to make the getter compatible with plotters' interface.
        
      time_stamp_evaluation: String
        C++ expression returning a double. This is used to write the time series
        file for a series of snapshots. Pass in "0" if you solve a stationary 
        problem, e.g. You can also hand in the constants NoMetaFile or 
        CountTimeSteps.
        
      number_of_unknows_per_vertex: Integer
        Has to match to the getter.
        
    """
    super( PlotVertexDataInPeanoBlockFormat, self ).__init__()

    self.d = {}
    self.d[ "FILENAME" ]            = filename
    self.d[ "VERTEX_UNKNOWN_NAME" ] = vertex_unknown.name
    self.d[ "GETTER" ]              = getter
    self.d[ "NUMBER_OF_UNKNOWNS_PER_VERTEX" ]              = number_of_unknows_per_vertex
    self.d[ "DESCRIPTION" ]         = description
    self.d[ "TIMESTAMP" ]           = time_stamp_evaluation
    self.d[ "GUARD_PREDICATE" ]    = guard_predicate
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


  __Template_TouchCellFirstTime = """ 
if ( {{GUARD_PREDICATE}} ) {
  int vertexIndices[TwoPowerD];

  int patchIndex = _writer->plotPatch(
    marker.x()-0.5 * marker.h(),
    marker.h()
  );

  assertion( _dataWriter!=nullptr );
  int vertexIndex  = _dataWriter->getFirstVertexWithinPatch(patchIndex);
  dfor2(k)
    auto data = fineGridVertices{{VERTEX_UNKNOWN_NAME}}(kScalar).{{GETTER}};
    _dataWriter->plotVertex( vertexIndex, data );
    vertexIndex++;
  enddforx
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
    
  _dataWriter = _writer->createVertexDataWriter( 
    "{{VERTEX_UNKNOWN_NAME}}", 
    2, // number of dofs per axis per cell
    {{NUMBER_OF_UNKNOWNS_PER_VERTEX}},
    "{{DESCRIPTION}}" 
  );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template( self.__Template_TouchCellFirstTime).render(**self.d) 
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = jinja2.Template( self.__Template_BeginTraversal).render(**self.d)             
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template( self.__Template_EndTraversal).render(**self.d)             
    return result


  def get_attributes(self):
    return """
    static tarch::mpi::BooleanSemaphore                                              _semaphore;
    
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                    _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*  _dataWriter;
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
    
