# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class PlotParticlesInVTKFormat(ActionSet):
  def __init__(self,filename,particle_set):
    """
      Plot only the grid structure
      
      :Attibutes:
      
      filename: String
         Name of the output file.
         
      particle: ParticleSet
         I take this as particle set and I assume that it yields a C++ 
         container which I can traverse.
          
    """
    self.d = {}
    self.d[ "FILENAME" ]            = filename
    self.d[ "PARTICLE" ]            = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ] = particle_set.name
    self.d[ "WRITE_BINARY" ]        = "false"

  __Template_Constructor = jinja2.Template("""
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;

  // An MPI lock (critical section) would be important!
    
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
""")


  def get_constructor_body(self):
    return self.__Template_Constructor.render(**self.d)


    
  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False

  __Template_TouchVertexFirstTime = jinja2.Template(""" 
  assertion( _dataWriter!=nullptr );
  for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}}) {
    int particleNumber = _writer->plotPoint(p.getX());
    _dataWriter->plot(particleNumber,p.getX());
  }
""")


  __Template_BeginTraversal = jinja2.Template("""
  static bool calledBefore = false;
  static tarch::mpi::BooleanSemaphore globalSemaphore("{{FILENAME}}");
  
  // This stuff only ensures that the overview file remains correct. The 
  // sync and add of the individual dumps is then managed within the actual
  // plotter files.
  if ( _treeNumber==0 and not calledBefore ) {
    calledBefore = true;
    _writer     = new tarch::plotter::pointdata::vtk::VTKWriter({{WRITE_BINARY}},"{{FILENAME}}",tarch::plotter::VTUTimeSeriesWriter::IndexFileMode::CreateNew);
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{{FILENAME}}");
  }
  else if ( _treeNumber==0 ) {
    _writer     = new tarch::plotter::pointdata::vtk::VTKWriter({{WRITE_BINARY}},"{{FILENAME}}",tarch::plotter::VTUTimeSeriesWriter::IndexFileMode::AppendNewDataSet);
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{{FILENAME}}");
  }
  else {
    ::peano4::parallel::SpacetreeSet::getInstance().orderedBarrier("{{FILENAME}}");
    _writer     = new tarch::plotter::pointdata::vtk::VTKWriter({{WRITE_BINARY}},"{{FILENAME}}",tarch::plotter::VTUTimeSeriesWriter::IndexFileMode::AppendNewData);
  }

  _dataWriter = _writer->createPointDataWriter( "x", 3 );
""")


  __Template_EndTraversal = jinja2.Template("""
  static int rankLocalCounter = 0;
  static tarch::multicore::BooleanSemaphore booleanSemaphore;
  
  assertion(_dataWriter!=nullptr);
  
  _dataWriter->close();
  
  if (_treeNumber>=0) {
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

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_TouchVertexFirstTime.render(**self.d) 
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginTraversal.render(**self.d)             
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d)             
    return result


  def get_attributes(self):
    return """
    int                _treeNumber;

    tarch::plotter::pointdata::vtk::VTKWriter*                _writer;
    tarch::plotter::pointdata::PointWriter::PointDataWriter*  _dataWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/pointdata/vtk/VTKWriter.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/mpi/Lock.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "../vertexdata/""" + self.d["PARTICLES_CONTAINER"] + """.h"
#include "../globaldata/""" + self.d["PARTICLE"] + """.h"
"""
