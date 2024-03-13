# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PlotDGDataInPeanoBlockFormat(ActionSet):

  def __init__(self,
               solver
               ):
    """
    
      Plot the DG mesh
      
    """
    super( PlotDGDataInPeanoBlockFormat, self ).__init__()
    
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    #self.d[ "FILENAME" ]            = filename
    #self.d[ "CELL_UNKNOWN_NAME" ]   = cell_unknown.name
    #self.d[ "GETTER" ]              = getter
    #self.d[ "NUMBER_OF_UNKNOWNS_PER_CELL" ] = number_of_unknows_per_cell
    #self.d[ "DESCRIPTION" ]         = description
    #self.d[ "TIMESTAMP" ]           = time_stamp_evaluation
    #self.d[ "GUARD_PREDICATE" ]    = guard_predicate
    #self.d[ "META_DATA" ]          = ""
    #self.additional_includes       = additional_includes
        

  __Template_Constructor = """
  _writer          = nullptr;
  _valueWriter     = nullptr;
  _rhsWriter       = nullptr;
  _treeNumber      = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_EndTraversal = """
  assertion(_valueWriter!=nullptr);
  assertion1(_valueWriter!=nullptr,_treeNumber);
  
  assertion(_rhsWriter!=nullptr);
  assertion1(_rhsWriter!=nullptr,_treeNumber);

  _valueWriter->close();
  _rhsWriter->close();
  _writer->writeToFile();
  
  delete _valueWriter;
  delete _rhsWriter;
  delete _writer;

  _valueWriter = nullptr;
  _rhsWriter   = nullptr;
  _writer      = nullptr;
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
if (fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior) {
  int patchIndex = _writer->plotPatch(
    marker.x()-0.5 * marker.h(),
    marker.h()
  );

  int valuePatchVertexIndex  = _valueWriter->getFirstVertexWithinPatch(patchIndex);
  int rhsPatchVertexIndex    = _rhsWriter->getFirstVertexWithinPatch(patchIndex);
  
  assertionEquals(valuePatchVertexIndex, rhsPatchVertexIndex);

  
  int currentDoF = 0;
  dfor(k,repositories::{{SOLVER_INSTANCE}}.Order+1) {
    double* value = fineGridCell{{SOLVER_NAME}}.getValue().data();
    double* rhs   = fineGridCell{{SOLVER_NAME}}.getRhs().data();
    _valueWriter->plotVertex( valuePatchVertexIndex, value+currentDoF );
    _rhsWriter->plotVertex(   rhsPatchVertexIndex,   rhs+currentDoF   );
    valuePatchVertexIndex++;
    rhsPatchVertexIndex++;
    currentDoF += repositories::{{SOLVER_INSTANCE}}.CellUnknowns;
  }

}
"""


  __Template_BeginTraversal = """
  tarch::mpi::Lock lock( _semaphore );
  
  std::ostringstream snapshotFileName;
  snapshotFileName << "{{SOLVER_NAME}}";

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{FILENAME}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::NoIndexFile,
    0.0
  );
    
  _valueWriter = _writer->createVertexDataWriter( 
    "value", 
    repositories::{{SOLVER_INSTANCE}}.Order+1,  // nodes per axis                                          
    repositories::{{SOLVER_INSTANCE}}.CellUnknowns,
    "Solution",
    "Solution as delivered through PETSc"
  );
  _rhsWriter = _writer->createVertexDataWriter( 
    "rhs", 
    repositories::{{SOLVER_INSTANCE}}.Order+1,  // nodes per axis                                          
    repositories::{{SOLVER_INSTANCE}}.CellUnknowns,
    "Right-hand side",
    "Nodal right-hand side value"
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

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                    _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*  _valueWriter;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*  _rhsWriter;
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
    
