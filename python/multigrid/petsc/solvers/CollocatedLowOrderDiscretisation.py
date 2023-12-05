# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

from .Solver                                              import Solver
from multigrid.petsc.actionsets.EnumerateDofs             import EnumerateDofs
from multigrid.petsc.actionsets.InitVertexDofs                             import InitVertexDoFs
from multigrid.petsc.actionsets.ProjectPETScSolutionOnVerticesBackOntoMesh import ProjectPETScSolutionOnVerticesBackOntoMesh

from abc import abstractmethod

from peano4.solversteps.ActionSet import ActionSet




class AssemblePetscMatrixOnVertices(ActionSet):
  """!
  Trigger assembly of PETSc matrix, using values from mesh

  I am aware the code below is very bad. Much of it needs changing.
  But for now it stays while development on other projects continues
  """
  TemplateTouchCellFirstTime = """
  //init a vector to collect global indices, and recall their position in the 
  //enumeration so we can apply the stencil correctly
  std::vector<int> enumeratedIndices(TwoPowerD, -1);

  for (int i=0; i<TwoPowerD ; i++) {
    if (fineGridVertices{{SOLVER_NAME}}PETScData(i).getType() == vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior) {
      //get a global index
      std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVertices{{SOLVER_NAME}}PETScData(i).getUnknownBaseNumber());
      int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);
      enumeratedIndices[i] = globalIndex;
    }
  }

  //next, loop twice over TwoPowerD, so we can correctly fill the petsc matrix.
  //calling our increment function 16 times is temporary; this can be done with 
  //one call to PETSc. While in development, we do it the dumb way.
  //recall that the vertices are enumerated in the following way:
  /*
  2 3
  0 1
  */
  // this geometry is crucial to getting the correct entries from the stencil

  for (int i=0; i<TwoPowerD; i++) {
    for (int j=0; j<TwoPowerD; j++) {
      if (enumeratedIndices[i] != -1 and enumeratedIndices[j] != -1) {
        auto lhsMatrixEntry = repositories::{{SOLVER_INSTANCE}}.getLhsMatrix(marker.x(),marker.h());

        repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().increment(
          enumeratedIndices[i], 
          enumeratedIndices[j], 
          lhsMatrixEntry(i,j)
        );
      }
    }
  }

  // rhs
  auto   rhs = repositories::{{SOLVER_INSTANCE}}.getRhsMatrix(marker.x(),marker.h());
  for (int i=0; i<TwoPowerD; i++) {
    if (fineGridVertices{{SOLVER_NAME}}PETScData(i).getType() == vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior) {
      double rhsContribution = 0.0;
      for (int j=0; j<TwoPowerD; j++) {
        double rhsValue = fineGridVertices{{SOLVER_NAME}}(j).getRhs(0);
        rhsContribution += rhs(i,j) * rhsValue;
      }
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().increment(
        enumeratedIndices[i],
        rhsContribution
      );
    }
  }
  """

  TemplateTouchVertexFirstTime="""
  //here we send the value, rhs from the mesh to petsc

  if (fineGridVertex{{SOLVER_NAME}}PETScData.getType() == vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior) {
    //first, get global index
    std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVertex{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);
    
    assertion3(globalIndex>=0, globalIndex, marker.toString(), fineGridVertex{{SOLVER_NAME}}PETScData.toString() );

    //get the val, rhs that was present in the mesh
//    double val = fineGridVertex{{SOLVER_NAME}}.getValue(0);
    double rhs = fineGridVertex{{SOLVER_NAME}}.getRhs(0);

    //send these to petsc
    repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(globalIndex, rhs);

  }
  """
  
  def __init__(self,
               solver,
               ):
    """!
    
Initialise vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.

stencil:
  list of lists which we will flatten into an intializer list,
  and transform into a tarch::la::Matrix
    
    """
    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    #stencil=[item for sublist in stencil for item in sublist]

    super( AssemblePetscMatrixOnVertices, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    #self.d["VERTEX_CARDINALITY"]     = solver.number_of_vertex_unknowns

    #self.d["STENCIL"]         = "{" + str(stencil[0])
    #for el in stencil[1:]:
    #  self.d["STENCIL"] += ", "
    #  self.d["STENCIL"] += str(el)
    #self.d["STENCIL"] += "}"




  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateTouchVertexFirstTime and 
replaces it with entries from the dictionary. The latter is befilled
in init().

We actually do something during touchVertexFirstTime and touchCellFirstTime. We insert the 
appropriate template into each.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchCellFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchVertexFirstTime).render(**self.d)
      pass 
    return result


  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
#include "tarch/la/Matrix.h"
"""

  def get_attributes(self):
    """!
    
    
    """
    return f"""
  int _spacetreeId;    
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

@see get_attributes()
    
    """
    return f"""
    _spacetreeId = treeNumber;
