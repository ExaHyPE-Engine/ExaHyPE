# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PlotVertexDataInPeanoBlockFormat(ActionSet):

  def __init__(self,
               solver,
               variable_name,
               getter
               ):
    """!
    
      Plot grid and assume all data are associated with vertices
      
      This is a copy of the one in toolbox, but for the time being
      we hardcode the number of unknowns per vertex that we plot
      to 1, and we make some important customisations. Particularly
      regarding the place we plot.
        
    """
    super( PlotVertexDataInPeanoBlockFormat, self ).__init__()

    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d[ "VARIABLE" ]             = variable_name
    self.d[ "GETTER" ]               = getter
        
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
if ( fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse ) {
  int vertexIndices[TwoPowerD];

  int patchIndex = _writer->plotPatch(
    marker.x()-0.5 * marker.h(),
    marker.h()
  );

  int vertexIndex  = _dataWriter->getFirstVertexWithinPatch(patchIndex);
  for (int i=0; i<TwoPowerD; i++) {
    double* value = fineGridVertices{{SOLVER_NAME}}(i).{{GETTER}}().data();
    _dataWriter->plotVertex( vertexIndex+i, value );
  }
}
"""


  __Template_BeginTraversal = """
  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;
  
  std::ostringstream snapshotFileName;
  snapshotFileName << "{{SOLVER_NAME}}.{{VARIABLE}}-" << counter ;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{SOLVER_NAME}}.{{VARIABLE}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    counter
  );
    
  _dataWriter = _writer->createVertexDataWriter( 
    "{{VARIABLE}}", 
    2,  // number of dofs per axis per cell                                          
    repositories::{{SOLVER_INSTANCE}}.VertexUnknowns,
    "Solution",
    "Solution as delivered through MF Solver"
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
#include "repositories/SolverRepository.h"
"""


  def get_static_initialisations(self,full_qualified_classname):
    return """
tarch::mpi::BooleanSemaphore  """ + full_qualified_classname + """::_semaphore;
"""
    
