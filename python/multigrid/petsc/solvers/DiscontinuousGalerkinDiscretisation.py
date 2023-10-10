# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

from .Solver                                              import Solver
from multigrid.petsc.actionsets.EnumerateDoFs             import EnumerateDoFs
from multigrid.petsc.actionsets.InitCellDoFs              import InitCellDoFs
from multigrid.petsc.actionsets.ProjectPETScSolutionBackOntoMesh    import ProjectPETScSolutionOnCellsBackOntoMesh
from multigrid.petsc.actionsets.PlotDGDataInPeanoBlockFormat        import PlotDGDataInPeanoBlockFormat

from abc import abstractmethod

from peano4.solversteps.ActionSet import ActionSet



class AssemblePetscMatrix(ActionSet):
  """!

  Trigger assembly of PETSc matrix and also assemble rhs vector

  """
  TemplateTouchCellFirstTime = """
  if (fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior) {
    std::pair<int,int> localCellIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalCellIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localCellIndex);

    auto lhsCellMatrix = repositories::{{SOLVER_INSTANCE}}.getLhsMatrix(marker.x(),marker.h());
    for (int row=0; row<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; row++) 
    for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalCellIndex+row, 
        globalCellIndex+col, 
        lhsCellMatrix(row,col)
      );
    }
    
    auto rhsCellMatrix = repositories::{{SOLVER_INSTANCE}}.getRhsMatrix(marker.x(),marker.h());
    for (int row=0; row<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; row++) {
      double rhs = 0.0;
      for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
        rhs += rhsCellMatrix(row,col) * fineGridCell{{SOLVER_NAME}}.getRhs(col);
      }
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        row, rhs
      );
    }
    
    auto projectionCellToFaceMatrix = repositories::{{SOLVER_INSTANCE}}.getProjectionOfCellDataOntoFace(marker.x(),marker.h());
    auto projectionFaceToCellMatrix = repositories::{{SOLVER_INSTANCE}}.getProjectionOfRiemannSolutionOntoCell(marker.x(),marker.h());
    int dofsPerFace = repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.Unknowns;
    for (int d=0; d<Dimensions; d++) {
      std::pair<int,int> localFirstDoFIndexOfLeftFace  = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}PETScData(d).getUnknownBaseNumber());
      std::pair<int,int> localFirstDoFIndexOfRightFace = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}PETScData(d+Dimensions).getUnknownBaseNumber());
      int globalFirstDoFIndexOfLeftFace  = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndexOfLeftFace);
      int globalFirstDoFIndexOfRightFace = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndexOfRightFace);
      
      for (int row=0; row<dofsPerFace; row++ ) {
        for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalFirstDoFIndexOfLeftFace + row + dofsPerFace, 
            globalCellIndex+col,
            projectionCellToFaceMatrix(d * dofsPerFace + row,col)
          );
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalFirstDoFIndexOfRightFace + row, 
            globalCellIndex+col,
            projectionCellToFaceMatrix(d * dofsPerFace + row + Dimensions*dofsPerFace,col)
          );
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalCellIndex+col,
            globalFirstDoFIndexOfLeftFace + row + 2 * dofsPerFace, 
            projectionFaceToCellMatrix(col, d * dofsPerFace + row)
          );
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalCellIndex+col,
            globalFirstDoFIndexOfRightFace + row + 2 * dofsPerFace, 
            projectionFaceToCellMatrix(col, d * dofsPerFace + row + Dimensions*dofsPerFace)
          );
        }
      }
    }
  }
  """

  TemplateTouchFaceFirstTime="""
  auto RiemannSolverMatrix = repositories::{{SOLVER_INSTANCE}}.getRiemannSolver(marker.x(),marker.h());
  int dofsPerFace = repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.Unknowns;
  if ( fineGridFace{{SOLVER_NAME}}PETScData.getType() != facedata::{{SOLVER_NAME}}PETScData::Type::Coarse ) {
    std::pair<int,int> localFirstDoFIndex = std::make_pair(_spacetreeId, fineGridFace{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalFirstDoFIndex  = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndex);
    for (int row=0; row<dofsPerFace; row++)
    for (int col=0; col<2*dofsPerFace; col++) {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalFirstDoFIndex + 2*dofsPerFace,
        globalFirstDoFIndex + col, 
        RiemannSolverMatrix(row, col)
      );
    }
  }
  """
  
  def __init__(self,
               solver,
               ):
    """!
    
    Yet to be written
        
    """
    
    super( AssemblePetscMatrix, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()


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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchFaceFirstTime).render(**self.d)
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



