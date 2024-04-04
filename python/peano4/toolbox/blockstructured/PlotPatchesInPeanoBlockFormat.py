# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PlotPatchesInPeanoBlockFormat(ActionSet):
  """!
  
  Simple patch plotter
  
  Very simple plotter that should be used in combination with Patches.
  Patches with varying datatypes are supported via the "dataType" parameter,
  but this defaults to doubles. This parameter should contain the C-style-
  name of the type of the variable in the patch, e.g. float, double, etc.
  This plotter works only for cell patches, i.e. patches associated with 
  faces or vertices are not supported.
  
  At the moment, I can only plot one dataset (patch type) per plotter.
  In theory, the underlying Peano plotter however could dump multiple
  patches at once.
  
  Also, this dump expects incoming data to be AoS.
  """
  
  def __init__(self,
               filename,
               patch,
               dataset_name, 
               description, time_stamp_evaluation, 
               plot_cell_data=True, 
               metadata = "", 
               mapping = [], 
               guard="true", 
               additional_includes="", 
               precision=3,
               select_dofs = None,
               dataType="double"):
    """!
    
      Construct the block plotter
    
      plot_cell_data: Boolean
        Shall I plot cell data or vertex data. If you map the patches onto
        vertex data, then I have to decrease the dofs pre axis, as we basically
        plot the dual grid.
    
      description: String
      
      mapping: Series of d-tuples which describe how to distort quadrature/sampling
        points within a reference cube/square. Can be empty alternatively.
         
      select_dofs: [Int] or None
        You can make the plotter only plot some of the dofs of interest
           
    """
    super(PlotPatchesInPeanoBlockFormat,self).__init__(descend_invocation_order=1,parallel=False)

    valid_datatypes = {"float", "double", "long double", "std::float16_t", "std::bfloat16_t"}

    self._plot_cell_data           = plot_cell_data
    
    self.d = {}
    self.d[ "FILENAME" ]           = filename
    self.d[ "GUARD_PREDICATE" ]    = guard
    
    self.additional_includes       = additional_includes

    dofs_per_axis = patch.dim[0]
      
    self.d[ "DOFS_PER_AXIS" ]      = str(dofs_per_axis)

    if select_dofs==None:     
      self.d[ "PLOTTED_UNKNOWNS" ]         = str(patch.no_of_unknowns)
    else:
      self.d[ "PLOTTED_UNKNOWNS" ]         = str(len(select_dofs))
    self.d[ "UNKNOWNS" ]           = str(patch.no_of_unknowns)
    self.d[ "SELECT_DOFS" ]        = select_dofs
    
    self.d[ "NAME" ]               = dataset_name
    self.d[ "DESCRIPTION" ]        = description
    self.d[ "METADATA" ]           = metadata
    if len(mapping)==0:
      self.d[ "MAPPING_2D"]        = "double* mapping = nullptr;"
      self.d[ "MAPPING_3D"]        = "double* mapping = nullptr;"
    else:
      counter = 0
      self.d[ "MAPPING_2D"]        = "double mapping[" + str(2*dofs_per_axis*dofs_per_axis) + "] = {"
      self.d[ "MAPPING_3D"]        = "double mapping[" + str(3*dofs_per_axis*dofs_per_axis*dofs_per_axis) + "] = {"
      for i in mapping:
        if counter>0:
          separator = ", "
        else:
          separator = " "
        if counter<dofs_per_axis*dofs_per_axis:
          self.d[ "MAPPING_2D"]     += separator + str(i[0])
          self.d[ "MAPPING_2D"]     += ", " + str(i[1])
          self.d[ "MAPPING_2D"]     += "\n"
        self.d[ "MAPPING_3D"]     += separator + str(i[0])
        self.d[ "MAPPING_3D"]     += ", " + str(i[1])
        if len(i)>2:
          self.d[ "MAPPING_3D"]     += ", " + str(i[2])
        else:
          self.d[ "MAPPING_3D"]     += ", 0.0"
        self.d[ "MAPPING_3D"]     += "\n"
        counter += 1

      self.d[ "MAPPING_2D"]        += "};"
      self.d[ "MAPPING_3D"]        += "};"
        
    for i in patch.dim:
      if i!=patch.dim[0]:
        print( "Error: patch plotter requires patch to have same dimension along all coordinate axes")

    if dataType not in valid_datatypes:
      raise Exception("Chosen datatype " + datatype + " for plotting of patches is not a recognized choice.")

    self.d[ "PRECISION" ]        = precision
    self.d[ "DATATYPE"  ]        = dataType
    self.d[ "TIMESTAMP" ]        = time_stamp_evaluation


  __Template_Constructor = """
  _writer      = nullptr;
  _dataWriter  = nullptr;
  _treeNumber  = treeNumber;
  
  logDebug( "PlotGrid2PlotGridInPeanoBlockFormat1()", "created tree instance for " << treeNumber );
"""


  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_EndTraversal = """
  assertion1( _dataWriter!=nullptr, _treeNumber );
  assertion1( _writer!=nullptr,     _treeNumber );

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


  __Template_TouchCellFirstTime_CellPlot = """ 
  if ( {{GUARD_PREDICATE}} ) {
    int vertexIndices[TwoPowerD];
  
    const double PatchScaling = 1.0;

    assertion( _writer!=nullptr );
    assertion( _dataWriter!=nullptr );
  
    const int patchIndex = _writer->plotPatch(
      marker.x() - marker.h() * PatchScaling * 0.5,
      marker.h() * PatchScaling
    );
 
    int cellIndex  = _dataWriter->getFirstCellWithinPatch(patchIndex);
    int currentDoF = 0;
  
    dfor(k,{{DOFS_PER_AXIS}}) {
      {% if SELECT_DOFS==None %}
      {{DATATYPE}}* data = fineGridCell{{NAME}}.value + currentDoF;
      {% else %}
      {{DATATYPE}} data[] = { 
          fineGridCell{{NAME}}.value[ currentDoF + {{SELECT_DOFS[0]}} ]
        {% for i in SELECT_DOFS[1:] %}
        , fineGridCell{{NAME}}.value[ currentDoF + {{i}} ]
        {% endfor %}
      };
      {% endif %}
      _dataWriter->plotCell( cellIndex, data );
      cellIndex++;
      currentDoF += {{UNKNOWNS}};
    }
  }
