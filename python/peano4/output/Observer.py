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
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS" ]     += "repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS" ]   += "repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]     += "repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_PICK_ENTRY" ]   += "repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)";

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
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS" ]     += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS" ]   += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]     += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS_PICK_ENTRY" ]   += "peano4::datamanagement::FaceEnumerator<" + face.get_full_qualified_type() + ">( &repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )(pick)";

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
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS" ]     += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS" ]   += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )";
      self.d[ "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]     += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)";
      self.d[ "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY" ]   += "peano4::datamanagement::VertexEnumerator<" + vertex.get_full_qualified_type() + ">( &repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )(pick)";


  def __generate_header(self,overwrite,directory):
    headerfile_template = os.path.realpath(__file__).replace( ".pyc", ".template.h" ).replace( ".py", ".template.h" )
    md = self.mkSubDict(["ATTRIBUTES", "INCLUDES"])
    header = Jinja2TemplatedHeaderFile(headerfile_template,self.classname,self.namespace,self.subdirectory, md,self.default_overwrite)
    header.generate(overwrite,directory)
    del header


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
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + cell.get_full_qualified_type() + "() );\n"
      pass

    for face in self.faces:
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  for (int i=0; i<TwoTimesD; i++) {\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "    repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + face.get_full_qualified_type() + "() );\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  }\n"
      pass

    for vertex in self.vertices:
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "  for (int i=0; i<TwoPowerD; i++) {\n"
      self.d[ "INITIAL_PUSH_TO_OUTPUT_STREAMS" ]    += "    repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( " + vertex.get_full_qualified_type() + "() );\n"
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
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      pass

    for face in self.faces:
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  for (int i=0; i<TwoTimesD; i++) {\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "    repositories::DataRepository::_" + face.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  }\n"
      pass

    for vertex in self.vertices:
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  for (int i=0; i<TwoPowerD; i++) {\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "    repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();\n"
      self.d[ "FINAL_POP_FROM_INPUT_STREAMS" ]    += "  }\n"
      pass


    output_file.write( self.TemplateEndTraversal.format(**self.d) )



  def __format_template_per_action(self,output_file,template,reverse_order=False, manual_dict=None):
    """
    
     Takes the specified template file, iterates over actions and pastes
     the template into the output file once per action. Per action, the dictionary's
     entries are updated. Otherwise, the dictionary remains unchanged.
     
     
     output_file:
       Handle on output file
     
    """
    local_actions = [x for x in self.included_actions]
    if reverse_order:
      local_actions.reverse()

    for action in local_actions:
      if manual_dict is None:
          self.d[ "ACTIVE_ACTION_SET" ]  = "_actionSet" + str( self.included_actions.index(action) )
          self.d[ "ACTIVE_ACTION_SET_FULL_QUALIFIED_NAME" ]  = action
          if output_file is not None:
            output_file.write( template.format(**self.d) )
          else:
            return template.format(**self.d)
      else:

          manual_dict[ "ACTIVE_ACTION_SET" ]  = "_actionSet" + str( self.included_actions.index(action) )
          manual_dict[ "ACTIVE_ACTION_SET_FULL_QUALIFIED_NAME" ]  = action
          if output_file is not None:
            output_file.write( template.format(**manual_dict) )
          else:
            return template.format(**manual_dict)


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
    output_file.write( self.TemplateGetGridControlEvents_Prologue.format(**{"FULL_QUALIFIED_CLASSNAME":self.d["FULL_QUALIFIED_CLASSNAME"]}) )
    self.__format_template_per_action(output_file,self.TemplateGetGridControlEvents_MappingCall, manual_dict={})
    output_file.write( self.TemplateGetGridControlEvents_Epilogue.format(**{}) )


  def __generate_clone(self,output_file):
    output_file.write( self.TemplateClone.format(**self.d) )


  TemplateEnterCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::enterCell( const peano4::grid::GridTraversalEvent&  event ) {{
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );
"""


  TemplateLoadCell_VertexLoad = """
  // Load vertex {name}
  {{
    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {{
      int inVertexStack          = event.getVertexDataFrom(i);
      int outVertexStackPosition = event.getVertexDataTo(i);
      logDebug("loadCell(...)", "vertex stack " << inVertexStack << "->pos-" << outVertexStackPosition );

      peano4::datamanagement::VertexMarker  marker(event,outVertexStackPosition);

      {full_qualified_type} data;
      if (
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inVertexStack) or {full_qualified_type}::loadPersistently(marker))
      ) {{
        assertion4( not repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->empty(), event.toString(), peano4::datamanagement::VertexMarker(event).toString(), _spacetreeId, inVertexStack);
        data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->pop();
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
        logDebug( "loadCell(...)", "initialise meta data of vertex {name} with " << marker.x(outVertexStackPosition) << " x " << marker.h() << ": load persistent data=" << {full_qualified_type}::loadPersistently(marker) << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outVertexStackPosition) );
        data.setDebugH( marker.h() );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
      ) {{
        assertionVectorNumericalEquals7( data.getDebugX(), marker.x(outVertexStackPosition), event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, outVertexStackPosition, _spacetreeId );
        assertionVectorNumericalEquals6( data.getDebugH(), marker.h(),                       event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, _spacetreeId );
      }}
      #endif

      view.set(outVertexStackPosition,data);
    }}
  }}