class DiscontinuousGalerkinDiscretisation(Solver):
  """!
   
  Discontinuous Galerkin class
  
  
  ## Usage of the built-in matrix generators
  
  Obviously, you are free to use any way to generate the matrices that you 
  want. If you however decide to use the built-in matrix generators, please
  take into account that you have to follow one convention: Whenever you
  work with plain DG, the unknowns per face node have to be the same as 
  the unknowns per cell node. Otherwise, your matrix sizes will not match.
  
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  matrices = multigrid.matrixgenerators.GaussLobatto(polynomial_degree     = args.degree,
                                                   dimensions              = args.dimensions,
                                                   unknowns_per_cell_node  = 1,
                                                   unknowns_per_face_node  = 1,  # for DG, this has to equal unknowns_per_cell_dof
                                                   )
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  
  

  cell_lhs_matrix: [double] or []
    Pass in [] if you prefer to assemble the matrix per cell yourself.
    
  cell_rhs_matrix: [double] or []
    Pass in [] if you prefer to assemble the matrix per cell yourself.

  cell_cell_lhs_matrix_scaling: Positive integer
    The lhs matrix is scaled with @f$ h^x @f$ where the x is defined by this 
    parameter. If you don't specify an lhs matrix, i.e. you prefer to inject
    this matrix manually, you can leave this parameter None, as it is not 
    used.
    
  cell_cell_rhs_matrix_scaling: Positive integer
    The rhs matrix is scaled with @f$ h^x @f$ where the x is defined by this 
    parameter. If you don't specify a rhs matrix, i.e. you prefer to inject
    this matrix manually, you can leave this parameter None, as it is not 
    used.
  
  """
  def __init__(self,
               name,
               polynomial_degree,
               dimensions,
               unknowns,
               min_h,
               max_h,
               cell_cell_lhs_matrix,
               cell_cell_rhs_matrix,
               cell_cell_lhs_matrix_scaling,
               cell_cell_rhs_matrix_scaling,
               cell_to_face_matrix,
               face_to_cell_matrix,
               cell_to_face_matrix_scaling,
               face_to_cell_matrix_scaling,
               face_face_Riemann_problem_matrix,
               quadrature_points_in_unit_interval,
               ):
    """!
   
 Test comment (C)
    
   
    """
    super( DiscontinuousGalerkinDiscretisation, self ).__init__( name,
                                                                 min_h,
                                                                 max_h
                                                                 )

    self.polynomial_degree = polynomial_degree
    self.dimensions        = dimensions
    self.unknowns          = unknowns

    self.cell_cell_lhs_matrix               = cell_cell_lhs_matrix
    self.cell_cell_rhs_matrix               = cell_cell_rhs_matrix
    self.cell_to_face_matrix                = cell_to_face_matrix
    self.face_to_cell_matrix                = face_to_cell_matrix
    self.face_face_Riemann_problem_matrix   = face_face_Riemann_problem_matrix

    self.cell_cell_lhs_matrix_scaling = cell_cell_lhs_matrix_scaling
    self.cell_cell_rhs_matrix_scaling = cell_cell_rhs_matrix_scaling
    self.cell_to_face_matrix_scaling  = cell_to_face_matrix_scaling
    self.face_to_cell_matrix_scaling  = face_to_cell_matrix_scaling
    
    self.quadrature_points_in_unit_interval = quadrature_points_in_unit_interval
    
    self._cell_pde_data   = peano4.datamodel.DaStGen2( name )
    self._cell_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(self.number_of_matrix_entries_per_cell) ) )
    self._cell_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(self.number_of_matrix_entries_per_cell) ) )
    
    pass


  @property
  def nodes_per_cell(self):
    return (self.polynomial_degree + 1) ** self.dimensions


  @property
  def nodes_per_face(self):
    return (self.polynomial_degree + 1) ** (self.dimensions-1)


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """!
    
    Initialise index model 
    
    There is an index data model, where all the vertices, faces and cells
    are properly enumerated. This one is taken care of by the superclass.
    The thing we have to add our our PDE-specific unknowns.
    
    """
    super( DiscontinuousGalerkinDiscretisation, self ).add_to_Peano4_datamodel(datamodel,verbose)
    datamodel.add_cell(self._cell_pde_data)


  def add_use_statements(self, observer):
    super( DiscontinuousGalerkinDiscretisation, self ).add_use_statements(observer)
    observer.use_cell(self._cell_pde_data)
    pass


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( PlotDGDataInPeanoBlockFormat(self) )
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
    observer.add_action_set( EnumerateDoFs(self) )
    observer.add_action_set( InitCellDoFs(self) ) 
    pass
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePetscMatrix(self) )
    pass

  def add_to_init_petsc(self, observer):
    pass

  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScSolutionOnCellsBackOntoMesh(self) )
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
        "MIN_H"             : self.min_h,
        "MAX_H"             : self.max_h,
        "POLYNOMIAL_DEGREE":  self.polynomial_degree,
        "UNKNOWNS":           self.unknowns,
        "CELL_CELL_LHS_MATRIX":      self.cell_cell_lhs_matrix,
        "CELL_CELL_RHS_MATRIX":      self.cell_cell_rhs_matrix,
        "FACE_FACE_RIEMANN_PROBLEM_MATRIX":      self.face_face_Riemann_problem_matrix,
        "CELL_TO_FACE_MATRIX":       self.cell_to_face_matrix,
        "FACE_TO_CELL_MATRIX":                   self.face_to_cell_matrix,
        "QUADRATURE_POINTS_IN_UNIT_INTERVAL":    self.quadrature_points_in_unit_interval,
        "CELL_CELL_LHS_MATRIX_SCALING": self.cell_cell_lhs_matrix_scaling,
        "CELL_CELL_RHS_MATRIX_SCALING": self.cell_cell_rhs_matrix_scaling,
        "CELL_TO_FACE_MATRIX_SCALING":           self.cell_to_face_matrix_scaling,
        "FACE_TO_CELL_MATRIX_SCALING":           self.face_to_cell_matrix_scaling,
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
    return 0
  
  @property
  def number_of_matrix_entries_per_face(self):
    """!
    
    In the DG scheme, we have a projection from the left and we have a 
    projection from the right. These values are proper projections, i.e.
    they do not carry any semantics. Then we have to solve the Riemann
    problem, and need one more unknown to store the outcome of that one.
    
    """
    return 3 * self.unknowns * self.nodes_per_face
  
  @property
  def number_of_matrix_entries_per_cell(self):
    """!
    
    This is the actual unknowns per cell
    
    """
    return self.unknowns * self.nodes_per_cell
  