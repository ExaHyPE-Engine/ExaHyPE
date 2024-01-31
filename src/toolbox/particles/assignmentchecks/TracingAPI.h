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
     */
    namespace assignmentchecks {
      namespace internal {
        /**
         * Key for internal bookkeeping of vertex-particle association
         *
         * We assume that a particle is unique due to its name (species) and
         * its position. Particle information will be stored in a (hash) map
         * and therefore needs a smaller operator.
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
          enum class Type { NotFound, AssignToSieveSet, AssignToVertex, Erase, RemoveFromVertex };

          Type                                  type;
          bool                                  isLocal;
          tarch::la::Vector<Dimensions, double> vertexX;
          tarch::la::Vector<Dimensions, double> vertexH;
          int                                   treeId;

          /**
           * Construct an event which identifies a vertex assignment or the removal from a vertex.
           */
          Event(Type type_, bool isLocal_, const tarch::la::Vector<Dimensions, double>& vertexX_, const tarch::la::Vector<Dimensions, double>& vertexH_, int treeId_);

          /**
           * Construct an event that identifies the association to the sieve set or an erase.
           */
          Event(Type type_, bool isLocal_, int treeId_);

          /**
           * Construct an invalid event. Is used as return value if no data is
           * found in the database.
           */
          Event(Type type_);

          std::string toString() const;
        };

        using ParticleEvents = std::vector<Event>;
        using MeshSweepData  = std::map<ParticleIdentifier, ParticleEvents>;

        class Database {
        private:
          std::vector<MeshSweepData> _data;

        public:
          Database();

          /**
           * Add a new snapshot for a new mesh sweep
           */
          void addMeshSweep();

          /**
           * Has a fitting entry
           *
           * This routine runs backwards (from new to old) through the
           * database and searches for a particle entry with the specified
           * position and name. If there's none or the last logged entry for
           * this particle is an erase event, then the routine returns an
           * invalid event.
           */
          Event getEntry(const ParticleIdentifier& identifier, int spactreeId, bool hasToBeLocal) const;

          /**
           * Has fitting entry
           *
           * Search backwards through history and return latest event that
           * matches the species (name) and position handed in.
           */
          Event getEntry(const ParticleIdentifier& identifier) const;

          /**
           * Print history of one particle
           */
          std::string particleHistory(const ParticleIdentifier& identifier) const;

          void addEvent(const ParticleIdentifier& identifier, const Event& event);
        };
      } // namespace internal

      /**
       * Inform API that this is the end of a grid run-throuch
       *
       * Delegates to the corresponding method in the database.
       */
      void endMeshSweep();

      /**
       * Log that a particle is erased
       *
       * This should solely happen for virtual particles, i.e. halo particles.
       * Before this function is called, the corresponding particle has to be
       * removed from its associated vertex.
       */
      void eraseParticle(const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId);

      /**
       * Assign a particle to a vertex
       *
       * To make this a valid operation, the particle has to be "free", i.e. it
       * may not be assigned to any other vertex yet. Consequently, the particle
       * is either new (created or flying in from somewhere else), or we have
       * triggered removeParticleFromVertex() before.
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
        int                                          treeId
      );

      /**
       * Remove particle from vertex
       *
       * To make this API call valid, the particle has to be assigned to a
       * vertex beforehand. So there has to be a assignParticleToVertex()
       * before this one. However, we might run into a situation where a
       * code updates a particle's position and then removes this particle
       * (with the new position) from its vertex, as it now has moved. In
       * this case, we cannot match the particle anymore to a previous
       * entry, as we do not use global particle ids. So, it is possible
       * that a code removes a particle from a vertex which we haven't
       * "seen" yet before. Basically, the only thing left for us is to ensure
       * that particles are not removed from a vertex twice.
       */
      void removeParticleFromVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId
      );

      /**
       * Assign a particle to a sieve set
       *
       * To make this routine valid, the particle may not be assigned to a
       * vertex anymore, i.e. removeParticleFromVertex() has to be called
       * before.
       */
      void assignParticleToSieveSet(const std::string& particleName, const tarch::la::Vector<Dimensions, double>& particleX, bool isLocal, int treeId);

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
