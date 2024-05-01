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
        super(InsertParticlesFromHDF5File, self).__init__(
            descend_invocation_order=1, parallel=False
        )

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

    using namespace ::swift2::kernels::legacy::kernelHydro; // for kernel_gamma

    std::list< tarch::la::Vector<Dimensions,double> > coords = _fileReaderHDF5.getParticlesWithinVoxel( marker.x(), marker.h(), true );

    /* Retrieve the rest of fields for same set of particles */
    std::vector< tarch::la::Vector<Dimensions,double> > VelocityWithinVoxel = _fileReaderHDF5.getVelocityWithinVoxel();
    std::vector <double> MassWithinVoxel            = _fileReaderHDF5.getMassWithinVoxel();
    std::vector <double> SmoothingLengthWithinVoxel = _fileReaderHDF5.getSmoothingLengthWithinVoxel();
    std::vector <double> InternalEnergyWithinVoxel  = _fileReaderHDF5.getInternalEnergyWithinVoxel();
    std::vector <int>    PartidWithinVoxel          = _fileReaderHDF5.getParticleIDsWithinVoxel();

    int indexEntry = 0;
    const double SmlMin = globaldata::{{PARTICLE}}::getSmlMin();
    const double SmlMax = globaldata::{{PARTICLE}}::getSmlMax();


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
      double Density         = 1.0;
      double Mass            = MassWithinVoxel[indexEntry];
      double SmoothingLength = SmoothingLengthWithinVoxel[indexEntry];
      double InternalEnergy  = InternalEnergyWithinVoxel[indexEntry];
      int    ParticleID      = PartidWithinVoxel[indexEntry];

      if (SmoothingLength > SmlMax){
        logWarning("InsertParticlesFromHDF5File.py",
          "particle smoothing length > global smoothing length max;"
          << " h= " << SmoothingLength << " h_max= " << SmlMax <<
          "setting it to 0.9 * Max");
        SmoothingLength = 0.9 * SmlMax;
      }

      /*
       * Other attributes not set in HDF5 file
       */

      tarch::la::Vector<Dimensions,double> acceleration;

      /* Compute the pressure from the EoS */
      const double Pressure = ::swift2::kernels::legacy::eos::gas_pressure_from_internal_energy( Density, InternalEnergy );

      /* Sound speed */
      const double Soundspeed = ::swift2::kernels::legacy::eos::gas_soundspeed_from_internal_energy( InternalEnergy );

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

      /* Internal search radius used by Peano.
       * We set the maximum allowed value, R = grid_size/2.  */
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0);
      const double searchRadiusGlobal = SmlMax * kernel_gamma;

      // TODO: use radius from smoothing length, not global one.
      // Solve this once you add variable search radii.

      if (tarch::la::greater(searchRadiusGlobal, 0.5 * GridSize_x)) {
          std::ostringstream out;
          out << " Global upper limit for search radius is > mesh size. You need to increase your cell size" <<
              " or reduce hydro_h_max in your project python file." <<
              " 0.5 * GridSize:" << 0.5 * GridSize_x <<
              " max global search radius: " << searchRadiusGlobal;
          logError("InsertParticlesFromHDF5File.py:", out.str());
      }

      newParticle->setSearchRadius( searchRadiusGlobal );

      newParticle->setPressure  ( Pressure );
      newParticle->setSoundSpeed( Soundspeed );

      // Vectors
      for (int d=0; d<Dimensions; d++) {
         newParticle->setA(d,0);
#if Dimensions > 2
         newParticle->setRot_v(d, 0);
#endif
      }

#if Dimensions <= 2
      /* For 2D sims, we only get one component */
      newParticle->setRot_v(0);
#endif

      /* Initialise the "extended" arrays */
      newParticle->setV_full( newParticle->getV() );
      newParticle->setU_full( newParticle->getU() );

      newParticle->setUDot  (0.0);
      newParticle->setHDot  (0.0);
      newParticle->setWcount(0.0);
      newParticle->setF(0.0);
      newParticle->setRho_dh(0.0);
      newParticle->setWcount_dh(0.0);


//      newParticle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      newParticle->setSmoothingLengthIterCount(0);

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
        fineGridVertices{{PARTICLE}}Set,
        _spacetreeId
      );
    }

    logDebug( "touchVertexFirstTime(...)", "assigned " << coords.size() << " particle(s) to vertex "
                                                       << marker.toString() << " on tree " << _spacetreeId );

  }
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  if (_particleNumberOnThisTree==0) {
    logWarning( "endIteration()", "inserted " << _particleNumberOnThisTree << " particle(s) on tree " << _spacetreeId );
  }
  else {
    logInfo( "endIteration()", "inserted " << _particleNumberOnThisTree << " particle(s) on tree " << _spacetreeId << " (boundary vertices might host redundant particle data)" );
  }
  #if !defined(Parallel) and !defined(SharedMemoryParallelisation)
  assertion( _particleNumberOnThisTree>0 );
  #endif
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
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/particles.h"
#include "toolbox/particles/FileReaderHDF5.h"
#include "toolbox/particles/ParticleFactory.h"

#include "Constants.h"

#include "swift2/kernels/legacy/kernel_hydro.h"
#include "swift2/kernels/legacy/equation_of_state.h"

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

    def get_attributes(self):
        return """
  static tarch::multicore::BooleanSemaphore _semaphore;

  int                                        _particleNumberOnThisTree;
  int                                        _spacetreeId;
  static toolbox::particles::FileReaderHDF5  _fileReaderHDF5;
"""
