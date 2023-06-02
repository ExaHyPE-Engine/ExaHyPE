# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class AssemblePetscMatrixOnVertices(ActionSet):
  """!
  Trigger assembly of PETSc matrix, using values from mesh
  """
  TemplateInitCell = """
  //init a vector to collect global indices, and recall their position in the 
  //enumeration so we can apply the stencil correctly
  std::vector<int> enumeratedIndices(TwoPowerD, -1);

  // hard-code some factors that we divide by when entering values into our matrix
  // if two vertices match, then we divide by 4
  // if they're adjacent, divide by 2
  // if opposite, divide by 1
  static constexpr double sameVertex     = 4.0;
  static constexpr double adjacentVertex = 2.0;
  static constexpr double oppositeVertex = 1.0;

  for (int i=0; i<TwoPowerD ; i++){
    if (fineGridVertices{{SOLVER_NAME}}(i).getType() != vertexdata::{{SOLVER_NAME}}::Type::Dirichlet
      //TODO work out why using marker.willBeRefined() here causes only 1 iteration to be taken
      // whereas bodging to say fineGridVertices{{SOLVER_NAME}}(i).getNumber() < repositories::getTotalDegreesOfFreedom()
      // causes it to do 84 iters
      and not marker.willBeRefined()
    ) {
      //get a global index
      std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVertices{{SOLVER_NAME}}(i).getNumber());
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

  double h2 = marker.h()[0] * marker.h()[0];
  double matrixEntry;



  for (int i=0; i<TwoPowerD; i++){
    for (int j=0; j<TwoPowerD; j++){
    if (enumeratedIndices[i] != -1 and enumeratedIndices[j] != -1){
      if (i == j){
        //hardcode the index, what we want is the middle of the matrix
        matrixEntry = _stencil(1,1) / sameVertex / h2;
        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], matrixEntry);
        assertion(matrixEntry == 2.0/3/h2);

      }

      // if they add up to 3, then they must be opposites
      else if (i + j == 3){
        if(i==0){
          //we want the top right entry
          matrixEntry = _stencil(0,2) / oppositeVertex / h2;
        }
        if(i==1){
          //top left
          matrixEntry = _stencil(0,0) / oppositeVertex / h2;
        }
        if(i==2){
          //bottom right
          matrixEntry = _stencil(2,2) / oppositeVertex / h2;          
        }
        if(i==3){
          //bottom left
          matrixEntry = _stencil(2,0) / oppositeVertex / h2;
        }

        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], matrixEntry);
        assertion(matrixEntry == -1.0/3/h2);
      }

      //final two cases
      else{
        if( abs(i-j)==2 ){
          if (i>j){
            //look up
            matrixEntry = _stencil(0,1) / adjacentVertex / h2;
          }
          else if(i<j){
            //look down
            matrixEntry = _stencil(2,1) / adjacentVertex / h2;
          }
        }

        else{
          if (i<j){
            //look to the right
            matrixEntry = _stencil(0,2) / adjacentVertex / h2;
          }
          if(i>j){
            //look to the left
            matrixEntry = _stencil(0,0) / adjacentVertex / h2;
          }
        }
        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], matrixEntry);    
        assertion(matrixEntry==  -1.0/6/h2);
      }
    }

    }
  }
  
  """

  TemplateInitVertex="""
  //here we send the value, rhs from the mesh to petsc

  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Dirichlet
      and not marker.willBeRefined()
  ){
    //first, get global index
    std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVertex{{SOLVER_NAME}}.getNumber());
    int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);

    //get the val, rhs that was present in the mesh
    double val = fineGridVertex{{SOLVER_NAME}}.getValue(0);
    double rhs = fineGridVertex{{SOLVER_NAME}}.getRhs(0);

    //send these to petsc
    repositories::instanceOfPetsc.insertValueAndRHS(val, rhs, globalIndex);
  }
  """
  
  def __init__(self,
               solver,
               stencil):
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
    stencil=[item for sublist in stencil for item in sublist]

    super( AssemblePetscMatrixOnVertices, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["VERTEX_CARDINALITY"]     = solver.number_of_vertex_unknowns

    self.d["STENCIL"]         = "{" + str(stencil[0])
    for el in stencil[1:]:
      self.d["STENCIL"] += ", "
      self.d["STENCIL"] += str(el)
    self.d["STENCIL"] += "}"




  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateInitVertex and 
replaces it with entries from the dictionary. The latter is befilled
in init().

We actually do something during touchVertexFirstTime and touchCellFirstTime. We insert the 
appropriate template into each.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitVertex).render(**self.d)
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
  const tarch::la::Matrix<3,3,double> _stencil = {self.d["STENCIL"]};

  /*
  * splitting the stencil up into portions
  * _stencilTopRight refers to the top right cell, placing 
  * the centre of the stencil on the vertex in question
  * we preserve enumeration order
  */
  tarch::la::Vector<TwoPowerD, double> _stencilTopRight;
  tarch::la::Vector<TwoPowerD, double> _stencilTopLeft;
  tarch::la::Vector<TwoPowerD, double> _stencilBottomRight;
  tarch::la::Vector<TwoPowerD, double> _stencilBottomLeft;
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

@see get_attributes()
    
    """
    return f"""
    _spacetreeId = treeNumber;
    _stencilTopRight    = {{_stencil(1,1)/4.0, _stencil(1,2)/2.0, _stencil(0,1)/2.0, _stencil(0,2)/1.0 }};
    _stencilTopLeft     = {{_stencil(1,0)/2.0, _stencil(1,1)/4.0, _stencil(0,0)/1.0, _stencil(0,1)/2.0 }};
    _stencilBottomRight = {{_stencil(2,1)/2.0, _stencil(2,2)/1.0, _stencil(1,1)/4.0, _stencil(1,2)/2.0 }};
    _stencilBottomLeft  = {{_stencil(2,0)/1.0, _stencil(2,1)/2.0, _stencil(1,1)/4.0, _stencil(1,1)/2.0 }};
"""

class AssemblePETSCMatrixOnCellsAndFaces(ActionSet):
  ## add templates

  def __init__(self, solver):
    super( AssemblePETSCMatrixOnCellsAndFaces, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_CARDINALITY"]   = solver.number_of_cell_unknowns
    self.d["FACE_CARDINALITY"]   = solver.number_of_face_unknowns

  def get_body_of_operation(self, operation_name):
    ## add touchcellfirsttime

    ## add touch face first time

    return """"""

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
"""
