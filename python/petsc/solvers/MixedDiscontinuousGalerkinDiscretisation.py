# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

#stuff we need to compute the quadratures
import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange

from .Solver                                    import Solver
from petsc.actionsets.EnumerateCellsAndFaces    import EnumerateCellsAndFaces
from petsc.actionsets.EnumerateVertices         import EnumerateVerticesGeneral
from petsc.actionsets.InitDofs                  import InitVertexDoFs, InitFaceAndCellDoFs
from petsc.actionsets.ProjectPETScSolutionOnVerticesBackOntoMesh import ProjectPETScCellSolutionOnVerticesBackOntoMesh
from petsc.actionsets.AssemblePETSCMatrix       import AssemblePETSCMatrixForMixed

from abc import abstractmethod


class MixedDiscontinuousGalerkinDiscretisation(Solver):
  """!
   
  we have a few different pieces of degree-of-freedom
  terminology here. 

  CELL_DOFS - this will be our jinja template expression
    the number of physical degrees of freedom within a 
    cell. this should be (poly_degree + 1) ** dimensions

  CELL_DOF_UNKNOWNS - this will be our jinja expression
    for the number of different unknonws that we associate
    with a physica degree of freedom. here we take this to
    be 3 (u, @f$ \phi_x, \ \phi_y @f$)
  
  CELL_CARDINALITY - the product of the above two.


  
  """
  def __init__(self,
               name,
               polynomial_degree,
               dimensions,
               unknowns_per_cell_dof,
               min_h,
               max_h,
               cellCellMassMatrix,
               cellCellSystemMatrix,
               cellToFaceMatrix,
               faceFaceMatrix,
               ):
    """!
   
 Test comment (C)
    
   
    """
    super( MixedDiscontinuousGalerkinDiscretisation, self ).__init__( name,
                                                    min_h,
                                                    max_h
                                                  )

    self.polynomial_degree     = polynomial_degree
    self.dimensions            = dimensions
    self.unknowns_per_cell_dof = unknowns_per_cell_dof
    self.unknowns_per_face_dof = self.unknowns_per_cell_dof - 1 #only one of them points in the right direction

    self.cellCellMassMatrix    = cellCellMassMatrix
    self.cellCellSystemMatrix  = cellCellSystemMatrix
    self.cellToFaceMatrix      = cellToFaceMatrix
    self.FaceFaceMatrix        = faceFaceMatrix

    self._cell_dofs             = ((polynomial_degree + 1) ** dimensions)
    self._cell_unknowns         = self._cell_dofs * self.unknowns_per_cell_dof
    self._face_dofs     = ((polynomial_degree + 1) **( dimensions -1 ))
    '''
                          double it to account for the fact that we have two separate "entities" 
                          meeting here. ie we want to treat the left face of one cell as being
                          different from the right face of the other. but this is not possible
                          in Peano. we allot twice as many spaces as we need for one face, and
                          then use clever indexing later
    '''
    self._face_unknowns = 2 * self.unknowns_per_face_dof * self._face_dofs

    self.quadrature_points, self.weights, self.poly_coeffs = \
      self.initGaussLobatto()
    
    self._vertex_unknowns = 1
    self._vertexDof = peano4.datamodel.DaStGen2( name )
    self._vertexDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(self._vertex_unknowns) ) )
    self._vertexDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(self._vertex_unknowns) ) )
    self._vertexDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    self._vertexDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["DoF", "Dirichlet"] ) )

    ## add in all the attributes we need for a cell dofs
    self._cellDof = peano4.datamodel.DaStGen2( name )
    self._cellDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(self._cell_unknowns) ) )
    self._cellDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(self._cell_unknowns) ) )
    
    self._cellDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    self._cellDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior"] ) )

    #add in some unknowns for the faces 
    #our aim is to have multiple dofs per face, but we just store the starting index in "number"
    self._faceDof = peano4.datamodel.DaStGen2( name )
    self._faceDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(self._face_unknowns) ) )
    self._faceDof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs"  ,  str(self._face_unknowns) ) )
    self._faceDof.data.add_attribute( dastgen2.attributes.Integer( "number" ) )
    self._faceDof.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior"] ) )
    
    pass


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( peano4.toolbox.PlotCellDataInPeanoBlockFormat(
      filename       = "solution-" + self._name,
      cell_unknown   = self._cellDof,
      getter         = "getValue().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_cell   = self._cell_unknowns,
      guard_predicate              = "true", 
      ))

    observer.add_action_set( peano4.toolbox.PlotCellDataInPeanoBlockFormat(
      filename       = "rhs-" + self._name,
      cell_unknown   = self._cellDof,
      getter         = "getRhs().data()",
      description    = "b",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_cell   = self._cell_unknowns,
      guard_predicate              = "true", 
      ))
    pass


  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
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
    observer.add_action_set( InitFaceAndCellDoFs(self) ) 
    observer.add_action_set( EnumerateVerticesGeneral(self) )
    observer.add_action_set( EnumerateCellsAndFaces(self) ) 
    pass
  
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePETSCMatrixForMixed(self) )
    pass

  def add_to_init_petsc(self, observer):
    pass

  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScCellSolutionOnVerticesBackOntoMesh(self) )
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
        "VERTEX_CARDINALITY": self._vertex_unknowns,
        "CELL_CARDINALITY"  : self._cell_unknowns,
        "CELL_DOFS"         : self._cell_dofs,
        "CELL_DOF_UNKNOWNS" : self.unknowns_per_cell_dof,
        "FACE_CARDINALITY"  : self._face_unknowns,
        "FACE_DOFS"         : self._face_dofs,
        "FACE_DOF_UNKNOWNS" : self.unknowns_per_face_dof,
        "POLYNOMIAL_DEGREE":  self.polynomial_degree,
        }

    dictionary["MATRICES"] = f"""
    const tarch::la::Matrix < {self.cellCellMassMatrix.shape[0]}, {self.cellCellMassMatrix.shape[1]}, double >  _cellCellMassMatrix = {self.convertMatrixToInitList(self.cellCellMassMatrix)};
    const tarch::la::Matrix < {self.cellCellSystemMatrix.shape[0]}, {self.cellCellSystemMatrix.shape[1]}, double >  _cellCellSystemMatrix = {self.convertMatrixToInitList(self.cellCellSystemMatrix)};
    const tarch::la::Matrix < {self.cellToFaceMatrix.shape[0]}, {self.cellToFaceMatrix.shape[1]}, double >  _cellToFaceMatrix = {self.convertMatrixToInitList(self.cellToFaceMatrix)};
    const tarch::la::Matrix < {self.FaceFaceMatrix.shape[0]}, {self.FaceFaceMatrix.shape[1]}, double >  _faceFaceMatrix = {self.convertMatrixToInitList(self.FaceFaceMatrix)};
    """

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
      True,
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
  def number_of_cell_dofs(self):
    return self._cell_dofs
  
  '''
  not adding property for unknowns_per_cell_dof
  or unknowns_per_face_dof. these are exposed
  already.
  '''
  
  @property
  def number_of_face_unknowns(self):
    return self._face_unknowns

  @property
  def number_of_face_dofs(self):
    return self._face_dofs

  def get_solver_repository_snippets(self):
    header = """
    int getTotalFaceDofs();
    int getTotalCellDofs();
    int getTotalVertexDofs();

    /**
    * we order the matrix so that cell-cell coupling sits
    * in the top left, with face-face coupling in the bottom right
    */
    int getGlobalCellIndex(std::pair<int,int> localIndex);
    int getGlobalFaceIndex(std::pair<int,int> localIndex);
    int getGlobalVertexIndex(std::pair<int,int> localIndex);

    /*
    * store points, weight and coefficients for gaussian
    * quadratures. aligned left to right / bottom to top
    */
    const std::vector<std::vector<double>> quadPoints = {{QUADRATURE_POINTS}};
    const std::vector<double> quadWeights             = {{WEIGHTS}};
    const std::vector<std::vector<double>> polyCoeffs = {{POLY_COEFFS}};

    const int polynomialDegree = {{POLY_DEGREE}};

    /*
    we will enumerate our DoFs 0 -> (n+1)^d.
    for now we only have 1D basis functions, and we promote them to 2d by taking their product.
    we do this by taking the x position of the quadrature point, and pulling out an appropriate basis function
    and then      taking the y position of the quadrature point, and pulling out an appropriate basis function
    and multiplying them together.

    dof ordering, with polynomial degree n:


    .......................
    .......................
    .......................
    n+1 - n+2 - .... - 2n+1
    0   - 1   - .... -    n

    so this method will pick out (x,y) coords for each dof. 

    This is temporary - I haven't ironed out where the dofs should sit physically. we 
    should be able to get some correct local matrices.

    TODO upgrade to 3d
    */
    const std::vector<int> convertDofIndexToXYCoordinate(const int& dofIndex);

    /*
    method to evaluate polynomial at given point, given a set of coefficients
    */
    double evalPoly( const std::vector<double>& coeffs, const double& quadPoint );

    /*
    overload to:
      - convert index into a tuple that describes position in the cell
      - evaluate the 1d basis at each of the points
      - take the product

      ie we promote a 1d basis to an n-dimensional basis by evaluating 
      our basis polynomial at (x,y,z,...) and taking the product
      poly(x)*poly(y)*poly(z)*...
    */
    double evalPoly( const int& index, const std::vector<double>& quadPoint );

    /*
    need to specify in which dimension we are taking derivative
    */
    double evalDeriv( const int& dofIndex, const int& dimension, const std::vector<double>& quadPoint );
    double evalDeriv( const std::vector<double>& coeffs, const double& quadPoint );

    tarch::la::Vector<Dimensions,double> convertQuadPointToRealLocation( 
      const int& quadIndex,
      const tarch::la::Vector<Dimensions,double>& cellCentre,
      const tarch::la::Vector<Dimensions,double>& cellH
     );
"""
    cpp = """
void {{NAMESPACE | join("::")}}::repositories::computeLocalToGlobalMapsForAllSolvers() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {{SOLVER[1]}}.getFaceLocalToGlobalMap().computeLocalToGlobalMap();
  {{SOLVER[1]}}.getCellLocalToGlobalMap().computeLocalToGlobalMap();
  {% endfor %}
} 

int {{NAMESPACE | join("::")}}::repositories::getTotalNumberOfDegreesOfFreedom() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalFaceDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalCellDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalVertexDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getVertexLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalCellIndex(std::pair<int,int> localIndex) {
  int result;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalFaceIndex(std::pair<int,int> localIndex) {
  int result;
  result += getTotalCellDofs();
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalVertexIndex(std::pair<int,int> localIndex) {
  int result;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getVertexLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

/*
overload to actually perform polynomial eval
*/
double {{NAMESPACE | join("::")}}::repositories::evalPoly( const std::vector<double>& coeffs, const double& quadPoint ){
  double output = 0;
  double x      = 1;
  //coeffs are ordered from highest degree to lowest degree,
  //so iterate from back to front
  for (auto iter=coeffs.crbegin(); iter!=coeffs.crend(); iter++){
    output += (*iter) * x;
    x *= quadPoint;
  }
  return output;
}

/*
overload to:
  - convert index into a tuple that describes position in the cell
  - evaluate the 1d basis at each of the points
  - take the product

  ie we promote a 1d basis to an n-dimensional basis by evaluating 
  our basis polynomial at (x,y,z,...) and taking the product
  poly(x)*poly(y)*poly(z)*...
*/
double {{NAMESPACE | join("::")}}::repositories::evalPoly( const int& index, const std::vector<double>& quadPoint ){
  //convert index to pair of indices
  const std::vector<int> indices = convertDofIndexToXYCoordinate(index);
  double output = 1;

  /*
  let our dof   indices be ordered [d0, d1, ...]
  and our point indices be ordered [p0, p1, ...]

  then we actually want to return 

  eval(d0, p0) * eval(d1, p1) * ...
  */

  auto indexIter = indices.cbegin();
  auto pointIter = quadPoint.cbegin();

  for (; indexIter!=indices.end(), pointIter!=quadPoint.end(); indexIter++, pointIter++)
    output *= evalPoly( polyCoeffs[ *indexIter ], *pointIter );

  return output;
}


const std::vector<int> {{NAMESPACE | join("::")}}::repositories::convertDofIndexToXYCoordinate(const int& dofIndex){
  std::vector<int> output;
  
  //get x coord by taking modulo with polynomialDegree + 1
  const int x = dofIndex % (polynomialDegree + 1);
  output.push_back(x);

  //take off the x coordinate, divide by (polynomialDegree + 1) to determine
  //how many blocks north we are
  const int y = dofIndex / (polynomialDegree + 1);
  output.push_back(y);

  return output;
}

double {{NAMESPACE | join("::")}}::repositories::evalDeriv( const int& dofIndex, const int& dimension, const std::vector<double>& quadPoint ){
  if (dimension >= Dimensions) throw std::runtime_error("taking derivative in wrong dimension!");

  //convert index to pair of indices
  const std::vector<int> indices = convertDofIndexToXYCoordinate(dofIndex);
  double output = 1;

  for (int iter = 0; iter!=indices.size(); iter++){
    //we want to eval deriv, but only in one dimension
    output *= (iter == dimension)
      ? evalDeriv( polyCoeffs[ indices[iter] ], quadPoint[iter] )
      : evalPoly(  polyCoeffs[ indices[iter] ], quadPoint[iter] );
  }

  return output;
}

double {{NAMESPACE | join("::")}}::repositories::evalDeriv( const std::vector<double>& coeffs, const double& quadPoint ){
  double output = 0;
  double x      = 1;
  double derivFactor = 1; //factor to account for falling powers of x

  for (auto iter=coeffs.crbegin()+1; iter!=coeffs.crend(); iter++){
    output += (*iter) * x * derivFactor;
    x *= quadPoint; derivFactor++;
  }
  return output;
}

tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::repositories::convertQuadPointToRealLocation(
  const int& quadIndex,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const tarch::la::Vector<Dimensions,double>& cellH
){
  //get the quadrature point associated with this index
  const std::vector<double> quadPoint = quadPoints[quadIndex];
  tarch::la::Vector<Dimensions,double> output;

  for (int i=0; i<Dimensions; i++){
    //init output to be centre of the cell
    output[i] =  cellCentre[i];
    
    //move from centre -> quadpoint, weighted by the size of the cell
    //need to divide by 2 - want to make sure that furthest from the 
    //centre we move is h/2
    output[i] += quadPoint[i] * cellH[i]/2;
  }
  return output;  
}

"""
    return header, cpp