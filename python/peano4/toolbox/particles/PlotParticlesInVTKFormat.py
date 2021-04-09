# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2





class PlotParticlesInVTKFormat(ActionSet):
  """

   By default, I plot the point particles (obviously) and drop their
   coordinates as vector, too. I also plot the cut-off radius which
   is a simple scalar. I often use it to colour-code or as radius
   of the rendered sphere.

   Finally, I do render the associativity. This is one vector per
   point that points towards the vertex the object belongs to. To
   visualise it, add a Glyph filter and select association as
   orientation array. Make association also the scale array and
   scale by magnitude.

  """
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
    self.d[ "FILENAME" ]                           = filename
    self.d[ "PARTICLE" ]                           = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]                = particle_set.name
    self.d[ "WRITE_BINARY" ]                       = "false"
    self.d[ "ATTRIBUTE_WRITER_NULL_INITIALISERS" ] = ""
    self.d[ "ATTRIBUTE_WRITER_CLOSES" ]            = ""
    self.d[ "ATTRIBUTE_WRITER_DELETES" ]           = ""
    self.d[ "ATTRIBUTE_WRITER_INITIALISERS" ]      = ""
    self.d[ "ATTRIBUTE_WRITER_PLOT_CALLS" ]        = ""
    self._attributes_to_plot = []

  __Template_Constructor = jinja2.Template("""
  _writer             = nullptr;
  _positionWriter     = nullptr;
  _cutOffWriter       = nullptr;
  _associationWriter  = nullptr;
  {{ATTRIBUTE_WRITER_NULL_INITIALISERS}}
  _treeNumber         = treeNumber;

  // An MPI lock (critical section) would be important!

  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
""")


  def add_attribute_to_plot(self, attribute, cardinality=1):
      self._attributes_to_plot.append(attribute)
      writer_name = f"{attribute.get_to_string()}Writer"
      delim = "\n  "
      name_no_underscore = attribute.get_to_string()[1:]
      accessor_name = name_no_underscore[0].title() + name_no_underscore[1:]

      self.d[ "ATTRIBUTE_WRITER_NULL_INITIALISERS" ] += f"{delim}{writer_name} = nullptr;"
      self.d[ "ATTRIBUTE_WRITER_CLOSES" ]            += f"{delim}{writer_name}->close();"
      self.d[ "ATTRIBUTE_WRITER_DELETES" ]           += f"{delim}delete {writer_name};"
      self.d[ "ATTRIBUTE_WRITER_INITIALISERS" ]      += f"{delim}{writer_name} = _writer->createPointDataWriter( \"{name_no_underscore}\", {cardinality} );"
      self.d[ "ATTRIBUTE_WRITER_PLOT_CALLS" ]        += f"{delim}  {writer_name}->plot(particleNumber,p->get{accessor_name}());"


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
  assertion( _positionWriter!=nullptr );
  for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}}) {
    int particleNumber = _writer->plotPoint(p->getX());
    _positionWriter->plot(particleNumber,p->getX());
    _cutOffWriter->plot(particleNumber,p->getCutOffRadius());
    _associationWriter->plot(particleNumber,marker.x()-p->getX());
    {{ATTRIBUTE_WRITER_PLOT_CALLS}}
  }
""")


  __Template_BeginTraversal = jinja2.Template("""
  static int counter = 0;

  static tarch::multicore::BooleanSemaphore localSemaphore;
  tarch::multicore::Lock  localLock( localSemaphore );

  int isFirstBarrierHitOnThisRank = ::peano4::parallel::SpacetreeSet::getInstance().synchroniseFirstThreadPerRank("{{FILENAME}}-init");

  std::ostringstream snapshotFileName;
  snapshotFileName << "{FILENAME}" << "-" << counter;

  if ( counter==0 and isFirstBarrierHitOnThisRank and tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{FILENAME}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::CreateNew
    );    
  }
  else if ( isFirstBarrierHitOnThisRank and tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{{FILENAME}}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewDataSet
    );
  }
  
  counter++;

  ::peano4::parallel::SpacetreeSet::getInstance().synchroniseFirstThreadPerRank("{FILENAME}-write");

  if ( _writer==nullptr ) {
    _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
      Dimensions, snapshotFileName.str(), "{{FILENAME}}",
      tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData
    );
  }
  
  _positionWriter    = _writer->createPointDataWriter( "x", 3 );
  _cutOffWriter      = _writer->createPointDataWriter( "cut-off-radius", 1 );
  _associationWriter = _writer->createPointDataWriter( "association", 3 );
  {{ATTRIBUTE_WRITER_INITIALISERS}}
""")


  __Template_EndTraversal = jinja2.Template("""
  assertion(_positionWriter!=nullptr);

  _positionWriter->close();
  _cutOffWriter->close();
  _associationWriter->close();
  {{ATTRIBUTE_WRITER_CLOSES}}
  _writer->writeToFile();

  delete _positionWriter;
  delete _cutOffWriter;
  delete _associationWriter;
  {{ATTRIBUTE_WRITER_DELETES}}
  delete _writer;

  _positionWriter    = nullptr;
  _cutOffWriter      = nullptr;
  _associationWriter = nullptr;
  {{ATTRIBUTE_WRITER_NULL_INITIALISERS}}
  _writer            = nullptr;
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


  def _get_attribute_writer_declarations(self):
      delim = "\n    "
      datatype = "tarch::plotter::pointdata::PointWriter::PointDataWriter*"
      return delim.join(f"{datatype}  {attr.get_to_string()}Writer;"
                          for attr in self._attributes_to_plot)


  def get_attributes(self):
    return f"""
    int                _treeNumber;

    tarch::plotter::pointdata::vtk::VTKWriter*                _writer;
    tarch::plotter::pointdata::PointWriter::PointDataWriter*  _positionWriter;
    tarch::plotter::pointdata::PointWriter::PointDataWriter*  _cutOffWriter;
    tarch::plotter::pointdata::PointWriter::PointDataWriter*  _associationWriter;
    {self._get_attribute_writer_declarations()}
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
