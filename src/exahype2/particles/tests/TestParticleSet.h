// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles::tests {
  class TestParticle;
  class TestParticleSet;
  class TestVertices;

  void recordInitialParticleAssignment(
    const TestParticle&                          particle,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId
  );
} // namespace exahype2::particles::tests

/**
 * Particle of the particle-action-set tests: It carries the attributes the templates of
 * exahype2::particles read and write, and nothing else.
 */
class exahype2::particles::tests::TestParticle final {
public:
  enum class ParallelState : std::uint8_t { Local, Virtual };

  TestParticle()                               = default;
  TestParticle(const TestParticle&)            = default;
  TestParticle(TestParticle&&)                 = default;
  TestParticle& operator=(const TestParticle&) = default;
  TestParticle& operator=(TestParticle&&)      = default;
  ~TestParticle()                              = default;

  [[nodiscard]] tarch::la::Vector<DIMENSIONS, double> getX() const;
  void                                                setX(const tarch::la::Vector<DIMENSIONS, double>& value);

  [[nodiscard]] tarch::la::Vector<DIMENSIONS, double> getCellH() const;
  void                                                setCellH(const tarch::la::Vector<DIMENSIONS, double>& value);

  [[nodiscard]] double getSearchRadius() const;
  void                 setSearchRadius(double value);

  [[nodiscard]] int getPartid() const;
  void              setPartid(int value);

  [[nodiscard]] ParallelState getParallelState() const;
  void                        setParallelState(ParallelState value);

  [[nodiscard]] std::bitset<peano4::CellsPerVertex> getContainedInAdjacentCell() const;
  void setContainedInAdjacentCell(const std::bitset<peano4::CellsPerVertex>& value);

  [[nodiscard]] int getNumber(int index) const;
  void              setNumber(int index, int value);

  [[nodiscard]] const std::array<double, 1>& getData() const;
  void                                       setData(int index, double value);

#if PEANO_DEBUG
  void setDebugX(const tarch::la::Vector<DIMENSIONS, double>& value);
  void setDebugH(double value);
#endif

  [[nodiscard]] std::string toString() const;

private:
  tarch::la::Vector<DIMENSIONS, double> _x{0.0};
  tarch::la::Vector<DIMENSIONS, double> _cellH{0.0};
  double                                _searchRadius{0.0};
  int                                   _partid{0};
  ParallelState                         _parallelState{ParallelState::Local};
  std::bitset<peano4::CellsPerVertex>   _containedInAdjacentCell{};
  std::array<int, 2>                    _number{0, 0};
  std::array<double, 1>                 _data{0.0};
};

/**
 * The container owns the particles it holds, and its static members stand in for the rank-wide
 * sieve set and statistics of a generated container, which clearTestState() resets.
 */
class exahype2::particles::tests::TestParticleSet final {
public:
  using DoFType        = TestParticle;
  using Container      = std::vector<TestParticle*>;
  using iterator       = Container::iterator;
  using const_iterator = Container::const_iterator;

  TestParticleSet()                                  = default;
  TestParticleSet(const TestParticleSet&)            = delete;
  TestParticleSet(TestParticleSet&&)                 = delete;
  TestParticleSet& operator=(const TestParticleSet&) = delete;
  TestParticleSet& operator=(TestParticleSet&&)      = delete;
  ~TestParticleSet()                                 = default;

  static bool                     hasParticlesToBeSievedIntoVertices();
  static std::list<TestParticle*> getParticlesToBeSievedIntoVertex(const peano4::dataview::VertexMarker& marker);

  static void updateLiftDropStatistics(
    int numberOfLifts,
    int numberOfDrops,
    int numberOfLiftsIntoSieveSet,
    int numberOfDropsFromSieveSet,
    int numberOfDropsIntoHorizontalTreeDecomposition
  );

  static void updateNumberOfLocalAndExpiredParticles(
    int                                 numberOfRemainingLocalParticles,
    int                                 numberOfExpiredHaloParticles,
    int                                 numberOfParticlesThatHaveLeftTheirDomain,
    const std::set<std::array<int, 2>>& remainingLocalParticleNumbers
  );

  static void clearTestState();

  static void                                              addParticleToBeSieved(TestParticle* particle);
  [[nodiscard]] static const std::vector<TestParticle*>&   getParticlesLiftedIntoSieveSet();
  [[nodiscard]] static const std::array<int, 5>&           getRecordedLiftDropStatistics();
  [[nodiscard]] static const std::array<int, 3>&           getRecordedParticleStateStatistics();
  [[nodiscard]] static const std::set<std::array<int, 2>>& getRecordedLocalParticleNumbers();
  [[nodiscard]] static int                                 getNumberOfDeletedParticles();

  void     mergeWithParticle(TestParticle* particle, const peano4::dataview::VertexMarker& marker, int treeId);
  void     particlesCanNotBeLiftedLocally();
  iterator particleCanNotBeLiftedLocally(iterator particle);
  void     addParticle(TestParticle* particle);
  iterator deleteParticle(iterator particle);
  void     grabParticles(TestParticleSet& sourceParticleSet);
  iterator grabParticle(iterator particle, TestParticleSet& sourceParticleSet);
  void     deleteParticles();

  iterator                     begin();
  iterator                     end();
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] int         size() const;
  [[nodiscard]] bool        isValid() const;
  [[nodiscard]] std::string toString() const;

private:
  static std::vector<TestParticle*>   _particlesToBeSieved;
  static std::vector<TestParticle*>   _particlesLiftedIntoSieveSet;
  static std::array<int, 5>           _recordedLiftDropStatistics;
  static std::array<int, 3>           _recordedParticleStateStatistics;
  static std::set<std::array<int, 2>> _recordedLocalParticleNumbers;
  static int                          _numberOfDeletedParticles;

  Container _container{};
};

/** Hands the containers of the vertices of one cell to a template that indexes them. */
class exahype2::particles::tests::TestVertices final {
public:
  TestVertices()                               = default;
  TestVertices(const TestVertices&)            = default;
  TestVertices(TestVertices&&)                 = default;
  TestVertices& operator=(const TestVertices&) = delete;
  TestVertices& operator=(TestVertices&&)      = delete;
  ~TestVertices()                              = default;

  explicit TestVertices(TestParticleSet* vertices);

  TestParticleSet& operator()(int vertex) const;

private:
  TestParticleSet* _vertices{nullptr};
};
