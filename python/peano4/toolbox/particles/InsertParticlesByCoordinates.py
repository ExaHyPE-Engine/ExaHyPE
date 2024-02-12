# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class InsertParticlesByCoordinates(ActionSet):
    def __init__(
        self, particle_set, coordinates, initialisation_call, additional_includes=""
    ):
        """

        particle_set: ParticleSet

        N: number of particles

        coordinates: [ [Float,Float,Float] ]
          Coordinates for the particle. So you can write

                coordinates=[ [0.0,0.0,0.0], [0.2,0.2,0.0]]

          for example to insert two particles. The code will create two particles.
          This variant will work with 2d and 3d compiles, where the third
          coordinate is ignored if you compile in 2d. If you pass in 2d coordinates,
          then the code will fail (assertion) if you try to compile it in 3d.

        initialisation_call: String (C++ code snippet)
          Arbitrary code snippet which can work over a pointer particle. The object
          to which this pointer points to is properly allocated, and its
          coordinates are set. Furthermore, the object's search radius is
          initialised with zero. You can alter the particle attributes now, i.e.
          initialise the domain-specific data. Please also ensure you assign the
          particle a proper search (interaction) radius if the radius is of
          relevance for your application.

        """
        super(InsertParticlesByCoordinates, self).__init__(
            descend_invocation_order=1, parallel=False
        )

        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["N"] = len(coordinates)
        self.d["INITIALISATION_CALL"] = initialisation_call

        self._coordinates = coordinates
        self.additional_includes = additional_includes

    __Template_TouchCellFirstTime = jinja2.Template(
        """

  for(int i=0;i<{{N}};i++){
    // It is important to have this asymmetric comparisons with <= as we
    // need to ensure that particles right in the centre are either associated
    // with the vertex left or right.
    if (
      not marker.willBeRefined()
      and
      marker.isContained(_coordinates[i])
    ) {
      globaldata::{{PARTICLE}}* particle = new globaldata::{{PARTICLE}}();

      toolbox::particles::init(*particle,_coordinates[i],0.0);

      // See documentation of constructor on initialisation snippet
      {{INITIALISATION_CALL}}

      toolbox::particles::insertParticleIntoCell(
        marker,
        particle,
        fineGridVertices{{PARTICLES_CONTAINER}},
        _spacetreeId
      );
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/particles.h"
#include "toolbox/particles/ParticleFactory.h"
"""
            + self.additional_includes
        )
        return result.render(**self.d)

    def get_constructor_body(self):
        """!

        Initialise the coordinates

        This routine initialises the coordinates array, but it does so if and only
        if this hasn't been done yet. Each tree traversal creates its own action
        set. However, it would be ridiculously stupid to create the coordinates
        multiple times. Rather, I

        """
        result = (
            """
_spacetreeId              = treeNumber;
  
#pragma clang optimize off
tarch::multicore::Lock lock (_coordinatesSemaphore);
if (_coordinates==nullptr) {
  _coordinates = new tarch::la::Vector<Dimensions,double>["""
            + str(self.d["N"])
            + """];

    """
        )
        dimensions = -1
        for i in range(self.d["N"]):
            if len(self._coordinates[i]) == 3:
                result += (
                    " _coordinates["
                    + str(i)
                    + "](0)="
                    + str(self._coordinates[i][0])
                    + ";\n"
                )
                result += (
                    " _coordinates["
                    + str(i)
                    + "](1)="
                    + str(self._coordinates[i][1])
                    + ";\n"
                )
                result += " #if Dimensions==3\n"
                result += (
                    " _coordinates["
                    + str(i)
                    + "](2)="
                    + str(self._coordinates[i][2])
                    + ";\n"
                )
                result += " #endif\n"
                result += "\n"
                assert (
                    dimensions == -1 or dimensions == 3
                ), "inconsistent data: all coordinates have to have three entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 3
            if len(self._coordinates[i]) == 2:
                result += (
                    " _coordinates["
                    + str(i)
                    + "](0)="
                    + str(self._coordinates[i][0])
                    + ";\n"
                )
                result += (
                    " _coordinates["
                    + str(i)
                    + "](1)="
                    + str(self._coordinates[i][1])
                    + ";\n"
                )
                result += " assertion(Dimensions==2);\n"
                result += "\n"
                assert (
                    dimensions == -1 or dimensions == 2
                ), "inconsistent data: all coordinates have to have three entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 2
        result += """
}
#pragma clang optimize on
"""
        return result

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
tarch::la::Vector<Dimensions,double>*  """
            + full_qualified_classname
            + """::_coordinates = nullptr;
tarch::multicore::BooleanSemaphore     """
            + full_qualified_classname
            + """::_coordinatesSemaphore;
"""
        )

    def get_attributes(self):
        return f"""
    static tarch::multicore::BooleanSemaphore     _coordinatesSemaphore;
    static tarch::la::Vector<Dimensions,double>*  _coordinates;
    
    int                                        _spacetreeId;
"""
