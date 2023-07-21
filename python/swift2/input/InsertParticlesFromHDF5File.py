# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer

import numpy as np


class InsertParticlesFromHDF5File(ActionSet):
    def __init__(self, particle_set, filename):
        """!

        Read position from HDF5 and insert particles


            Read an HDF5 file and insert particles into the simulation box. This is
            typically used to read an Initial Condition file.

            particle_set: ParticleSet

            filename: String
              Name of the HDF5 file that we want to read.

            @TODO implement some bool checks, e.g. is this a cosmological simulation?
            hydro scheme?

        """
        super(InsertParticlesFromHDF5File, self).__init__(priority=1, parallel=False)

        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self._filename = filename

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  if ( not marker.hasBeenRefined() and not marker.willBeRefined() ) {
    tarch::multicore::Lock lock( _semaphore );

    /*
     * List of particles that overlap with the current cell.
     */

    std::list< tarch::la::Vector<Dimensions,double> > coords = _fileReaderHDF5.getParticlesWithinVoxel( marker.x(), marker.h(), true );

    /* Retrieve the rest of fields for same set of particles */
    std::vector< tarch::la::Vector<Dimensions,double> > VelocityWithinVoxel = _fileReaderHDF5.getVelocityWithinVoxel();
    std::vector <double> MassWithinVoxel            = _fileReaderHDF5.getMassWithinVoxel();
    std::vector <double> SmoothingLengthWithinVoxel = _fileReaderHDF5.getSmoothingLengthWithinVoxel();
    std::vector <double> InternalEnergyWithinVoxel  = _fileReaderHDF5.getInternalEnergyWithinVoxel();
    std::vector <int>    PartidWithinVoxel          = _fileReaderHDF5.getParticleIDsWithinVoxel();

    int indexEntry = 0;
    const double hmin = globaldata::{{PARTICLE}}::getSmlMin();
    const double hmax = globaldata::{{PARTICLE}}::getSmlMax();


    // Now loop over these particle coordinates
    for (auto& p: coords) {

      // Create a particle pointer
      globaldata::{{PARTICLE}}* newParticle = new globaldata::{{PARTICLE}}();

      // Initialise the particle with x=coords and SearchRadius=0
      toolbox::particles::init(*newParticle, p, 0.0);
      _particleNumberOnThisTree++;

      /*
       * Initialise the full set of particle attributes
       */

      tarch::la::Vector<Dimensions,double> Velocity;
      /* Density is not encoded in all SWIFT HDF5 IC files as it is recomputed
      by the density loop anyway. Here we just initialse to an arbitrary value */
      const double Density         = 1.0;
      const double Mass            = MassWithinVoxel[indexEntry];
      const double SmoothingLength = SmoothingLengthWithinVoxel[indexEntry];
      const double InternalEnergy  = InternalEnergyWithinVoxel[indexEntry];
      const int    ParticleID      = PartidWithinVoxel[indexEntry];

      /*
       * Other attributes not set in HDF5 file
       */

      tarch::la::Vector<Dimensions,double> acceleration;

      /* Internal search radius used by Peano.
       * We set the maximum allowed value, R = grid_size/2.
       */
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0);

      /* Compute the pressure from the EoS */
      const double Pressure = gas_pressure_from_internal_energy( Density, InternalEnergy );

      /* Sound speed */
      const double Soundspeed = gas_soundspeed_from_internal_energy( InternalEnergy );

      /* Now set the values */
      newParticle->setV              ( Velocity );
      newParticle->setDensity        ( Density );
      newParticle->setMass           ( Mass );
      newParticle->setSmoothingLength( SmoothingLength );

      newParticle->setU              ( InternalEnergy );

      /*
       * Initialize remaining fields
       * @TODO some of these should be initialised elsewhere to keep this
       * initialisation script general for any hydro scheme, etc.
       */

      newParticle->setSmoothingLengthPrevious( SmoothingLength );

      newParticle->setSearchRadius(GridSize_x / 2.0);

      newParticle->setPressure  ( Pressure );
      newParticle->setSoundSpeed( Soundspeed );

      // Vectors
      for (int d=0; d<Dimensions; d++) {
         newParticle->setA          (d,0);
      }

      /* For 2D sims, we only get one component */
      newParticle->setRot_v(0);

      /* Initialise the "extended" arrays */
      newParticle->setV_full( newParticle->getV() );
      newParticle->setU_full( newParticle->getU() );

      newParticle->setUDot  (0.0);
      newParticle->setHDot  (0.0);
      newParticle->setWcount(0.0);
      newParticle->setNcount(0  );
      newParticle->setF     (0.0);
      newParticle->setRho_dh(0.0);
      newParticle->setWcount_dh(0.0);

      newParticle->setHLeft (hmin);
      newParticle->setHRight(hmax);

//      newParticle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      newParticle->setHasConverged(false);
      newParticle->setHasNoNeighbours(false);
      newParticle->setResidual(0.0);
      newParticle->setIterCount(0);
      newParticle->setIsBoundaryParticle(false);

      // Artificial viscosity scheme
      newParticle->setDiv_v   (0.0);
      newParticle->setV_sig_AV(2.0 * newParticle->getSoundSpeed());
      newParticle->setBalsara (0.0);

      // Set particle id
      newParticle->setPartid( ParticleID );

      /*
       * All particle attributes for SPH have thus been set
       */

      indexEntry++;

      toolbox::particles::insertParticleIntoCell(
        marker,
        newParticle,
        fineGridVerticeshydroPartSet
      );
    }

    logDebug( "touchVertexFirstTime(...)", "assigned " << coords.size() << " particle(s) to vertex "
                                                       << marker.toString() << " on tree " << _spacetreeId );

  }
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  logInfo( "endIteration()", "inserted " << _particleNumberOnThisTree << " particle(s) on tree "
                                         << _spacetreeId
                                         << " (boundary vertices might host redundant particle data)" );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
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
#include "toolbox/particles/FileReaderHDF5.h"
#include "toolbox/particles/ParticleFactory.h"

#include "Constants.h"

#include "swift2/kernels/kernel_hydro.h"
#include "swift2/kernels/equation_of_state.h"

#include <fstream>
#include <string>
#include <vector>
"""
        )
        return result.render(**self.d)

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
tarch::multicore::BooleanSemaphore """
            + full_qualified_classname
            + """::_semaphore;
toolbox::particles::FileReaderHDF5 """
            + full_qualified_classname
            + """::_fileReaderHDF5;
"""
        )

    def get_constructor_body(self):
        return (
            """
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;

  tarch::multicore::Lock lock( _semaphore );

  if ( _fileReaderHDF5.empty() )
    _fileReaderHDF5.readHDF5File( \""""
            + self._filename
            + """\" );
"""
        )

    # def get_destructor_body(self):
    #  return """
    # """

    def get_attributes(self):
        return """
  static tarch::multicore::BooleanSemaphore _semaphore;

  int                                        _particleNumberOnThisTree;
  int                                        _spacetreeId;
  static toolbox::particles::FileReaderHDF5  _fileReaderHDF5;
"""
