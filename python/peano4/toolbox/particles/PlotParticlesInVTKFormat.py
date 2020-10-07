# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class PlotParticlesInVTKFormat(ActionSet):
  def __init__(self,filename,particle):
    """
      Plot only the grid structure
      
      :Attibutes:
      
      filename: String
         Name of the output file.
         
      particle: @todo   
          
    """
    self.d = {}
    self.d[ "FILENAME" ]     = filename
    self.d[ "CELL_WRAPPER" ] = "Cell"
    #if cell_unknown!=None:
    #  self.d[ "CELL_WRAPPER" ] += cell_unknown.name
    

  __Template_Constructor = jinja2.Template("""
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
""")


  def get_constructor_body(self):
    return self.__Template_Constructor.render(**self.d)


  __Template_Destructor = jinja2.Template("""
  static int rankLocalCounter = 0;
  static tarch::multicore::BooleanSemaphore booleanSemaphore;
  
  if (_dataWriter!=nullptr and _treeNumber>=0) {
    _dataWriter->close();

    int counter;
    {
      tarch::multicore::Lock lock(booleanSemaphore);
      counter = rankLocalCounter;
      rankLocalCounter++;
    }

    std::ostringstream filename;
    filename << "{{FILENAME}}" << "-tree-" << _treeNumber << "-" << counter;
    _writer->writeToFile( filename.str() );
  }
  
  delete _dataWriter;
  delete _writer;

  _dataWriter = nullptr;
  _writer     = nullptr;
""")

    
  def get_destructor_body(self):
    return self.__Template_Destructor.render(**self.d)


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_TouchCellFirstTime = jinja2.Template(""" 
  int particleNumber = _writer->plotPoint(marker.x());

  assertion( _dataWriter!=nullptr );
  
  //_dataWriter->plotCell(indices,markerData);
""")


  __Template_BeginTraversal = jinja2.Template("""
  static bool calledBefore = false;
  
  _writer     = new tarch::plotter::pointdata::vtk::VTKBinaryFileWriter();
  _dataWriter = _writer->createPointDataWriter( "x", 3 );
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d) 
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginTraversal.render(**self.d)             
    return result


  def get_attributes(self):
    return """
    int                _treeNumber;

    tarch::plotter::pointdata::vtk::VTKBinaryFileWriter*      _writer;
    tarch::plotter::pointdata::PointWriter::PointDataWriter*  _dataWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/pointdata/vtk/VTKBinaryFileWriter.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/mpi/Lock.h"
#include "peano4/parallel/SpacetreeSet.h"
"""
