# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

from .Jinja2TemplatedHeaderFile import Jinja2TemplatedHeaderFile


import os
import re



class Observer(object):
  default_overwrite = True
  
  
  def __init__(self,classname,namespace,subdirectory,included_actions,vertices,faces,cells):
    """
      Included actions is a list of qualified actions which are used
    """
    self.classname    = classname
    self.namespace    = namespace
    self.subdirectory = subdirectory 
    
    self.included_actions = included_actions
    self.vertices          = vertices
    self.faces             = faces
    self.cells             = cells
    
    self.d = {}
    self.d[ "OPEN_NAMESPACE" ]           = ""
    self.d[ "CLOSE_NAMESPACE" ]          = ""
    self.d[ "FULL_QUALIFIED_CLASSNAME" ] = ""
    for i in namespace:
      self.d[ "OPEN_NAMESPACE" ]           += "namespace " + i + "{\n"
      self.d[ "CLOSE_NAMESPACE" ]          += "}\n"
      self.d[ "FULL_QUALIFIED_CLASSNAME" ] += i + "::"
    self.d[ "CLASSNAME" ]                 = classname
    self.d[ "FULL_QUALIFIED_CLASSNAME" ] += classname
    self.d[ "INCLUDE_GUARD" ]             = "_" + self.d[ "FULL_QUALIFIED_CLASSNAME" ].replace( "::", "_" ).upper() + "_H_"
    self.d[ "INCLUDES" ]                  = ""
    self.d[ "ATTRIBUTES" ]                = ""

    for action in self.included_actions:
      self.d[ "INCLUDES" ] += "#include \""
      self.d[ "INCLUDES" ] += action.replace( "::", "/" )
      self.d[ "INCLUDES" ] += ".h\"\n"
      self.d[ "ATTRIBUTES" ] += "    "
      self.d[ "ATTRIBUTES" ] += action
      self.d[ "ATTRIBUTES" ] += "   _actionSet"
      self.d[ "ATTRIBUTES" ] += str( self.included_actions.index(action) )
      self.d[ "ATTRIBUTES" ] += "; \n"

    self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS" ]       = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS" ]     = ""
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]       = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]     = ""
    for cell in cells:
      if cells.index(cell)!=0:
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS" ]   += ",";
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS" ] += ",";
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]   += ",";
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ] += ",";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS" ]     += "DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS" ]   += "DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]     += "DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]   += "DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)";
      
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_CELL_EVENT" ]       = self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS" ]
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_CELL_EVENT" ]     = self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS" ]
           
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS" ]      = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS" ]    = ""
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]      = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]    = ""
    for face in faces:
      if faces.index(face)!=0:
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS" ]   += ","
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS" ] += ","
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]   += ","
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ] += ","
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS" ]     += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS" ]   += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]     += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]   += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )(pick)";
      
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS" ]   = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS" ] = ""
    self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]   = ""
    self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ] = ""
    for vertex in vertices:
      if vertices.index(vertex)!=0:
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS" ]   += ","
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS" ] += ","
        self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]   += ","
        self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ] += ","
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS" ]     += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS" ]   += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]     += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]   += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )(pick)";

    #
    # Create powersets of arguments
    #
    for i in range(0,2):
      new_dictionary_entries = {}
      for keyA in self.d:
        for keyB in self.d:
          new_entry = ""
          if self.d[keyA]!="" and self.d[keyB]!="":
            new_entry = self.d[keyA] + "," + self.d[keyB]
          else:
            new_entry = self.d[keyA] + self.d[keyB]
          new_dictionary_entries[keyA+","+keyB] = new_entry
      for key in new_dictionary_entries:
        self.d[key] = new_dictionary_entries[key]
    
    #
    # Create variants with leading and tailing comma
    #
    new_dictionary_entries = {}
    for key in self.d:
      if self.d[key]!="":
        new_dictionary_entries[ key+","     ] = self.d[key] + ","
        new_dictionary_entries[ ","+key     ] = "," + self.d[key] 
        new_dictionary_entries[ ","+key+"," ] = "," + self.d[key] + ","
      else:
        new_dictionary_entries[ key+"," ]       = ""
        new_dictionary_entries[ "," + key ]     = ""
        new_dictionary_entries[ "," + key+"," ] = ""
        
    for key in new_dictionary_entries:
      self.d[key] = new_dictionary_entries[key]


  def __generate_header(self,overwrite,directory):
    headerfile_template = os.path.realpath(__file__).replace( ".pyc", ".h.template" ).replace( ".py", ".h.template" )
    header = Jinja2TemplatedHeaderFile(headerfile_template,self.classname,self.namespace,self.subdirectory,self.d,self.default_overwrite)
    header.generate(overwrite,directory)
    


  TemplateConstructor = """
  
{FULL_QUALIFIED_CLASSNAME}::{CLASSNAME}(int spacetreeId):
  _spacetreeId( spacetreeId ) {MAPPING_INITIALISATION_LIST}
{{
}}
  

  """


  def __generate_constructor(self,output_file):
    #
    # Constructor
    #
    self.d[ "MAPPING_INITIALISATION_LIST" ]                = ""
    for actions in self.included_actions:
      self.d[ "MAPPING_INITIALISATION_LIST" ]  += ", _actionSet"
      self.d[ "MAPPING_INITIALISATION_LIST" ]  += str( self.included_actions.index(actions) )
      self.d[ "MAPPING_INITIALISATION_LIST" ]  += "(spacetreeId)"
    output_file.write( self.TemplateConstructor.format(**self.d) )


  TemplateClone = """
  
peano4::grid::TraversalObserver* {FULL_QUALIFIED_CLASSNAME}::clone(int spacetreeId) {{
  return new {CLASSNAME}(spacetreeId);
}}
  
  """


  TemplateBeginTraversal = """
  
void {FULL_QUALIFIED_CLASSNAME}::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {{
  logTraceInWith2Arguments( "beginTraversal(...)", x, h );
  //
  // Invoke beginTraversal() on the actions
  //
{MAPPING_BEGIN_TRAVERSAL_CALLS}

  //
  // Fill call stacks with dummies which represent level -1 such that we can 
  // call standard action routines on level 0 with parents. Without these 
  // statements, a top(1) call would raise an assertion. 
  //
{INITIAL_PUSH_TO_OUTPUT_STREAMS}
  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}}
  
  """

    
  def __generate_beginTraversal(self,output_file):
    self.d[ "MAPPING_BEGIN_TRAVERSAL_CALLS" ]     = ""
    for action in self.included_actions:
      self.d[ "MAPPING_BEGIN_TRAVERSAL_CALLS" ]  += "  _actionSet"
      self.d[ "MAPPING_BEGIN_TRAVERSAL_CALLS" ]  += str( self.included_actions.index(action) )
      self.d[ "MAPPING_BEGIN_TRAVERSAL_CALLS" ]  += ".beginTraversal();\n"
      
    self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    = ""
    for cell in self.cells:
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + cell.get_full_qualified_type() + "() );\n"
      pass

    for face in self.faces:
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  for (int i=0; i<TwoTimesD; i++) {\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "    DataRepository::_" + face.get_logical_type_name() + "Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + face.get_full_qualified_type() + "() );\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  }\n"
      pass

    for vertex in self.vertices:
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  for (int i=0; i<TwoPowerD; i++) {\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "    DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + vertex.get_full_qualified_type() + "() );\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  }\n"
      pass

    output_file.write( self.TemplateBeginTraversal.format(**self.d) )


  TemplateEndTraversal = """
  
void {FULL_QUALIFIED_CLASSNAME}::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {{
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
{MAPPING_END_TRAVERSAL_CALLS}
{FINAL_POP_FROM_INPUT_STREAMS}
  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}}
  
  """

    
  def __generate_endTraversal(self,output_file):
    self.d[ "MAPPING_END_TRAVERSAL_CALLS" ]                = ""
    for action in self.included_actions:
      self.d[ "MAPPING_END_TRAVERSAL_CALLS" ]  += "  _actionSet"
      self.d[ "MAPPING_END_TRAVERSAL_CALLS" ]  += str( self.included_actions.index(action) )
      self.d[ "MAPPING_END_TRAVERSAL_CALLS" ]  += ".endTraversal();\n"

    self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    = ""
    for cell in self.cells:
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      pass

    for face in self.faces:
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  for (int i=0; i<TwoTimesD; i++) {\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "    DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  }\n"
      pass

    for vertex in self.vertices:
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  for (int i=0; i<TwoPowerD; i++) {\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "    DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  }\n"
      pass

    
    output_file.write( self.TemplateEndTraversal.format(**self.d) )
    


  def __format_template_per_action(self,output_file,template,reverse_order=False):
    """
     Takes the specified template file, iterates over actions and pastes
     the template into the output file once per action. Per action, the dictionary's
     entries are updated. Otherwise, the dictionary remains unchanged.
    """ 
    local_actions = [x for x in self.included_actions]
    if reverse_order:
      local_actions.reverse()
        
    for action in local_actions:
      self.d[ "ACTIVE_ACTION_SET" ]  = "_actionSet" + str( self.included_actions.index(action) )
      self.d[ "ACTIVE_ACTION_SET_FULL_QUALIFIED_NAME" ]  = action
      output_file.write( template.format(**self.d) )
        

  TemplateGetGridControlEvents_Prologue = """
  
std::vector< peano4::grid::GridControlEvent > {FULL_QUALIFIED_CLASSNAME}::getGridControlEvents() const {{
  std::vector< peano4::grid::GridControlEvent > result;
"""

  TemplateGetGridControlEvents_MappingCall = """
  {{
    const std::vector< peano4::grid::GridControlEvent > actionResult = {ACTIVE_ACTION_SET}.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }}
"""        

  
  TemplateGetGridControlEvents_Epilogue = """
  return result;
}}
  
"""
  
        
  def __generate_getGridControlEvents(self,output_file):
    output_file.write( self.TemplateGetGridControlEvents_Prologue.format(**self.d) )
    self.__format_template_per_action(output_file,self.TemplateGetGridControlEvents_MappingCall)
    output_file.write( self.TemplateGetGridControlEvents_Epilogue.format(**self.d) )


  def __generate_clone(self,output_file):
    output_file.write( self.TemplateClone.format(**self.d) )


  TemplateEnterCell_Prologue = """  
void {FULL_QUALIFIED_CLASSNAME}::enterCell( const peano4::grid::GridTraversalEvent&  event ) {{
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );
"""


  TemplateEnterCell_VertexLoad_Prologue = """
  // Load vertex {name}
  {{
    auto view = DataRepository::_{logical_type_name}Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {{
      int inVertexStack          = event.getVertexDataFrom(i);
      int outVertexStackPosition = event.getVertexDataTo(i);
      logDebug("enterCell(...)", "vertex stack " << inVertexStack << "->pos-" << outVertexStackPosition );

      peano4::datamanagement::VertexMarker  marker(event,outVertexStackPosition);

      {full_qualified_type} data;
      if (
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        {full_qualified_type}::loadPersistently(marker)
      ) {{
        assertion4( not DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inVertexStack))->empty(), event.toString(), peano4::datamanagement::VertexMarker(event).toString(), _spacetreeId, inVertexStack);
        data = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inVertexStack))->pop();
      }}
   
      #if PeanoDebug>0  
      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inVertexStack==peano4::grid::TraversalObserver::NoData
        or
        not {full_qualified_type}::loadPersistently(marker)
      ) {{
        data.setDebugX( marker.x(outVertexStackPosition) );
        data.setDebugH( marker.h() );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
      ) {{
        assertionVectorNumericalEquals4( data.getDebugX(), marker.x(outVertexStackPosition), data.getDebugX(), marker.toString(), outVertexStackPosition, _spacetreeId );
        assertionVectorNumericalEquals3( data.getDebugH(), marker.h(), data.getDebugX(), marker.toString(), _spacetreeId );
      }}
      #endif

      view.set(outVertexStackPosition,data);
    }}
  }}
"""


  TemplateEnterCell_VertexLoad_MappingCall = """
  // Handle vertex {name}
  {{
    peano4::datamanagement::VertexMarker  marker(event);
    
    for (int i=0; i<TwoPowerD; i++) {{
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);
      
      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {{
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$ 
        // but only half of it.
        {ACTIVE_ACTION_SET}.createPersistentVertex(
           marker.select(pick) @pick raus
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchVertexFirstTime(
           marker.select(pick)
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.createHangingVertex(
           marker.select(pick)
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchVertexFirstTime(
           marker.select(pick)
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack
        and
        marker.isLocal()
      )) {{
        {ACTIVE_ACTION_SET}.touchVertexFirstTime(
           marker.select(pick)
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""


  TemplateEnterCell_FaceLoad_Prologue = """  
  // Load face {name}
  {{
    auto view = DataRepository::_{logical_type_name}Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {{
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("enterCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);
      
      {full_qualified_type} data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        {full_qualified_type}::loadPersistently(marker)
      ) {{
        assertion4( not DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }}
      
      #if PeanoDebug>0  
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not {full_qualified_type}::loadPersistently(marker)
      ) {{
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
      ) {{
        assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
      }}
      #endif

      view.set(outFaceStackPosition,data);
    }}
  }}
"""


  TemplateEnterCell_FaceLoad_MappingCall = """  
  // Handle face {name}
  {{
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {{
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.createPersistentFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchFaceFirstTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.createHangingFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchFaceFirstTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchFaceFirstTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""


  TemplateEnterCell_CellLoad_Prologue = """  
  // Load cell {name}
  {{
    auto view = DataRepository::_{logical_type_name}Stack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );
    
    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("enterCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << {full_qualified_type}::loadPersistently(marker) << ")" );
    
    peano4::datamanagement::CellMarker  marker(event);
    
    {full_qualified_type}& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      {full_qualified_type}::loadPersistently(marker)
    ) {{
      assertion3( not DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }}
    
    #if PeanoDebug>0  
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not {full_qualified_type}::loadPersistently(marker)
    ) {{
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }}
    else {{
      assertionVectorNumericalEquals3( data.getDebugX(), marker.x(), data.getDebugX(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals3( data.getDebugH(), marker.h(), data.getDebugX(), marker.toString(), _spacetreeId );
    }}
    #endif
  }}
"""


  TemplateEnterCell_CellLoad_MappingCall = """  
  // Invoke creational events on cell {name}
  {{
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {{
      {ACTIVE_ACTION_SET}.createCell(
        marker,
        {MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,}
        {MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,}
        {MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY}
        {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS}
        {,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS}
        {,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
      );
    }}  
  }}
"""


  TemplateEnterCell_MappingCall = """  
  {{
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {{
      {ACTIVE_ACTION_SET}.touchCellFirstTime(
         marker
        {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_CELL_EVENT}
        {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_CELL_EVENT}
      );
    }}
  }}
"""


  TemplateEnterCell_Epilogue = """
  logTraceOut( "enterCell(...)" );
}}
"""

  
  def __generate_enterCell(self,output_file):
    """
      Generates enter cell
    """
    output_file.write( self.TemplateEnterCell_Prologue.format(**self.d) )
          
    for vertex in self.vertices:
      self.d[ "name" ]                 = vertex.name
      self.d[ "enumeration_type" ]     = vertex.get_enumeration_type()
      self.d[ "logical_type_name" ]    = vertex.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = vertex.get_full_qualified_type()
      output_file.write( self.TemplateEnterCell_VertexLoad_Prologue.format(**self.d) )
    if len(self.vertices)>0:
      self.__format_template_per_action(output_file, self.TemplateEnterCell_VertexLoad_MappingCall, False)

    for face in self.faces:
      self.d[ "name" ]                 = face.name
      self.d[ "enumeration_type" ]     = face.get_enumeration_type()
      self.d[ "logical_type_name" ]    = face.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = face.get_full_qualified_type()
      output_file.write( self.TemplateEnterCell_FaceLoad_Prologue.format(**self.d) )
    if len(self.faces)>0:
      self.__format_template_per_action(output_file, self.TemplateEnterCell_FaceLoad_MappingCall, False)

    for cell in self.cells:
      self.d[ "name" ]                 = cell.name
      #self.d[ "enumeration_type" ]     = cell.get_enumeration_type()
      self.d[ "logical_type_name" ]    = cell.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = cell.get_full_qualified_type()
      output_file.write( self.TemplateEnterCell_CellLoad_Prologue.format(**self.d) )
    if len(self.cells)>0:
      self.__format_template_per_action(output_file, self.TemplateEnterCell_CellLoad_MappingCall, False)


    self.__format_template_per_action(output_file, self.TemplateEnterCell_MappingCall, False)
    output_file.write( self.TemplateEnterCell_Epilogue.format(**self.d) )


  TemplateLeaveCell_Prologue = """  
void {FULL_QUALIFIED_CLASSNAME}::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {{
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
"""



  TemplateLeaveCell_MappingCall = """  
  {{
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {{
      {ACTIVE_ACTION_SET}.touchCellLastTime(
         marker
        {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_CELL_EVENT}
        {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_CELL_EVENT}
      );
    }}
  }}
"""


  TemplateLeaveCell_CellStore_MappingCall = """
  {{
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {{
    {ACTIVE_ACTION_SET}.destroyCell(
      marker
        {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS}
        ,{MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY}
        {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
      ); 
    }}
  }}
"""


  TemplateLeaveCell_CellStore_Epilogue = """  
  // Handle cell {name}
  {{
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("leaveCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      {full_qualified_type}::storePersistently(marker)
    ) {{
      DataRepository::_{logical_type_name}Stack.getForPush( DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }}
  }}
"""


  TemplateLeaveCell_FaceStore_MappingCall = """
  // Handle face {name}
  {{
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {{
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchFaceLastTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.destroyPersistentFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchFaceLastTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.destroyHangingFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchFaceLastTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""

  TemplateLeaveCell_FaceStore_Epilogue = """
  // Store face {name}
  {{
    auto view = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {{
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("leaveCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );
      
      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      {full_qualified_type} data = view.get(inFaceStackPosition);
      
      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        {full_qualified_type}::storePersistently(marker)
      ) {{
        DataRepository::_{logical_type_name}Stack.getForPush( DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }}
    }}
  }}
"""

  TemplateLeaveCell_VertexStore_MappingCall = """
  // Handle vertex {name}
  {{
    peano4::datamanagement::VertexMarker  marker(event);
    
    for (int i=0; i<TwoPowerD; i++) {{
      int outVertexStack        = event.getVertexDataTo(i);
      int inVertexPosition      = event.getVertexDataFrom(i);

      marker.select(inVertexPosition);
      
      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchVertexLastTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.destroyPersistentVertex(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchVertexLastTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.destroyHangingVertex(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchVertexLastTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""

  TemplateLeaveCell_VertexStore_Epilogue = """
  // Store vertex {name}
  {{
    auto view = DataRepository::_{logical_type_name}Stack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {{
      int inVertexStackPosition  = event.getVertexDataFrom(i);
      int outVertexStack         = event.getVertexDataTo(i);
      logDebug("leaveCell(...)", "pos-" << inVertexStackPosition << "->vertex stack " << outVertexStack );
      
      peano4::datamanagement::VertexMarker  marker(event,inVertexStackPosition);

      {full_qualified_type} data = view.get(inVertexStackPosition);
  
      if (
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        {full_qualified_type}::storePersistently(marker)
      ) {{
        DataRepository::_{logical_type_name}Stack.getForPush(DataRepository::DataKey(_spacetreeId,outVertexStack))->push(data);
      }}
    }}
  }}
"""


  TemplateLeaveCell_Epilogue = """  
  logTraceOut( "leaveCell(...)" );
}}
"""


  def __generate_leaveCell(self,output_file):
    """
      Generates enter cell
    """
    output_file.write( self.TemplateLeaveCell_Prologue.format(**self.d) )
    self.__format_template_per_action(output_file, self.TemplateLeaveCell_MappingCall, True)
          
    if len(self.cells)>0:
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_CellStore_MappingCall, True)
    for cell in self.cells:
      self.d[ "name" ]                 = cell.name
      self.d[ "logical_type_name" ]    = cell.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = cell.get_full_qualified_type()
      output_file.write( self.TemplateLeaveCell_CellStore_Epilogue.format(**self.d) )

    if len(self.faces):
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_FaceStore_MappingCall, True)
    for face in self.faces:
      self.d[ "name" ]                 = face.name
      self.d[ "enumeration_type" ]     = face.get_enumeration_type()
      self.d[ "logical_type_name" ]    = face.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = face.get_full_qualified_type()
      output_file.write( self.TemplateLeaveCell_FaceStore_Epilogue.format(**self.d) )
        
    if len(self.vertices)>0:
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_VertexStore_MappingCall, True)
    for vertex in self.vertices:
      self.d[ "name" ]                 = vertex.name
      self.d[ "enumeration_type" ]     = vertex.get_enumeration_type()
      self.d[ "logical_type_name" ]    = vertex.get_logical_type_name()
      self.d[ "full_qualified_type" ]  = vertex.get_full_qualified_type()
      output_file.write( self.TemplateLeaveCell_VertexStore_Epilogue.format(**self.d) )

    output_file.write( self.TemplateLeaveCell_Epilogue.format(**self.d) )


  TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::exchangeAllVerticalDataExchangeStacks( int masterId ) {{
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );
"""

  TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange = """
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    {DATASET},
    _spacetreeId,
    masterId
  );
"""

  TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Epilogue = """
  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}}
"""




  TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {{
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );
"""

  TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange = """
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    {DATASET},
    _spacetreeId,
    symmetricDataCardinality
  );
"""

  TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Epilogue = """
  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}}
"""




  TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::exchangeAllPeriodicBoundaryDataStacks() {{
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );
"""

  TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange = """
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    {DATASET},
    _spacetreeId
  );
"""

  TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Epilogue = """
  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}}
"""




  TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::streamDataFromSplittingTreeToNewTree(int newWorker) {{
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );
"""

  TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange = """
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    {DATASET},
    _spacetreeId,
    newWorker
  );
"""

  TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Epilogue = """
  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}}
"""


  TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::streamDataFromJoiningTreeToMasterTree(int master) {{
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );
"""

  TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange = """
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    {DATASET},
    _spacetreeId,
    master
  );
"""

  TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Epilogue = """
  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}}
"""


  TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::finishAllOutstandingSendsAndReceives() {{
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );
"""

  TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange = """
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    {DATASET},
    _spacetreeId
  );
"""

  TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Epilogue = """
  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}}
"""


  TemplateExchangeRoutines_sendVertex_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendVertex(int inOutStack, int relativePositionOnInOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {{
  logTraceInWith4Arguments( "sendVertex(int,int,int)", inOutStack, relativePositionOnInOutStack, toStack, _spacetreeId );
"""

  TemplateExchangeRoutines_sendVertex_Epilogue = """
  logTraceOut( "sendVertex(int,int,int)");
}}
"""

  TemplateExchangeRoutines_sendFace_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendFace(int inOutStack, int relativePositionOnInOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {{
  logTraceInWith4Arguments( "sendFace(int,int,int)", inOutStack, relativePositionOnInOutStack, toStack, _spacetreeId );
"""

  TemplateExchangeRoutines_sendFace_Epilogue = """
  logTraceOut( "sendFace(int,int,int)");
}}
"""


  TemplateExchangeRoutines_sendCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendCell(int inOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::CellMarker& marker) {{
  logTraceInWith3Arguments( "sendCell(int,int)", inOutStack, toStack, _spacetreeId );
  const int relativePositionOnInOutStack = 0;
"""

  TemplateExchangeRoutines_sendCell_Epilogue = """
  logTraceOut( "sendCell(int,int)");
}}
"""


  TemplateExchangeRoutines_send_Exchange = """
  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::send(marker) ) {{
    auto& data = DataRepository::_{logical_type_name}Stack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to tree " << toTree << "'s stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_{logical_type_name}Stack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }}
"""


  TemplateExchangeRoutines_receiveAndMergeVertex_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::receiveAndMergeVertex(const peano4::grid::GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {{
  logTraceInWith7Arguments( "receiveAndMergeVertex(...)", event.toString(), positionWithinCell, inOutStack, relativePositionOnInOutStack, fromStack, marker.toString(), _spacetreeId );
"""

  TemplateExchangeRoutines_receiveAndMergeVertex_Epilogue = """
  logTraceOut( "receiveAndMergeVertex(...)");
}}
"""

  TemplateExchangeRoutines_receiveAndMergeFace_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::receiveAndMergeFace(const peano4::grid::GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {{
  logTraceInWith7Arguments( "receiveAndMergeFace(...)", event.toString(), positionWithinCell, inOutStack, relativePositionOnInOutStack, fromStack, marker.toString(), _spacetreeId ); 
"""

  TemplateExchangeRoutines_receiveAndMergeFace_Epilogue = """
  logTraceOut( "receiveAndMergeFace(...)");
}}
"""


  TemplateExchangeRoutines_receiveAndMerge_Exchange = """
  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::receiveAndMerge(marker) ) {{
    auto   incomingData = DataRepository::_{logical_type_name}Stack.getForPop(
      _spacetreeId, fromStack
    )->pop();
  
    auto&  data = DataRepository::_{logical_type_name}Stack.getForPush( _spacetreeId, inOutStack )->top(
      relativePositionOnInOutStack
    );
    
    assertionVectorNumericalEquals7( 
      data.getDebugX(), incomingData.getDebugX(), 
      data.getDebugH(), incomingData.getDebugH(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    assertionVectorNumericalEquals7( 
      data.getDebugH(), incomingData.getDebugH(), 
      data.getDebugX(), incomingData.getDebugX(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    
    data.merge(incomingData, marker);
  }}
"""


  TemplateExchangeRoutines_deleteAllStacks_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::deleteAllStacks() {{
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );
"""

  TemplateExchangeRoutines_deleteAllStacks_Exchange = """
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    {DATASET},
    _spacetreeId
  );
"""

  TemplateExchangeRoutines_deleteAllStacks_Epilogue = """
  logTraceOut( "deleteAllStacks()");
}}
"""

  def __generate_exchange_routines(self,output_file):
    output_file.write( self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Epilogue.format(**self.d) )
      
    output_file.write( self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_sendVertex_Prologue.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "logical_type_name" ]   = vertex.get_logical_type_name()
      self.d[ "full_qualified_type" ] = vertex.get_full_qualified_type()
      output_file.write( self.TemplateExchangeRoutines_send_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_sendVertex_Epilogue.format(**self.d) )
      
    output_file.write( self.TemplateExchangeRoutines_sendFace_Prologue.format(**self.d) )
    for face in self.faces:
      self.d[ "logical_type_name" ]   = face.get_logical_type_name()
      self.d[ "full_qualified_type" ] = face.get_full_qualified_type()
      output_file.write( self.TemplateExchangeRoutines_send_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_sendFace_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_sendCell_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "logical_type_name" ]   = cell.get_logical_type_name()
      self.d[ "full_qualified_type" ] = cell.get_full_qualified_type()
      output_file.write( self.TemplateExchangeRoutines_send_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_sendCell_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_receiveAndMergeVertex_Prologue.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "logical_type_name" ]   = vertex.get_logical_type_name()
      self.d[ "full_qualified_type" ] = vertex.get_full_qualified_type()
      output_file.write( self.TemplateExchangeRoutines_receiveAndMerge_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_receiveAndMergeVertex_Epilogue.format(**self.d) )
      
    output_file.write( self.TemplateExchangeRoutines_receiveAndMergeFace_Prologue.format(**self.d) )
    for face in self.faces:
      self.d[ "logical_type_name" ]   = face.get_logical_type_name()
      self.d[ "full_qualified_type" ] = face.get_full_qualified_type()
      output_file.write( self.TemplateExchangeRoutines_receiveAndMerge_Exchange.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_receiveAndMergeFace_Epilogue.format(**self.d) )

    output_file.write( self.TemplateExchangeRoutines_deleteAllStacks_Prologue.format(**self.d) )
    for cell in self.cells:
      self.d[ "DATASET" ] = "DataRepository::_" + cell.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_deleteAllStacks_Exchange.format(**self.d) )
    for face in self.faces:
      self.d[ "DATASET" ] = "DataRepository::_" + face.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_deleteAllStacks_Exchange.format(**self.d) )
    for vertex in self.vertices:
      self.d[ "DATASET" ] = "DataRepository::_" + vertex.get_logical_type_name() + "Stack";
      output_file.write( self.TemplateExchangeRoutines_deleteAllStacks.format(**self.d) )
    output_file.write( self.TemplateExchangeRoutines_deleteAllStacks_Epilogue.format(**self.d) )
      
      
      



  TemplateImplementationFilePrologue = """
#include "{CLASSNAME}.h"
#include "DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log {FULL_QUALIFIED_CLASSNAME}::_log( "{FULL_QUALIFIED_CLASSNAME}" );

"""


  def __generate_implementation(self,overwrite,full_qualified_filename):
    if writeFile(overwrite,self.default_overwrite,full_qualified_filename):
      print( "write " + full_qualified_filename )
      output_file = open( full_qualified_filename, "w" )
      output_file.write( self.TemplateImplementationFilePrologue.format(**self.d) )
      
      self.__generate_constructor(output_file)
      self.__generate_clone(output_file)
      self.__generate_getGridControlEvents(output_file)
      self.__generate_beginTraversal(output_file)
      self.__generate_endTraversal(output_file)
      self.__generate_enterCell(output_file)
      self.__generate_leaveCell(output_file)
      self.__generate_exchange_routines(output_file)


  #
  # @todo Sollte man mit Jinja 2 nicht mehr brauchen
  #
  def get_cpp_file_name(self):
    return self.subdirectory + "/" + self.classname + ".cpp"


  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)
    
    cpp_filename    = directory + "/" + self.get_cpp_file_name()
    
    self.__generate_header(overwrite,directory)
    self.__generate_implementation(overwrite,cpp_filename)

