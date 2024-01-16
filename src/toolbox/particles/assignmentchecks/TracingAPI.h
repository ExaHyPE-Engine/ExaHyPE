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
        struct ParticleIdentifier;
        struct Event;
        class MeshSweepData;
        class Database;

        /**
         * Key for internal bookkeeping of vertex-particle association
         *
         * We assume that a particle is unique due to its name (species) and
         * its position. Particle information will be stored in a (hash) map
         * and therefore needs a smaller operator.
         *
         * ## Sorting
         *
         * The sorting is very simple: We first use the name as criterion if
         * two particles are the same or one is smaller than the others. For
         * this, we can use the string's < operator. If two particles have the
         * same name, we first sort along the x axis, then (for those particles
         * with the same x value) along the y axis, and so forth.
         *
         * As the keys work with floating point data, we have to take
         * truncation errors into account. Originally, I did attempt to realise
         * the routine as follows:
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
         * This variant does not work, as it is too fuzzy. Situations
         * as the one below can arise:
         *
         * - We have A!<B, as A and B overlap.
         * - We have B!<A, as B and A overlap (and B is actually further right).
         * - Consequently, A==B. This follows the C++ map logic.
         * - We have B!<C, as B and C overlap.
         * - We have C!<B, as B and C overlap.
         * - Consequently, B==C.
         *
         * However, we clearly have A!=C which contradicts the statements above.
         *
         * I originally envisaged a very non-elegant solution to this problem:
         *
         * - The < operator which determines the storage order in the map is
         *   bit-wise and neglects the fact that we have floating-point data.
         * - Whenever we construct a new particle identifier, we first check
         *   if there's an entry in the database which is "close". If so, we
         *   use this identifier rather than a completely new one.
         *
         * I found this approach to be extremely expensive. Every time we
         * construct a particle identifier, we might run throught the whole
         * database. In most cases however, we will terminate the database
         * run through early: As soon as getEntry() finds a fit, it returns
         * immediately. So it is unsuccessful entry constructions which really
         * hurt us. These arise for assignParticleToVertex() for new particles.
         * In this case, we cannot avoid that we run through the whole
         * database. If a particle moves however, we don't have to search for
         * existing entries. This movement will yield a new position which is
         * not in there yet. So we disable the search here (though this
         * obviously means we cannot check if two particles coincide
         * spatially).
         */
        struct ParticleIdentifier {
          friend class Database;

          std::string                           particleName;
          tarch::la::Vector<Dimensions, double> particleX;

          bool operator==(const ParticleIdentifier& rhs) const;
          bool operator<(const ParticleIdentifier& rhs) const;

          std::string toString() const;

          private:
            /**
             * Never create a particle identifier manually
             *
             * Instead, use Database::createParticleIdentifier() to construct
             * one. This routine takes care of floating point in accuracies.
             */
            ParticleIdentifier(const std::string& particleName__, const tarch::la::Vector<Dimensions, double>& particleX__);
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
         *
         * Please see ParticleIdentifier for an in-depth discussion of the
         * comparison key to be used here.
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
             * Create a particle identifier
             *
             * This routine runs through the whole database and looks for a
             * particle identifier which is equal to the parameters. Equal here
             * uses the comparison operator of ParticleIdentifier::operator==()
             * and hence is floating-point aware.
             *
             * If it finds no matching entry, it returns a new identifier with
             * the precise arguments. If it finds one "close enough", it returns
             * this one instead of one matching the precise arguments. See
             * ParticleIdentifier's discussion on map storage for further
             * details.
             */
            ParticleIdentifier createParticleIdentifier(
              const std::string&                            particleName,
              const tarch::la::Vector<Dimensions, double>&  particleX,
              bool                                          fitToExistingParticles = true
            ) const;

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
           * We expect that identifier has been constructed through
           * createParticleIdentifier() to accommodate floating-point inaccuracies.
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

          std::string lastMeshSweepSnapshot() const;

          /**
           * Count total number of entries for a particle
           */
          int totalEntries(const ParticleIdentifier& identifier) const;

          /**
           * Add a new event to the database
           *
           * If there is not yet an entry for identifier in the current
           * snapshot, we create one. Afterwards, we append event to this
           * new dataset for particle identifier.
           */
          void addEvent(ParticleIdentifier identifier, const Event& event);

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
       * ## Consistency checks
       *
       * ### Consistency checks for local particles
       *
       * - If we have
       * - The particle has to be "free", i.e. it may not be assigned to any
       *   other vertex yet. Consequently, the particle is either new (created or flying in from somewhere else), or we have
       * triggered detachParticleFromVertex() before.
       *
       * ### Consistency check for virtual particles
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
       *
       * ## Scenarios
       *
       * In the scenario below, we study the interface between two ranks. It
       * is denoted below as grey partition (one left rank) and a blueish
       * partition (on the right rank). They have kind of a halo layer of
       * width one, which is a logical construct, i.e. not maintained manually.
       *
       * @image html scenario00_particle-through-domain-wall.png
       *
       * ### Particle is held redundantly on both ranks
       *
       * As long as the particle does not move, each rank updates its local
       * copy and sends it over to its adjacent partner. Here, the received
       * copy replaces the old one (even though they should hold exactly the
       * same data). We end up with a detach->erase->assign sequence in merge()
       * right at the begin of a traversal.
       *
       * Let there be one particle close to the domain boundary, such that it
       * is held both on the left and right subdomain. If the particle sits
       * directly on the boundary (left scenario), both ranks hold them as a
       * local particle. If it is slightly shifted, one holds a local version,
       * the other one a virtual one. If the particle move, it either moves
       * through the boundary (middle) or away from it (right).
       *
       * If both subdomains hold a local copy (left), both will move the
       * particle. On one of the ranks, the particle will end up outside of the
       * domain (grey). On the other one (blue), it will end up "more inside
       * the domain". The trace resembles
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * (HydroPart,[0.166756,0.333378])
       * ============================
       * sweep #9 (Drift):(moved-while-associated-to-vertex,[0.166667,0.333333]->x_new,tree=0,trace=toolbox::particles::assignmentchecks::traceParticleMovements())
       * ============================
       * (HydroPart,[0.166667,0.333333])
       * ============================
       * sweep #9 (Drift):(detach-from-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(erase,local=1,tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(assign-to-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(detach-from-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(erase,local=1,tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(assign-to-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * @todo This trace is wrong!!!!! there is no move on tree 1 and therefore all data are inconsistent
       *
       * In the next sweep, the grey partition will receive a
       * If lift
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * (HydroPart,[0.166756,0.333378])
       * ============================
       * sweep #9 (Drift):(moved-while-associated-to-vertex,[0.166667,0.333333]->x_new,tree=0,trace=toolbox::particles::assignmentchecks::traceParticleMovements())->(detach-from-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=0,trace=HydroPart::touchVertexLastTime())->(assign-to-vertex,local=1,x=[0.222222,0.333333],h=[0.111111,0.111111],tree=0,trace=HydroPart::touchVertexLastTime())
       * ============================
       * (HydroPart,[0.166667,0.333333])
       * ============================
       * sweep #9 (Drift):(detach-from-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(erase,local=1,tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(assign-to-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=0,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(detach-from-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(erase,local=1,tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())->(assign-to-vertex,local=1,x=[0.148148,0.333333],h=[0.037037,0.037037],tree=1,trace=benchmarks::swift2::noh::vertexdata::HydroPartSet::merge())
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * ### Particle is local on one rank and virtual on the other
       *
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