"""

#    self.d[ "SELECT_DOFS" ]        = select_dofs


  __Template_TouchCellFirstTime_VertexPlot = """ 
  if ( {{GUARD_PREDICATE}} ) {
    int vertexIndices[TwoPowerD];
  
    const double PatchScaling = 1.0;

    assertion( _writer!=nullptr );
    assertion( _dataWriter!=nullptr );
  
    const int patchIndex = _writer->plotPatch(
      marker.x() - marker.h() * PatchScaling * 0.5,
      marker.h() * PatchScaling
    );
 
    int vertexIndex  = _dataWriter->getFirstVertexWithinPatch(patchIndex);
    int currentDoF = 0;
  
    dfor(k,{{DOFS_PER_AXIS}}) {
      {% if SELECT_DOFS==None %}
      {{DATATYPE}}* data = fineGridCell{{NAME}}.value + currentDoF;
      {% else %}
      {{DATATYPE}} data[] = { 
          fineGridCell{{NAME}}.value[ currentDoF + {{SELECT_DOFS[0]}} ]
        {% for i in SELECT_DOFS[1:] %}
        , fineGridCell{{NAME}}.value[ currentDoF + {{i}} ]
        {% endfor %}
      };
      {% endif %}
      _dataWriter->plotVertex( vertexIndex, data );
      vertexIndex++;
      currentDoF += {{UNKNOWNS}};
    }
  }
"""


  __Template_BeginTraversal_Generic = """
  tarch::mpi::Lock lock( _semaphore );
  
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "{{FILENAME}}-" << counter;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 ) {
    snapshotFileName << "-rank-" << tarch::mpi::Rank::getInstance().getRank();
  }

  _writer = new tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions, snapshotFileName.str(), "{{FILENAME}}",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    {{TIMESTAMP}}
  );    
      
  #if Dimensions==2
  {{MAPPING_2D}}
  #else
  {{MAPPING_3D}}
  #endif
"""



  __Template_BeginTraversal_CellPlot = __Template_BeginTraversal_Generic + """
  _dataWriter = _writer->createCellDataWriter( "{{NAME}}", {{DOFS_PER_AXIS}}, {{PLOTTED_UNKNOWNS}}, "{{DESCRIPTION}}", "{{METADATA}}", mapping );
  _dataWriter->setPrecision( {{PRECISION}} );
"""


  __Template_BeginTraversal_VertexPlot = __Template_BeginTraversal_Generic + """
  _dataWriter = _writer->createVertexDataWriter( "{{NAME}}", {{DOFS_PER_AXIS}}, {{PLOTTED_UNKNOWNS}}, "{{DESCRIPTION}}", "{{METADATA}}", mapping );
  _dataWriter->setPrecision( {{PRECISION}} );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME and self._plot_cell_data:
      result = jinja2.Template( self.__Template_TouchCellFirstTime_CellPlot ).render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME and not self._plot_cell_data:
      result = jinja2.Template( self.__Template_TouchCellFirstTime_VertexPlot ).render(**self.d)
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL and self._plot_cell_data:
      result = jinja2.Template( self.__Template_BeginTraversal_CellPlot ).render(**self.d)             
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL and not self._plot_cell_data:
      result = jinja2.Template( self.__Template_BeginTraversal_VertexPlot ).render(**self.d)             
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template( self.__Template_EndTraversal ).render(**self.d)             
    return result


  def get_attributes(self):
    if self._plot_cell_data:
      return """
    static tarch::mpi::BooleanSemaphore                                              _semaphore;
      
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                  _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _dataWriter;
"""
    else:
      return """
    static tarch::mpi::BooleanSemaphore                                              _semaphore;
    
    int                _treeNumber;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                    _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*  _dataWriter;
"""


  def get_includes(self):
    return """
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "peano4/utils/Loop.h"
#include "peano4/parallel/SpacetreeSet.h"
""" + self.additional_includes


  def get_static_initialisations(self,full_qualified_classname):
    return """
tarch::mpi::BooleanSemaphore  """ + full_qualified_classname + """::_semaphore;
"""
    