"""


  TemplateEnterCell_VertexMappingCall = """
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
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchVertexFirstTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.createHangingVertex(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchVertexFirstTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""


  TemplateLoadCell_FaceLoad = """
  // Load face {name}
  {{
    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {{
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      {full_qualified_type} data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or {full_qualified_type}::loadPersistently(marker))
      ) {{
        assertion4( not repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
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
        logDebug( "loadCell(...)", "initialise meta data of face {name} with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << {full_qualified_type}::loadPersistently(marker)  << " (" << marker.toString() << ")" );
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


  TemplateEnterCell_FaceMappingCall = """
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.touchFaceFirstTime(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""


  TemplateLoadCell_CellLoad = """
  // Load cell {name}
  {{
    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << {full_qualified_type}::loadPersistently(marker) << ")" );

    {full_qualified_type}& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or {full_qualified_type}::loadPersistently(marker))
    ) {{
      assertion3( not repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }}

    #if PeanoDebug>0
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not {full_qualified_type}::loadPersistently(marker)
    ) {{
      logDebug( "loadCell(...)", "initialise meta data of cell {name} with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }}
    else {{
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
    }}
    #endif
  }}
"""


  TemplateEnterCell_CellMappingCall = """
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

  def generateDictEntry(self, key):
      """
      Some logic to produce new dictionary entries from existing ones.
      This deals mainly with having to avoid C++ code with consequtive commata
      as some dict entries can be an empty string.
      """

      fields = key.split(",")
      nonempty = [self.d[f] for f in fields if not f=="" and not self.d[f]==""]
      if len(nonempty)>0:
          s = ",".join(nonempty)
          if key.startswith(",") and not s.startswith(","): s = "," + s
          if key.endswith(",") and not s.endswith(","): s+=","
      else:
          s=""

      return s


  def mkSubDict(self, keys):
      temp = {}
      for k in keys:
          if not k in self.d:
              temp[k] = self.generateDictEntry(k)
          else:
              temp[k] = self.d[k]
      return temp


  def __generate_loadCell(self,output_file):
    output_file.write( self.TemplateLoadCell_Prologue.format(**self.d) )
    
    for vertex in self.vertices:
      temp = {
        "name": vertex.name,
        "enumeration_type":vertex.get_enumeration_type(),
        "logical_type_name":vertex.get_logical_type_name(),
        "full_qualified_type": vertex.get_full_qualified_type()
       }
      self.d[ "name" ]                 = vertex.name
      output_file.write( self.TemplateLoadCell_VertexLoad.format(**temp) )

    for face in self.faces:
      temp = {
        "name": face.name,
        "enumeration_type":face.get_enumeration_type(),
        "logical_type_name":face.get_logical_type_name(),
        "full_qualified_type": face.get_full_qualified_type()
       }
      self.d[ "name" ]                 = face.name
      output_file.write( self.TemplateLoadCell_FaceLoad.format(**temp) )

    for cell in self.cells:
      temp = {
        "name": cell.name,
        # "enumeration_type":cell.get_enumeration_type(),
        "logical_type_name":cell.get_logical_type_name(),
        "full_qualified_type": cell.get_full_qualified_type()
       }
      self.d[ "name" ]                 = cell.name
      output_file.write( self.TemplateLoadCell_CellLoad.format(**temp) )

    output_file.write( self.TemplateLoadCell_Epilogue.format({}) )


  def __generate_enterCell(self,output_file):
    """
      Generates enter cell
    """
    output_file.write( self.TemplateEnterCell_Prologue.format(**self.d) )

    if len(self.vertices)>0:
      md = self.mkSubDict(["name",
        "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS",
        ])
      self.__format_template_per_action(output_file, self.TemplateEnterCell_VertexMappingCall, False, manual_dict=md)

    if len(self.faces)>0:
      md = self.mkSubDict(["name",
        "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY",
        "MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS",
        "MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS",
        ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS"
        ])
      self.__format_template_per_action(output_file, self.TemplateEnterCell_FaceMappingCall, False, manual_dict=md)

    if len(self.cells)>0:
      md = self.mkSubDict(["name",
        "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,",
        "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,",
        "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS",
        ",MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS",
        ",MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS"
        ])
      self.__format_template_per_action(output_file, self.TemplateEnterCell_CellMappingCall, False, manual_dict=md)


    md = self.mkSubDict([
      ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_CELL_EVENT",
      ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_CELL_EVENT"])

    self.__format_template_per_action(output_file, self.TemplateEnterCell_MappingCall, False, manual_dict=md)
    output_file.write( self.TemplateEnterCell_Epilogue.format({}) )


  TemplateLoadCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::loadCell( const peano4::grid::GridTraversalEvent&  event ) {{
  logTraceInWith2Arguments( "loadCell(...)", _spacetreeId, event.toString() );
"""


  TemplateStoreCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::storeCell( const peano4::grid::GridTraversalEvent&  event ) {{
  logTraceInWith2Arguments( "storeCell(...)", _spacetreeId, event.toString() );
"""


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
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or {full_qualified_type}::storePersistently(marker))
    ) {{
      repositories::DataRepository::_{logical_type_name}Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }}
    else {{
      logDebug( "storeCell(...)", "do not store cell {name} with " << marker.x() << " x " << marker.h() );
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
        {ACTIVE_ACTION_SET}.destroyPersistentFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.destroyHangingFace(
           marker
          {,MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS}
          ,{MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""

  TemplateLeaveCell_FaceStore_Epilogue = """
  // Store face {name}
  {{
    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {{
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      {full_qualified_type} data = view.get(inFaceStackPosition);

      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outFaceStack) or {full_qualified_type}::storePersistently(marker))
      ) {{
        logDebug( "storeCell(...)", "store face {name} with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_{logical_type_name}Stack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }}
      else {{
        logDebug( "storeCell(...)", "do not store face {name} with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << {full_qualified_type}::storePersistently(marker) );
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
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {{
        {ACTIVE_ACTION_SET}.touchVertexLastTime(
           marker
          ,{MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY}
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
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
          {,MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS}
        );
      }}
    }}
  }}
"""

  TemplateLeaveCell_VertexStore_Epilogue = """
  // Store vertex {name}
  {{
    auto view = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {{
      int inVertexStackPosition  = event.getVertexDataFrom(i);
      int outVertexStack         = event.getVertexDataTo(i);
      logDebug("storeCell(...)", "pos-" << inVertexStackPosition << "->vertex stack " << outVertexStack);

      peano4::datamanagement::VertexMarker  marker(event,inVertexStackPosition);

      {full_qualified_type} data = view.get(inVertexStackPosition);

      if (
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outVertexStack) or {full_qualified_type}::storePersistently(marker))
      ) {{
        logDebug( "storeCell(...)", "store vertex {name} with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_{logical_type_name}Stack.getForPush(repositories::DataRepository::DataKey(_spacetreeId,outVertexStack))->push(data);
      }}
      else {{
        logDebug( "storeCell(...)", "do not store vertex {name} with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
      }}
    }}
  }}
"""


  TemplateLeaveCell_Epilogue = """
  logTraceOutWith1Argument( "leaveCell(...)", _spacetreeId );
}}
"""


  TemplateLoadCell_Epilogue = """
  logTraceOutWith1Argument( "loadCell(...)", _spacetreeId );
}}
"""


  TemplateStoreCell_Epilogue = """
  logTraceOutWith1Argument( "storeCell(...)", _spacetreeId );
}}
"""


  def __generate_leaveCell(self,output_file):
    """
      Generates enter cell
    """
    output_file.write( self.TemplateLeaveCell_Prologue.format(**self.mkSubDict(["FULL_QUALIFIED_CLASSNAME"])) )

    md = self.mkSubDict([
      ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_CELL_EVENT",
      ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS_CELL_EVENT"
      ])
    self.__format_template_per_action(output_file, self.TemplateLeaveCell_MappingCall, True, manual_dict=md)

    if len(self.cells)>0:
      md = self.mkSubDict([
        ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS"
        ])
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_CellStore_MappingCall, True, manual_dict=md)

    if len(self.faces)>0:
      md = self.mkSubDict([
        "name",
        ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS",
        ",MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS",
        ])
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_FaceStore_MappingCall, True, manual_dict=md)

    if len(self.vertices)>0:
      md = self.mkSubDict([
        "name",
        "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_FINE_GRID_CELL_ARGUMENTS",
        ",MAPPING_SIGNATURE_COARSE_GRID_VERTICES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_FACES_ARGUMENTS,MAPPING_SIGNATURE_COARSE_GRID_CELL_ARGUMENTS",
        "MAPPING_SIGNATURE_FINE_GRID_VERTICES_ARGUMENTS_PICK_ENTRY",
        ])
      self.__format_template_per_action(output_file, self.TemplateLeaveCell_VertexStore_MappingCall, True, manual_dict=md)

    output_file.write( self.TemplateLeaveCell_Epilogue.format({}))


  def __generate_storeCell(self,output_file):
    output_file.write( self.TemplateStoreCell_Prologue.format(**self.d) )
    
    for cell in self.cells:
      self.d[ "name" ] = cell.name
      temp = { "name" : cell.name, "logical_type_name" : cell.get_logical_type_name(), "full_qualified_type" : cell.get_full_qualified_type() }
      output_file.write( self.TemplateLeaveCell_CellStore_Epilogue.format(**temp) )

    for face in self.faces:
      self.d[ "name" ] = face.name
      temp = { "name" : face.name, "enumeration_type":face.get_enumeration_type(), "logical_type_name":face.get_logical_type_name(), "full_qualified_type":face.get_full_qualified_type()}
      output_file.write( self.TemplateLeaveCell_FaceStore_Epilogue.format(**temp) )

    for vertex in self.vertices:
      self.d[ "name" ]                 = vertex.name
      temp = { "name":vertex.name, "enumeration_type": vertex.get_enumeration_type(), "logical_type_name":vertex.get_logical_type_name(), "full_qualified_type":vertex.get_full_qualified_type()}
      output_file.write( self.TemplateLeaveCell_VertexStore_Epilogue.format(**temp) )

    output_file.write( self.TemplateStoreCell_Epilogue.format({}) )
    
      
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


  TemplateSendVertex_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {{
  logTraceInWith4Arguments( "sendVertex(int,int,int)", position, toStack, event.toString(), _spacetreeId );
  
"""
  
  TemplateSendVertex_SendCall = """
  {{
    peano4::datamanagement::VertexMarker  marker(event,position);
    if (
      {full_qualified_type}::storePersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::send(marker))
     ) {{
      {full_qualified_type} data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoPowerD-1-position);
    
      logDebug( "sendVertex(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_{logical_type_name}Stack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }}
  }}
"""

  TemplateSendVertex_Epilogue = """
  logTraceOut( "sendVertex(int,int,int)");
}}
"""


  TemplateSendFace_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendFace(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {{
  logTraceInWith4Arguments( "sendFace(int,int,int)", position, toStack, event.toString(), _spacetreeId );

"""

  TemplateSendFace_SendCall = """
  {{
    peano4::datamanagement::FaceMarker  marker(event,position);
    if (
      {full_qualified_type}::storePersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::send(marker))
     ) {{
      {full_qualified_type} data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoTimesD-1-position);
    
      logDebug( "sendFace(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_{logical_type_name}Stack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }}
  }}
"""


  TemplateSendFace_Epilogue = """
  logTraceOut( "sendFace(int,int,int)");
}}
"""


  TemplateSendCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::sendCell(int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {{
  logTraceInWith3Arguments( "sendCell(int,int,int)", toStack, event.toString(), _spacetreeId );
  
"""

  TemplateSendCell_SendCall = """
  {{
    peano4::datamanagement::CellMarker  marker(event);
    if (
      {full_qualified_type}::storePersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::send(marker))
     ) {{
      {full_qualified_type} data = repositories::DataRepository::_{logical_type_name}Stack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
    
      logDebug( "sendCell(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId );

      repositories::DataRepository::_{logical_type_name}Stack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }}
  }}
"""


  TemplateSendCell_Epilogue = """
  logTraceOut( "sendCell(int,int,int)");
}}
"""


  TemplateReceiveAndMergeCell_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::receiveAndMergeCell(int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event ) {{
  logTraceInWith3Arguments( "receiveAndMergeCell(...)", fromStack, event.toString(), _spacetreeId );
"""


  TemplateReceiveAndMergeCell_Epilogue = """
  logTraceOut( "receiveAndMergeCell(...)");
}}
"""


  TemplateReceiveAndMergeVertex_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::receiveAndMergeVertex(int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {{
  logTraceInWith4Arguments( "receiveAndMergeVertex(...)", position, fromStack, event.toString(), _spacetreeId );
"""


  TemplateReceiveAndMergeVertex_Epilogue = """
  logTraceOut( "receiveAndMergeVertex(...)");
}}
"""


  TemplateReceiveAndMergeFace_Epilogue = """
  logTraceOut( "receiveAndMergeFace(...)");
}}
"""


  TemplateReceiveAndMergeFace_Prologue = """
void {FULL_QUALIFIED_CLASSNAME}::receiveAndMergeFace(int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {{
  logTraceInWith4Arguments( "receiveAndMergeFace(...)", position, fromStack, event.toString(), _spacetreeId );
"""


  TemplateReceiveAndMergeVertex_ReceiveAndMergeCalls = """
  {{
    peano4::datamanagement::VertexMarker  marker(event,position);

    if ( 
      {full_qualified_type}::loadPersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::receiveAndMerge(marker))
     ) {{
      auto   incomingData = repositories::DataRepository::_{logical_type_name}Stack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      {full_qualified_type}& data = repositories::DataRepository::_{logical_type_name}Stack.getForPush( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoPowerD-1-position);

      logDebug( "receiveAndMergeVertex(...)", "merge " << incomingData.toString() << " into " << data.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {{
        assertion8(
          tarch::la::countEqualEntries(data.getDebugX(), incomingData.getDebugX())==Dimensions-1,
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), 
          fromStack, event.toString(), marker.toString(), _spacetreeId );
        assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}
      else {{
        assertionVectorNumericalEquals6(
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), fromStack, event.toString(), marker.toString(), _spacetreeId );
          assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}

      data.merge(incomingData, marker);
    }}
  }}
"""



  TemplateReceiveAndMergeFace_ReceiveAndMergeCalls = """
  {{
    peano4::datamanagement::FaceMarker  marker(event,position);

    if ( 
      {full_qualified_type}::loadPersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::receiveAndMerge(marker))
     ) {{
      auto   incomingData = repositories::DataRepository::_{logical_type_name}Stack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      {full_qualified_type}& data = repositories::DataRepository::_{logical_type_name}Stack.getForPush( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoTimesD-1-position);

      logDebug( "receiveAndMergeFace(...)", "merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {{
        assertion8(
          tarch::la::countEqualEntries(data.getDebugX(), incomingData.getDebugX())==Dimensions-1,
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), 
          fromStack, event.toString(), marker.toString(), _spacetreeId );
        assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}
      else {{
        assertionVectorNumericalEquals6(
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), fromStack, event.toString(), marker.toString(), _spacetreeId );
          assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}

      data.merge(incomingData, marker);
    }}
  }}
"""


  TemplateReceiveAndMergeCell_ReceiveAndMergeCalls = """
  {{
    peano4::datamanagement::CellMarker  marker(event);

    if ( 
      {full_qualified_type}::loadPersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or {full_qualified_type}::receiveAndMerge(marker))
     ) {{
      auto   incomingData = repositories::DataRepository::_{logical_type_name}Stack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      {full_qualified_type}& data = repositories::DataRepository::_{logical_type_name}Stack.getForPush( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top();

      logDebug( "receiveAndMergeCell(...)", "merge " << incomingData.toString() << " into " << data.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {{
        assertion8(
          tarch::la::countEqualEntries(data.getDebugX(), incomingData.getDebugX())==Dimensions-1,
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), 
          fromStack, event.toString(), marker.toString(), _spacetreeId );
        assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}
      else {{
        assertionVectorNumericalEquals6(
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), fromStack, event.toString(), marker.toString(), _spacetreeId );
          assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }}

      data.merge(incomingData, marker);
    }}
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
    s=""

    generic_dict_for_prologue_and_epilogue = {"FULL_QUALIFIED_CLASSNAME":self.d["FULL_QUALIFIED_CLASSNAME"]}

    s+=  self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_exchangeAllVerticalDataExchangeStacks_Epilogue.format({})

    s+= self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+= self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+= self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+= self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+= self.TemplateExchangeRoutines_exchangeAllHorizontalDataExchangeStacks_Epilogue.format({})

    s+= self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_exchangeAllPeriodicBoundaryDataStacks_Epilogue.format({})

    s+=  self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_streamDataFromSplittingTreeToNewTree_Epilogue.format({})

    s+=  self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_streamDataFromJoiningTreeToMasterTree_Epilogue.format({})

    s+=  self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_finishAllOutstandingSendsAndReceives_Epilogue.format({})

    s+=  self.TemplateSendVertex_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for vertex in self.vertices:
      temp = { 
        "name" : vertex.name, 
        "enumeration_type":vertex.get_enumeration_type(), 
        "logical_type_name":vertex.get_logical_type_name(), 
        "full_qualified_type":vertex.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateSendVertex_SendCall.format(**temp)
    s+=  self.TemplateSendVertex_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateSendFace_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for face in self.faces:
      temp = { 
        "name" : face.name, 
        "enumeration_type":face.get_enumeration_type(), 
        "logical_type_name":face.get_logical_type_name(), 
        "full_qualified_type":face.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateSendFace_SendCall.format(**temp)
    s+=  self.TemplateSendFace_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateSendCell_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      temp = { 
        "name" : cell.name, 
        "logical_type_name":cell.get_logical_type_name(), 
        "full_qualified_type":cell.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateSendCell_SendCall.format(**temp)
    s+=  self.TemplateSendCell_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateReceiveAndMergeCell_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      temp = { 
        "name" : cell.name, 
        "logical_type_name":cell.get_logical_type_name(), 
        "full_qualified_type":cell.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateReceiveAndMergeCell_ReceiveAndMergeCalls.format(**temp)
    s+=  self.TemplateReceiveAndMergeCell_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateReceiveAndMergeFace_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for face in self.faces:
      temp = { 
        "name" : face.name, 
        "enumeration_type":face.get_enumeration_type(), 
        "logical_type_name":face.get_logical_type_name(), 
        "full_qualified_type":face.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateReceiveAndMergeFace_ReceiveAndMergeCalls.format(**temp)
    s+=  self.TemplateReceiveAndMergeFace_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateReceiveAndMergeVertex_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for vertex in self.vertices:
      temp = { 
        "name" : vertex.name, 
        "enumeration_type":vertex.get_enumeration_type(), 
        "logical_type_name":vertex.get_logical_type_name(), 
        "full_qualified_type":vertex.get_full_qualified_type(),
        "FULL_QUALIFIED_CLASSNAME": self.d["FULL_QUALIFIED_CLASSNAME"]
      }
      s+=  self.TemplateReceiveAndMergeVertex_ReceiveAndMergeCalls.format(**temp)
    s+=  self.TemplateReceiveAndMergeVertex_Epilogue.format(**generic_dict_for_prologue_and_epilogue)

    s+=  self.TemplateExchangeRoutines_deleteAllStacks_Prologue.format(**generic_dict_for_prologue_and_epilogue)
    for cell in self.cells:
      s+=  self.TemplateExchangeRoutines_deleteAllStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + cell.get_logical_type_name() + "Stack"})
    for face in self.faces:
      s+=  self.TemplateExchangeRoutines_deleteAllStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + face.get_logical_type_name() + "Stack"})
    for vertex in self.vertices:
      s+=  self.TemplateExchangeRoutines_deleteAllStacks_Exchange.format(**{"DATASET":"repositories::DataRepository::_" + vertex.get_logical_type_name() + "Stack"})
    s+=  self.TemplateExchangeRoutines_deleteAllStacks_Epilogue.format(**generic_dict_for_prologue_and_epilogue)


    output_file.write( s)



  TemplateImplementationFilePrologue = """
#include "{CLASSNAME}.h"
#include "repositories/DataRepository.h"

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
      import inspect, os
      print( "{} written by {}".format(full_qualified_filename, os.path.basename(inspect.getfile(self.__class__))))

      output_file = open( full_qualified_filename, "w" )
      output_file.write( self.TemplateImplementationFilePrologue.format(**self.d) )

      self.__generate_constructor(output_file)
      self.__generate_clone(output_file)
      self.__generate_getGridControlEvents(output_file)
      self.__generate_beginTraversal(output_file)
      self.__generate_endTraversal(output_file)
      self.__generate_loadCell(output_file)
      self.__generate_enterCell(output_file)
      self.__generate_leaveCell(output_file)
      self.__generate_storeCell(output_file)
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
