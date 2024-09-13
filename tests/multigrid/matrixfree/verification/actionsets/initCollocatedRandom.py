from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2



class InitDofsCollocatedRandomRhs(ActionSet):
  """!
  In touchVertexFirstTime, we set the type of vertex we see (either Interior, Boundary or Coarse),
  as well as sending some values to the solver implementation file to be placed into the mesh.

  In touchCellFirstTime, we just determine the type, as we don't store anything on cells in 
  this solver.
  """

  templateTouchVertexFirstTime = """

  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isCorner = true;
    for (int d=0; d<Dimensions; d++) {
      isCorner &= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isCorner &= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isCorner;
  };

  logTraceInWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        if ( isCorner( marker.x() ) )
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Corner );
        else
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue( dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRand(  dof, 0.0); // vector v_random is set to 0 on the boundary
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRand( dof, dis(rng)); // vector v_random receives random values in the interior vertices

        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }


  logTraceOutWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );
        // perform initialisation proc
        // this does not work for more than one unknown per vertex

        tarch::la::Vector<TwoPowerD, double> randValues;
        for (int i=0; i<TwoPowerD; i++){
          randValues[i] = fineGridVertices{{SOLVER_NAME}}(i).getRand(0);
        }

        // multiply v_random by A_cg
        randValues = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * randValues;

        // place this into rhs
        for (int i=0; i<TwoPowerD; i++){
          fineGridVertices{{SOLVER_NAME}}(i).setRhs(
            fineGridVertices{{SOLVER_NAME}}(i).getRhs() + randValues(i)
          );
        }

      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """
  
  templateTouchVertexLastTime="""
  
  """

  def __init__(self,
               solver):
    super( InitDofsCollocatedRandomRhs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
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
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include <random>
#include "peano4/utils/Loop.h"
"""


class InitDofsCollocatedRandomTest2(InitDofsCollocatedRandomRhs):
  """!
  Exactly the same as above, with slightly modified templateTouchVertexFirstTime

  Don't specialise anything apart from one template. Everything else
  will be exactly the same
  """
  def __init__(self,
               solver):
    ## catch everything from the parent class
    super( InitDofsCollocatedRandomTest2, self ).__init__( solver )

  templateTouchVertexFirstTime = """

  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isCorner = true;
    for (int d=0; d<Dimensions; d++) {
      isCorner &= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isCorner &= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isCorner;
  };

  logTraceInWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        if ( isCorner( marker.x() ) )
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Corner );
        else
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue( dof, 0.0); // vector v_random is set to 0 on the boundary as the initial guess
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRand(  dof, 0.0); // vector v_random is set to 0 on the boundary
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          double randVal = dis(rng);
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, randVal); // vector v_random receives random values in the interior vertices 
                                                                // and used as the initial guess for the CG solver
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRand( dof, randVal); // the same vector v_random will be used to compute rhs = A_CG * v_random, 
                                                                // see templateTouchCellFirstTime in the parent class

          // use a continuous distribution x*y*(1-x)*(1-y) for "u_random" instead of the random one
          //fineGridVertex{{SOLVER_NAME}}.setRand( dof, marker.x()(0)*marker.x()(1)*(1.0-marker.x()(0))*(1.0-marker.x()(1)) );
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }


  logTraceOutWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());
   """
  

class InitCollocatedTest4(InitDofsCollocatedRandomRhs):
  templateTouchVertexFirstTime="""
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isCorner = true;
    for (int d=0; d<Dimensions; d++) {
      isCorner &= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isCorner &= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isCorner;
  };

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        if ( isCorner( marker.x() ) )
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Corner );
        else
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue( dof, dis(rng)); // vector v_random is set to 0 on the boundary as the initial guess
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setVector2(  dof, 0.0); // vector v_random is set to 0 on the boundary
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          // give it a random number
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, dis(rng)); // vector v_random receives random values in the interior vertices 
                                                                // and used as the initial guess for the CG solver
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setVector2( dof, 0.0); 
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }
  """

  templateTouchCellFirstTime="""
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );
      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
  
  """
  def __init__(self,
               solver):
    super( InitCollocatedTest4, self ).__init__(solver)
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

class InitCollocatedTest5(InitDofsCollocatedRandomRhs):
  templateTouchVertexFirstTime="""
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isCorner = true;
    for (int d=0; d<Dimensions; d++) {
      isCorner &= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isCorner &= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isCorner;
  };

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        if ( isCorner( marker.x() ) )
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Corner );
        else
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setRand(  dof, dis(rng)); // vector v_random is set to 0 on the boundary as the initial guess
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, 0.0);
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          // give it a random number
          fineGridVertex{{SOLVER_NAME}}.setRand(dof, dis(rng)); // vector v_random receives random values in the interior vertices 
                                                                // and used as the initial guess for the CG solver
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, 0.0);
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }
  """

  templateTouchCellFirstTime="""

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // also set the extras
        // start by fetching the random values from the cg vertices
        tarch::la::Vector<TwoPowerD, double> randoms;
        for (int i=0; i<TwoPowerD; i++) randoms(i) = fineGridVertices{{SOLVER_NAME}}(i).getRand(0);

        auto ACGValues = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix( marker.x(), marker.h() ) * randoms;
        auto MCGValues = repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() )          * randoms;

        for (int i=0; i<TwoPowerD; i++) {
          fineGridVertices{{SOLVER_NAME}}(i).setAcg( 0, ACGValues(i) + fineGridVertices{{SOLVER_NAME}}(i).getAcg(0) );
          fineGridVertices{{SOLVER_NAME}}(i).setMcg( 0, MCGValues(i) + fineGridVertices{{SOLVER_NAME}}(i).getMcg(0) );
        }
      }
    }
    break;

  }
  
  """

  def __init__(self,
               solver):
    super( InitCollocatedTest5, self ).__init__(solver)
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

class InitCollocatedTest7(InitDofsCollocatedRandomRhs):
  """!
  We just need random numbers in the inital guess and elsewhere zeroess
  """

  templateTouchVertexFirstTime = """

  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);


  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isCorner = true;
    for (int d=0; d<Dimensions; d++) {
      isCorner &= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isCorner &= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isCorner;
  };

  logTraceInWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        if ( isCorner( marker.x() ) )
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Corner );
        else
          fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue( dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, 0.0);

          fineGridVertex{{SOLVER_NAME}}.setRand( dof,  0.0); // vector v_random is set to 0 on the boundary
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          // set the ref. solution instead of random for debugging
          //fineGridVertex{{SOLVER_NAME}}.setValue(dof, std::sin( 2.0*tarch::la::PI * marker.x()(0) ) * std::sin( 2.0*tarch::la::PI * marker.x()(1) ) );
          //double randVal = dis(rng);
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, 0.0);
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, 0.0);

          double randVal = std::sin( 2.0*tarch::la::PI * marker.x()(0) ) * std::sin( 2.0*tarch::la::PI * marker.x()(1) );

          // save the random vector separately
          fineGridVertex{{SOLVER_NAME}}.setRand( dof, randVal); 
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }


  logTraceOutWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime="""
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );
      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
  
  """
  
  templateTouchVertexLastTime="""
  
  """