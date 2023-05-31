# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

from .Solver                                    import Solver
from petsc.actionsets.EnumerateVertices         import EnumerateVertices
from petsc.actionsets.InitVertexDoFs            import InitVertexDoFs
from petsc.actionsets.InitFaceAndCellDoFs       import InitFaceAndCellDoFs
from petsc.actionsets.ProjectPETScSolutionOnVerticesBackOntoMesh import ProjectPETScSolutionOnVerticesBackOntoMesh
from petsc.actionsets.AssemblePETSCMatrix       import AssemblePETSCMatrixOnCellsAndFaces

from abc import abstractmethod


class HdgFirstDiscretisation(Solver):
  """!
   
  this is a copy of the CollocatedLowOrderDiscretisation, but we rewrite
  to capture DoFs on faces and cells.

  We reuse some of the files from the first attempt at doing the Poisson problem.
  A quick summary of the ones we reuse and the ones we rewrite:

  Reuse, with the use of jinja if statements to 
  add the functionality we need:
    - Project.* in top level
    - PETScMain.* in top level
    - AssemblePETScMatrix.py in action sets
    - ProjectPETScSolutionOnVerticesBackOntoMesh.py
    - Solver.py
    - petscData/* (all the routines for PETSc matrices and solvers etc)

  Rewrite:
    - EnumerateCellsAndFaces.py
    - InitFaceAndCellDoFs.py
    - All the templates in the solver directory
  
  """
  def __init__(self,
               name,
               cell_dofs,
               face_dofs,
               min_h,
               max_h,
               stencil):
    """!
   
 Test comment (C)
    
   
    """
    super( HdgFirstDiscretisation, self ).__init__( name,
                                                              min_h,
                                                              max_h
                                                              )
    


    #hardcode one unknown on vertices for now
    self._vertex_unknowns = 1
    self._vertexDof = peano4.datamodel.DaStGen2( name )
    # self._vertexDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(number_of_unknowns) ) )
    # self._vertexDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(number_of_unknowns) ) )
    self._vertexDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    self._vertexDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["DoF", "Dirichlet"] ) )

    self._cell_unknowns = cell_dofs
    ## add in all the attributes we need for a cell dofs
    self._cellDof = peano4.datamodel.DaStGen2( name )
    self._cellDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(cell_dofs) ) )
    self._cellDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(cell_dofs) ) )
    self._cellDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    # self._cellDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["interior", "exterior"] ) )

    #add in some unknowns for the faces 
    self._face_unknowns = face_dofs
    self._faceDof = peano4.datamodel.DaStGen2( name )
    self._faceDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(face_dofs) ) )
    self._faceDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    self._faceDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior"] ) )
    
    self.stencil = stencil
    pass


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    # observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
    #   filename       = "solution-" + self._name,
    #   vertex_unknown = self._vertexDof,
    #   getter         = "getValue().data()",
    #   description    = "u",
    #   time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
    #   number_of_unknows_per_vertex = self._vertex_unknowns,
    #   guard_predicate              = "true", 
    #   ))
    # observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
    #   filename       = "rhs-" + self._name,
    #   vertex_unknown = self._vertexDof,
    #   getter         = "getRhs().data()",
    #   description    = "u",
    #   time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
    #   number_of_unknows_per_vertex = self._vertex_unknowns,
    #   guard_predicate              = "true", 
    #   ))
    pass


  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h),False)
    pass

  
  def add_to_enumerate(self, observer):
    pass
  
  
  def add_to_enumerate_and_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    Close to trivial: Just add the action set petsc.actionsets.InitVertexDoFs
    to the observer.
    
    """
    observer.add_action_set( InitVertexDoFs(self) ) 
    # observer.add_action_set( EnumerateVertices(self) ) 
    observer.add_action_set( InitFaceAndCellDoFs(self) ) 
    pass
  
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePETSCMatrixOnCellsAndFaces(self, self.stencil) )
    pass
  

  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScSolutionOnVerticesBackOntoMesh(self) )
    pass
  
  
  def add_use_statements(self, observer):
    observer.use_vertex( self._vertexDof )
    observer.use_face(self._faceDof)
    observer.use_cell(self._cellDof)
    pass


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    datamodel.add_vertex(self._vertexDof)
    datamodel.add_face(self._faceDof)
    datamodel.add_cell(self._cellDof)


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
        ###
        # fix this
        ###
        "VERTEX_CARDINALITY": self._vertex_unknowns,
        "CELL_CARDINALITY":   self._cell_unknowns,
        "FACE_CARDINALITY":   self._face_unknowns,
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
    output.makefile.add_h_file( "Abstract" + self._name + ".h" )
    output.makefile.add_h_file( self._name + ".h" )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp" )
    output.makefile.add_cpp_file( self._name + ".cpp" )


  @property
  def number_of_vertex_unknowns(self):
    return self._vertex_unknowns
  
  @property
  def number_of_cell_unknowns(self):
    return self._cell_unknowns
  
  @property
  def number_of_face_unknowns(self):
    return self._face_unknowns
  