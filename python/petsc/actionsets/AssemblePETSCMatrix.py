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


'''

we leave the class below in place. We can rewrite soon to fit our needs

'''

class AssemblePETSCMatrixOnCellsAndFaces(ActionSet):
  ## add templates
  TemplateInitCell = """
  if (not marker.willBeRefined()){
    //init vector to collect the cell indices we use
    std::vector<int> cellDofIndices;

    //get global index
    std::pair<int,int> cellLocalIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}.getNumber());
    int cellGlobalIndex = repositories::{{SOLVER_INSTANCE}}.getCellLocalToGlobalMap().getGlobalIndex(cellLocalIndex);

    //add this global index, plus each of its degrees of freedom, to the cellDofIndices
    for (int i=0; i<_cellDofs; i++)
      cellDofIndices.push_back( cellGlobalIndex + i ); //works because we reserved (eg) 27 indices per cell earlier
    
    /*
    enumerate the faces. for each face, we do the following:
    1. collect the global index of the face, add this to the list faceDofIndices
      - also add to the list each degree of freedom that lies on this face
    2. (have already done same for cell indices above)
    3. insert stencil for FF, CF, FC and CC using these indices
    */

    for (int face=0; face<2*Dimensions; face++){
      std::vector<int> faceDofIndices;

      //get global index
      std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}(face).getNumber());
      int globalIndex = repositories::{{SOLVER_INSTANCE}}.getFaceLocalToGlobalMap().getGlobalIndex(localIndex);

      //insert the faceDofs into our temp vector.
      for (int i=0; i<_faceDofs; i++)
        faceDofIndices.push_back( globalIndex + i );
      
      //insert the stencil for CF interactions
      //use face iterator to index our vector of stencils
      repositories::instanceOfPetsc.insertLocalMatrix( cellDofIndices, faceDofIndices, _stencilsCF[face] );

      //insert the stencil for FC interactions, only if we are not on the boundary
      if ( fineGridFaces{{SOLVER_NAME}}(face).getType() == facedata::{{SOLVER_NAME}}::Type::Interior ){
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, cellDofIndices, _stencilsFC[face] );

        //also insert the FF stencil intended for the interior
        //there's only two, so we use 0 for boundary and 1 for interior
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, faceDofIndices, _stencilsFF[1] );
      }

      else if ( fineGridFaces{{SOLVER_NAME}}(face).getType() == facedata::{{SOLVER_NAME}}::Type::Boundary ){
        //insert FF stencil intended for boundary
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, faceDofIndices, _stencilsFF[0] );
      }

    }

    //before we exit, let's insert the stencil for cell-cell interactions
    repositories::instanceOfPetsc.insertLocalMatrix( cellDofIndices, cellDofIndices, _stencilCC );
  }
"""

  def __init__(self, solver):
    super( AssemblePETSCMatrixOnCellsAndFaces, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_CARDINALITY"]   = solver.number_of_cell_unknowns
    self.d["FACE_CARDINALITY"]   = solver.number_of_face_unknowns

    '''
    HANDLING THE STENCILS!
    '''
    
    #FIRST - HANDLE STENCIL FOR FACE-FACE INTERACTIONS
    #we have two stencils, and we insert them into a std::vector
    #the 0th one is for the boundary, and the 1st one is for the interior

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsFF"] = len(solver.stencilsFF[0])
    self.d["stencilColsFF"] = len(solver.stencilsFF[0][0])
    numberOfCols = self.d["stencilColsFF"] #use this to insert a line break after every row
    self.d["STENCILSFF"] = []
    
    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsFF:
      #flatten it
      stencil=[item for sublist in stencil for item in sublist]
      stencilAsString = "\n{" + str(stencil[0])
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"

      #add to list of stencils
      self.d["STENCILSFF"].append(stencilAsString)

    #NEXT - HANDLE STENCIL FOR CELL-FACE INTERACTIONS
    #this time we have a list of stencils which must be ordered

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsCF"] = len(solver.stencilsCF[0])   #solver.stencilCF[0] is the 0th stencil itself
    self.d["stencilColsCF"] = len(solver.stencilsCF[0][0])#captures the number of columns 
    numberOfCols = self.d["stencilColsCF"]                #use this to insert a line break after every row
    self.d["STENCILSCF"] = []                             #init a list. we will insert the stencils into them.

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsCF:
      #flatten it
      stencil=[item for sublist in stencil for item in sublist]
      
      #init a string to capture the stencil, we will insert this into the dict at the end
      stencilAsString = "\n{" + str(stencil[0]) # add "{" to begin the intializer list
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"                    #end the initializer list

      #add to list of stencils 
      self.d["STENCILSCF"].append(stencilAsString)

    #NEXT - HANDLE STENCIL FOR FACE-CELL INTERACTIONS
    #this time we have a list of stencils which must be ordered

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsFC"] = len(solver.stencilsFC[0])   #solver.stencilFC[0] is the 0th stencil itself
    self.d["stencilColsFC"] = len(solver.stencilsFC[0][0])#captures the number of columns 
    numberOfCols = self.d["stencilColsFC"]               #use this to insert a line break after every row
    self.d["STENCILSFC"] = []                            #init a list. we will insert the stencils into them

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsFC:
      stencil=[item for sublist in stencil for item in sublist]
      stencilAsString = "\n{" + str(stencil[0])
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"

      #add to list
      self.d["STENCILSFC"].append(stencilAsString)

    #FINALLY - HANDLE STENCIL FOR CELL-CELL INTERACTIONS

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsCC"] = len(solver.stencilCC)
    self.d["stencilColsCC"] = len(solver.stencilCC[0])
    numberOfCols = len(solver.stencilCC[0]) #use this to insert a line break after every row

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    stencil=[item for sublist in solver.stencilCC for item in sublist]
    self.d["STENCILCC"] = "\n{" + str(stencil[0])
    for index, el in enumerate(stencil[1:]):
      self.d["STENCILCC"] += ", "
      if index > 0 and (index-1) % numberOfCols == 1:
        self.d["STENCILCC"] += "\n"
      self.d["STENCILCC"] += str(el)
    self.d["STENCILCC"] += "}"

  def get_constructor_body(self):
    return f"""
    _spacetreeId = treeNumber;
"""


  def get_body_of_operation(self, operation_name):
    """!
    for now, only touch face first time
    """
    ## add touchcellfirsttime

    ## add touch face first time
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
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
#include "tarch/la/Matrix.h"
#include "repositories/SolverRepository.h"
"""
  def get_attributes(self):
    """!
    """

    #construct the output strings for stencilsCF and stencilsFC
    stencilsCF = '{' #start the std vector
    for stencil in self.d["STENCILSCF"]:
      stencilsCF += stencil
      stencilsCF += ",\n"
    stencilsCF += "}" #end the std vector
    #same again
    stencilsFC = '{' #start the std vector
    for stencil in self.d["STENCILSFC"]:
      stencilsFC += stencil
      stencilsFC += ",\n"
    stencilsFC += "}" #end the std vector

    #also for stencilsFF
    stencilsFF = '{' #start the std vector
    for stencil in self.d["STENCILSFF"]:
      stencilsFF += stencil
      stencilsFF += ",\n"
    stencilsFF += "}" #end the std vector


    return f"""
  int _spacetreeId;
  const int _faceDofs = {self.d["FACE_CARDINALITY"]};
  const int _cellDofs = {self.d["CELL_CARDINALITY"]};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsFF"]},{self.d["stencilColsFF"]},double> > _stencilsFF = 
    {stencilsFF};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsCF"]},{self.d["stencilColsCF"]},double> > _stencilsCF = 
    {stencilsCF};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsFC"]},{self.d["stencilColsFC"]},double> > _stencilsFC = 
    {stencilsFC};

  const tarch::la::Matrix<{self.d["stencilRowsCC"]},{self.d["stencilColsCC"]},double> _stencilCC = {self.d["STENCILCC"]};
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

  def get_constructor_body(self):
    return f"""
    _spacetreeId = treeNumber;
"""


  def get_body_of_operation(self, operation_name):
    """!
    for now, only touch face first time
    """
    ## add touchcellfirsttime

    ## add touch face first time
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
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
"""
  def get_attributes(self):
    """!
    """

    return f"""
  int _spacetreeId;
"""

