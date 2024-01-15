// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <map>
#include <vector>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace particles {
    /**
     * @namespace toolbox::particles::assignmentchecks Correctness checks of particle-mesh assignment
     *
     * This API is inspired by SWIFT's swift2::dependencychecks as introduced by
     * Mladen: We keep some history of particle-mesh associations, trace new mesh
     * assignments and validate them against the recorded history. All the tracing
     * degenerates to empty routines if we work in release mode, so there should not
     * be any overhead.
     *
     * Most toolbox routines do automatically report against this API. However, to
     * make the checks work, you have to report particle movements manually.
     *
     * Furthermore, you might want to invoke toolbox::particles::assignmentchecks::startMeshSweep()
     * prior to your traversals to ensure that you get a nice break-down of all
     * mesh transitions.
     */
    namespace assignmentchecks {
      namespace internal {
        /**
         * Key for internal bookkeeping of vertex-particle association
         *
         * We assume that a particle is unique due to its name (species) and
         * its position. Particle information will be stored in a (hash) map
         * and therefore needs a smaller operator.
         *
         * ## Sorting
         *
         * As the keys work with floating point data, we have to take
         * truncation errors into account. However, we have to be careful
         * with this: We still need a total order if particles are not
         * exactly the same. Originally, I did attempt to realise the
         * routine as follows:
         *
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * if (particleName < rhs.particleName) {
         *   assertion2( *this!=rhs, toString(), rhs.toString() );
         *   return true;
         * }
         * if (particleName > rhs.particleName) {
         *   assertion2( *this!=rhs, toString(), rhs.toString() );
         *   return false;
         * }
         * for (int d = 0; d < Dimensions; d++) {
         *   if (tarch::la::smaller(particleX(d), rhs.particleX(d))) {
         *     assertion2( *this!=rhs, toString(), rhs.toString() );
         *     return true;
         *   }
         *   if (tarch::la::greater(particleX(d), rhs.particleX(d))) {
         *     assertion2( *this!=rhs, toString(), rhs.toString() );
         *     return false;
         *   }
         * }
         * assertion2( *this==rhs, toString(), rhs.toString() );
         * return false;
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         *
         * This variant does not work, as it is too fuzzy. Indeed, we have
         * to check first if two particles are equal (including the floating
         * point precision). If they are, then the comparison operator <
         * has to return false. If they are not, we fall back to bit-wise
         * comparisons.
         */
        struct ParticleIdentifier {
          std::string                           particleName;
          tarch::la::Vector<Dimensions, double> particleX;
          ParticleIdentifier(const std::string& particleName__, const tarch::la::Vector<Dimensions, double>& particleX__);

          bool operator==(const ParticleIdentifier& rhs) const;
          bool operator<(const ParticleIdentifier& rhs) const;

          std::string toString() const;
        };

        /**
         * Event
         *
         * An event is an assocation or de-association of a particle and a
         * vertex.
         */
        struct Event {
          enum class Type { NotFound, AssignToSieveSet, AssignToVertex, Erase, DetachFromVertex, MoveWhileAssociatedToVertex };

          Type                                  type;
          bool                                  isLocal;
          tarch::la::Vector<Dimensions, double> vertexX;
          tarch::la::Vector<Dimensions, double> previousParticleX;
          tarch::la::Vector<Dimensions, double> vertexH;
          int                                   treeId;
          std::string                           trace;

          /**
           * Construct an event which identifies a vertex assignment or the removal from a vertex.
           */
          Event(
            Type                                          type_,
            bool                                          isLocal_,
            const tarch::la::Vector<Dimensions, double>&  vertexX_,
            const tarch::la::Vector<Dimensions, double>&  vertexH_,
            int                                           treeId_,
            const std::string&                            trace_
          );

          /**
           * Construct a move event
           *
           * This event should always be tied to an identifier in the global map
           * which uses the new position as key.
           */
          Event(
            Type                                          type_,
            const tarch::la::Vector<Dimensions, double>&  previousParticleX,
            int                                           treeId_,
            const std::string&                            trace_
          );

          /**
           * Construct an event that identifies the association to the sieve set or an erase.
           */
          Event(Type type_, bool isLocal_, int treeId_,             const std::string& trace_);

          /**
           * Construct an invalid event. Is used as return value if no data is
           * found in the database.
           */
          Event(Type type_);

          std::string toString() const;
        };

        using ParticleEvents = std::vector<Event>;

        /**
         * Mesh sweep data set
         *
         * All the traces bookkept for one single mesh sweep. This is a plain
         * extension of the std::map which simply adds a unique name on top.
         * So all the original semantics of a map are preserved.
         */
        class MeshSweepData: public std::map<ParticleIdentifier, ParticleEvents> {
          public:
            MeshSweepData(const std::string& meshSweepName);

            std::string getName() const;

          private:
            const std::string _meshSweepName;
        };

        class Database {
        private:
          std::vector<MeshSweepData> _data;

        public:
          static constexpr int AnyTree = -1;

          Database();

          /**
           * Add a new snapshot for a new mesh sweep
           */
          void startMeshSweep(const std::string&  meshSweepName);

          /**
           * We enumerate the mesh sweeps. This routine returns the number of
           * the current one.
           */
          int getCurrentMeshSweep() const;

          static constexpr int SearchWholeDatabase = 0;
          static constexpr int DoNotFollowParticleMovementsInDatabase = -1;

          /**
           * Has a fitting entry
           *
           * This routine runs backwards (from new to old) through the
           * database and searches for a particle entry with the specified
           * position and name. If there's none or the last logged entry for
           * this particle is an erase event, then the routine returns an
           * invalid event.
           *
           * @param spacetreeId Either a valid spacetree or AnyTree if you don't care
           */
          Event getEntry(
            const ParticleIdentifier&  identifier,
            int                        spacetreeId,
            int                        firstNRecentEntriesToSkip = SearchWholeDatabase
          ) const;

          /**
           * Print history of one particle
           */
          std::string particleHistory(const ParticleIdentifier& identifier) const;

          /**
           * Count total number of entries for a particle
           */
          int totalEntries(const ParticleIdentifier& identifier) const;

          void addEvent(const ParticleIdentifier& identifier, const Event& event);

          /**
           * Dump the whole database
           */
          std::string toString() const;
        };
      } // namespace internal

      /**
       * Inform API that this is the end of a grid run-throuch
       *
       * Delegates to the corresponding method in the database.
       */
      void startMeshSweep(const std::string& meshSweepName);

      /**
       * Log that a particle is erased
       *
       * This should solely happen for virtual particles, i.e. halo particles.
       * Before this function is called, the corresponding particle has to be
       * removed from its associated vertex.
       */
      void eraseParticle(
        const std::string&                            particleName,
        const tarch::la::Vector<Dimensions, double>&  particleX,
        bool                                          isLocal,
        int                                           treeId,
        const std::string&                            trace
      );

      /**
       * Record the movement of a particle
       *
       * If oldParticleX equals newParticleX, the function degenerates to nop
       * and nothing is traced. Otherwise, we run some consistency checks and
       * then put a move event into our database.
       */
      void moveParticle(
         const std::string&                            particleName,
         const tarch::la::Vector<Dimensions, double>&  oldParticleX,
         const tarch::la::Vector<Dimensions, double>&  newParticleX,
         int                                           treeId,
         const std::string&                            trace
      );

      /**
       * Assign a particle to a vertex
       *
       * Take the particle, perform some consistency checks, and then add the
       * particle event to our history database. The consistency checks differ
       * for local from virtual particles.
       *
       * ## Consistency checks for local particles
       *
       * - If we have
       * - The particle has to be "free", i.e. it may not be assigned to any
       *   other vertex yet. Consequently, the particle is either new (created or flying in from somewhere else), or we have
       * triggered detachParticleFromVertex() before.
       *
       * ## Consistency check for virtual particles
       *
       * - The particle has to be "free", i.e. it may not be assigned to any
       *   other vertex yet. The particle has to be new. If it is an update of
       *   a halo copy, we would expect the corresponing merge() to replace
       *   this copy. A replacement manifests in an event sequence of
       *   detach->erase->assign.
       *
       * To make this a valid operation,
       *
       * Alternatively, we might have a virtual particle here. In this case,
       * we can ensure that all vertices are associated to the same vertex on
       * all trees. They also have to belong to the same mesh level.
       */
      void assignParticleToVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace
      );

      /**
       * Remove particle from vertex
       *
       * To make this API call valid, the particle has to be assigned to a
       * vertex beforehand. So there has to be a assignParticleToVertex()
       * before this one.
       */
      void detachParticleFromVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace
      );

      /**
       * Assign a particle to a sieve set
       *
       * To make this routine valid, the particle may not be assigned to a
       * vertex anymore, i.e. detachParticleFromVertex() has to be called
       * before.
       */
      void assignParticleToSieveSet(
        const std::string&                            particleName,
        const tarch::la::Vector<Dimensions, double>&  particleX,
        bool                                          isLocal,
        int                                           treeId,
        const std::string&                            trace
      );

      /**
       * Find out particle name
       *
       * In some places, it is convenient to construct the particle name from
       * the type. However, some compilers only give you something similar to
       *
       * ~~~~~~~~~~~~~~~~~~~~
       * N10benchmarks6swift23noh10globaldata9HydroPartE
       * ~~~~~~~~~~~~~~~~~~~~
       *
       * which is really not what you want. This routine tries to reconstruct a
       * meaningful result.
       */
      template <typename Particle>
      std::string pruneTypeName() {
        std::string result    = typeid(Particle).name();
        std::size_t lastDigit = result.find_last_of("0123456789");
        if (lastDigit != std::string::npos) {
          result = result.substr(lastDigit + 1);
        }
        assertion2(result.size() > 1, typeid(Particle).name(), result);
        return result.size() > 1 ? result.substr(0, result.size() - 1) : result;
      }
    } // namespace assignmentchecks
  }   // namespace particles
} // namespace toolbox



