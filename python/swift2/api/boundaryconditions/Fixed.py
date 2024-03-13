# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class Fixed(ActionSet):
    """!

    Fixed boundary conditions

    Impose fixed boundary conditions within an action set. Such an action set 
    can be injected into each and every grid sweep. 
    @ref swift_boundary_conditions "Swift's generic boundary condition"
    remarks discuss this realisation variant. It is quite heavy-weight and can 
    yield invalid states prior to the very first time step. 

    The action set is used by adding it after you have lowered the Swift 2
    formalism into a Peano 4 project:
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~
project.additional_action_sets_per_solver_step.append(
    swift2.api.boundaryconditions.Fixed(particle_set=particle_set, 
                                        damp_particles_as_they_approach_boundary=True
                                        )
)
    ~~~~~~~~~~~~~~~~~~~~~~~~~
    
    @param particle_set: swift2.particle.Particle
      The particle set for which we implement the boundary conditions.
            
    @param relative_domain_halo: Positive float smaller than 0.5
      The relative boundary in which we actually check particles at
      all. This one refers to the cells adjacent to the boundary. So 
      if you pass in a 0.1, it means that 10% of this cell will next
      to the boundary will actually be read as "on the boundary".
    
    @param damp_particles_as_they_approach_boundary: Boolean
      
    """

    def __init__(self, 
                 particle_set, 
                 relative_domain_halo=0.1
                 ):
        """!

        Fixed boundary condition

        Fixed boundary conditions are very difficult to impose within SPH. This
        is a very simplistic realisation of fixed boundary conditions, where
        particles are forced to come to rest once they approach the domain
        boundary.

        """
        super(Fixed, self).__init__()
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["VELOCITY_ATTRIBUTE"] = particle_set.particle_model.velocity.get_accessor_name()
        self.d["RELATIVE_DOMAIN_HALO"] = relative_domain_halo
 
    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    __Template_TouchVertexLastTime = jinja2.Template(
        """
    // check only boundary vertices
    if (::swift2::boundaryconditions::isVertexOnGlobalBoundary(marker,DomainOffset,DomainSize)) {
      for (auto& particle: fineGridVertex{{PARTICLES_CONTAINER}}) {
        ::swift2::boundaryconditions::applyFixedBoundaryCondition(
          *particle,
          marker,
          DomainOffset,
          DomainSize,
          {{RELATIVE_DOMAIN_HALO}},
          _spacetreeId
        );
      } // for loop over particles
    } // if on marker
    """
    )

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
#include "swift2/kernels/ParticleSetIterators.h"
#include "swift2/boundaryconditions/Utils.h"
#include "swift2/boundaryconditions/FixedBoundary.h"
"""
        )
        return result.render(**self.d)

    def get_constructor_body(self):
        return (
            super(Fixed, self).get_constructor_body()
            + """
            _spacetreeId              = treeNumber;
          """
        )

    def get_attributes(self):
        return """
  int  _spacetreeId;
"""