"""



class CollocatedLowOrderDiscretisation(Solver):
  """!
   
  A low-order discretisation with degrees of freedom on the vertices
    
  This is one of the simplest solvers that you can think of. It places
  one degree of freedom (can be scalar or a vector) on each vertex. As
  we support solely element-wise assembly, this means you can implement
  things like a 9-point (2d) or 27-point (3d) stencil, but not really
  anything more sophisticated.

  cell_lhs_matrix: [double] or []
    Pass in [] if you prefer to assemble the matrix per cell yourself.
    
  cell_rhs_matrix: [double] or []
    Pass in [] if you prefer to assemble the matrix per cell yourself.
  
  """
  def __init__(self,
               name,
               unknowns,
               dimensions,
               min_h,
               max_h,
               cell_lhs_matrix,
               cell_rhs_matrix,
               ):
    """!
   
    Collocated low-order (d-linear) Finite Elements 
       
    """
    super( CollocatedLowOrderDiscretisation, self ).__init__( name,
                                                              min_h,
                                                              max_h
                                                              )
    self._unknowns  = unknowns
    self._vertex_pde_data = peano4.datamodel.DaStGen2( name )
    self._vertex_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(unknowns) ) )
    self._vertex_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(unknowns) ) )

    self.cell_lhs_matrix = cell_lhs_matrix
    self.cell_rhs_matrix = cell_rhs_matrix
    pass


  def __str__(self):
      result = (
          """
Name:            """
          + self._name
          + """
Type:            """
          + self.__class__.__name__
          + """
Unknowns:        """
          + str(self._unknowns)
          + """
min_h:           """
          + str(self.min_h)
          + """
max_h:           """
          + str(self.max_h)
          + """
"""
      )
      return result

  __repr__ = __str__


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( CollocatedLowOrderDiscretisation, self ).add_to_Peano4_datamodel(datamodel,verbose)
    datamodel.add_vertex(self._vertex_pde_data)
  
  
  def add_use_statements(self, observer):
    super( CollocatedLowOrderDiscretisation, self ).add_use_statements(observer)
    observer.use_vertex( self._vertex_pde_data )


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "solution-" + self._name,
      vertex_unknown = self._vertex_pde_data,
      getter         = "getValue().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns,
      guard_predicate              = "true", 
      ))
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "rhs-" + self._name,
      vertex_unknown = self._vertex_pde_data,
      getter         = "getRhs().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns,
      guard_predicate              = "true", 
      ))
    pass


  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  
  def add_to_enumerate_and_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    Close to trivial: Just add the action set petsc.actionsets.InitVertexDoFs
    to the observer.
    
    """
    observer.add_action_set( EnumerateDofs(self) )
    observer.add_action_set( InitVertexDoFs(self) ) 
    pass
  
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePetscMatrixOnVertices(self) )
    pass
  

  def add_to_init_petsc(self, observer):
    pass


  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScSolutionOnVerticesBackOntoMesh(self) )
    pass


  def add_implementation_files_to_project(self, namespace, output):
    """
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    The implementation file will not be overwritten, as I assume that
    the users will write their own domain-specific stuff in there. If
    it does not exist yet, I'll create an empty stub which can be 
    befilled with something meaningful.
    
    Besides the creation of these two files, I also add the files to the
    project artifacts and the makefile, so they are captured by the build
    system.
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/"
    templatefile_prefix += self.__class__.__name__

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "CELL_LHS_MATRIX": self.cell_lhs_matrix,
        "CELL_RHS_MATRIX": self.cell_rhs_matrix,
        "VERTEX_CARDINALITY": self.number_of_matrix_entries_per_vertex,
        }

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".Abstract.template.h",
      templatefile_prefix + ".Abstract.template.cpp",
      "Abstract" + self.typename(), 
      namespace,
      ".", 
      dictionary,
      True,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self.typename(), 
      namespace,
      ".", 
      dictionary,
      False,
      True)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_h_file( "Abstract" + self._name + ".h", generated=True )
    output.makefile.add_h_file( self._name + ".h", generated=True )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp", generated=True )
    output.makefile.add_cpp_file( self._name + ".cpp", generated=True )



  @property
  def number_of_matrix_entries_per_vertex(self):
    """!
    
    Nothing is associated with the vertex. There's nothing to be done here.
    
    """
    return self._unknowns
  
  @property
  def number_of_matrix_entries_per_face(self):
    """!
    
    In the DG scheme, we have a projection from the left and we have a 
    projection from the right. These values are proper projections, i.e.
    they do not carry any semantics. Then we have to solve the Riemann
    problem, and need one more unknown to store the outcome of that one.
    
    """
    return 0
  
  @property
  def number_of_matrix_entries_per_cell(self):
    """!
    
    This is the actual unknowns per cell
    
    """
    return 0

  @property
  def name(self):
    return self._name
