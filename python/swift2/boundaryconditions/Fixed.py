# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class Fixed(ActionSet):
    """!
    
    Fixed boundary conditions
    
    This is a very primitive implementation of the fixed boundary conditions. 
    We shrink the computational domain by domain_halo. Every particle which 
    approaches the domain boundary closer than domain_halo will effectively 
    be put back on the boundary of the shrinked domain. We do not allow 
    particles to leave the shrinked domain and, hence, they should not leave
    the computational domain, which is bigger, either.
    
    This concept alone does not ensure that particles never leave their domain.
    Their velocity might be so big that they effectively could be pressed out
    of the domain. So we need to create another buffer within the domain which
    ensures that the particles cannot tunnel through the buffer within one 
    time step. For this, we have to take the maximum velocity into account, and
    we have to damp the velocities as they run risk to approach our domain 
    boundaries way too fast.
    
    """
    def __init__(self, 
                 particle_set, 
                 domain_halo = 0.001
                 ):
        """!

        Fixed boundary condition
        
        Fixed boundary conditions are very difficult to impose within SPH. This
        is a very simplistic realisation of fixed boundary conditions, where 
        particles are forced to come to rest once they approach the domain 
        boundary.
                
        """
        super( Fixed, self ).__init__()
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["VELOCITY_ATTRIBUTE"]  = particle_set.particle_model.velocity.get_accessor_name()
        self.d["DOMAIN_HALO"]         = domain_halo

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


    __Template_TouchVertexLastTime = jinja2.Template( """
    constexpr double DomainHalo     = {{DOMAIN_HALO}};
    // check only boundary vertices
    if ( 
        tarch::la::oneSmallerEquals( marker.x() - 0.5 * marker.h(), DomainOffset )
        or 
        tarch::la::oneGreaterEquals( marker.x() + 0.5 * marker.h(), DomainOffset + DomainSize )
    ) {
      for (auto& particle: fineGridVertex{{PARTICLES_CONTAINER}}) {
        const double MaxDt          = particle->getSpecies().getMaxTimeStepSize();
        const double MaxV           = particle->getSpecies().getMaxVelocity();
        const double RelativeBuffer = 0.5  * tarch::la::min(marker.h()) + MaxV * MaxDt;
        for (int d=0; d<Dimensions; d++) {
          // damp velocity left
          if ( 
            particle->getX(d) < DomainOffset(d) + RelativeBuffer
            and
            particle->get{{VELOCITY_ATTRIBUTE}}(d)<0.0
          ) {
            double scaleOriginalVelocity = (particle->getX(d) - DomainOffset(d)) / RelativeBuffer; 
            assertion5(scaleOriginalVelocity>=0.0, scaleOriginalVelocity, particle->getX(), DomainOffset(d), RelativeBuffer, marker.h()(d) );
            assertion5(scaleOriginalVelocity<=1.0, scaleOriginalVelocity, particle->getX(), DomainOffset(d), RelativeBuffer, marker.h()(d) );
            particle->set{{VELOCITY_ATTRIBUTE}}(d,scaleOriginalVelocity * particle->get{{VELOCITY_ATTRIBUTE}}(d));
            logDebug( "touchVertexLastTime(...)", "damp velocity along x" << d << "-axis by " << scaleOriginalVelocity << ": " << particle->toString() );
          }
          
          // reposition particle left
          if ( tarch::la::smallerEquals( particle->getX(d), DomainOffset(d) + DomainHalo ) ) {
            particle->setX(d,DomainOffset(d) + DomainHalo);
            particle->setV(d,0.0 );
            logDebug( "touchVertexLastTime(...)", "relocate particle along x" << d << "-axis: " << particle->toString() );
          }
          
          // damp velocity right
          if ( 
            particle->getX(d) > DomainOffset(d) + DomainSize(d) - RelativeBuffer
            and
            particle->get{{VELOCITY_ATTRIBUTE}}(d)>0.0
          ) {
            double scaleOriginalVelocity = (DomainOffset(d) + DomainSize(d) - particle->getX(d)) / RelativeBuffer; 
            assertion5(scaleOriginalVelocity>=0.0, scaleOriginalVelocity, particle->getX(), DomainOffset(d), RelativeBuffer, marker.h()(d) );
            assertion5(scaleOriginalVelocity<=1.0, scaleOriginalVelocity, particle->getX(), DomainOffset(d), RelativeBuffer, marker.h()(d) );
            particle->set{{VELOCITY_ATTRIBUTE}}(d,scaleOriginalVelocity * particle->get{{VELOCITY_ATTRIBUTE}}(d));
            logDebug( "touchVertexLastTime(...)", "damp velocity along x" << d << "-axis by " << scaleOriginalVelocity << ": " << particle->toString() );
          }
          
          // reposition particle right
          if ( tarch::la::greaterEquals( particle->getX(d), DomainOffset(d) + DomainSize(d) - DomainHalo ) ) {
            particle->setX(d,DomainOffset(d) + DomainSize(d) - DomainHalo );
            particle->setV(d,0.0 );
            logDebug( "touchVertexLastTime(...)", "relocate particle along x" << d << "-axis: " << particle->toString() );
          }
        } // for loop over dimensions
      } // for loop over particles
    } // if on marker
    """)
    
    
    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self.d)
        return result

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "Constants.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)
