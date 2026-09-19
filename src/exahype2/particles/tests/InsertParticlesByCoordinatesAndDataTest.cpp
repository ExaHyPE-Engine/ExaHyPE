// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InsertParticlesByCoordinatesAndDataTest.h"

#include "exahype2/particles/InsertParticlesByCoordinatesAndData.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "tarch/la/Vector.h"

#if defined(__NVCOMPILER)
// The compiler nvc++ diagnoses members that only the debug and assertion builds call as unused.
#pragma diag_push
#pragma diag_suppress declared_but_not_referenced
#endif

namespace {
  constexpr int    DataEntriesPerParticle = 4;
  constexpr int    TreeId                 = 5;
  constexpr double CellWidth              = 0.2;
  constexpr double CellCentre             = 0.5;
  constexpr double LowerFace              = CellCentre - CellWidth / 2.0;

  /** Stands in for a generated tracer particle. */
  struct TracerParticle final {
    enum class ParallelState { Local, Virtual };

    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    tarch::la::Vector<DIMENSIONS, double> cellH{0.0};
    double                                searchRadius{0.0};
    int                                   number[2]{0, 0};
    int                                   partId{0};
    ParallelState                         parallelState{ParallelState::Virtual};
    std::bitset<peano4::CellsPerVertex>   containedInAdjacentCell{0};
    std::vector<double>                   data = std::vector<double>(DataEntriesPerParticle, 0.0);

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }
    void                 setX(const tarch::la::Vector<DIMENSIONS, double>& value) { x = value; }
    void                 setDebugX(const tarch::la::Vector<DIMENSIONS, double>& value) { x = value; }
    void                 setDebugH(const tarch::la::Vector<DIMENSIONS, double>& value) { cellH = value; }
    void                 setCellH(const tarch::la::Vector<DIMENSIONS, double>& value) { cellH = value; }
    [[nodiscard]] double getSearchRadius() const { return searchRadius; }
    void                 setSearchRadius(const double value) { searchRadius = value; }
    [[nodiscard]] int    getNumber(const int index) const { return number[index]; }
    void                 setNumber(const int index, const int value) { number[index] = value; }
    [[nodiscard]] int    getPartid() const { return partId; }
    void                 setPartid(const int value) { partId = value; }
    void                 setParallelState(const ParallelState value) { parallelState = value; }
    void                 setContainedInAdjacentCell(const std::bitset<peano4::CellsPerVertex>& value) {
      containedInAdjacentCell = value;
    }
    [[nodiscard]] const std::vector<double>& getData() const { return data; }
    void                                     setData(const int index, const double value) { data[index] = value; }
    [[nodiscard]] std::string                toString() const { return "particle"; }
  };

  /** Stands in for the generated particle container of one vertex. */
  struct ParticleSet final {
    std::vector<TracerParticle*> particles{};

    void                      addParticle(TracerParticle* const particle) { particles.push_back(particle); }
    [[nodiscard]] std::string toString() const { return "particleSet"; }
  };

  using Vertices = std::array<ParticleSet, peano4::VerticesPerCell>;

  peano4::dataview::VertexEnumerator<ParticleSet> enumerateVertices(Vertices& vertices) {
    std::array<int, peano4::VerticesPerCell> slots{};
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      slots[vertexNumber] = vertexNumber;
    }
    return peano4::dataview::VertexEnumerator<ParticleSet>(vertices.data(), slots.data(), 0);
  }

  peano4::dataview::CellMarker createCellMarker() {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(CellCentre);
    return marker;
  }

  std::vector<TracerParticle*> collectParticles(const Vertices& vertices) {
    std::vector<TracerParticle*> result;
    for (const ParticleSet& vertex : vertices) {
      for (TracerParticle* const particle : vertex.particles) {
        result.push_back(particle);
      }
    }
    return result;
  }

  void deleteParticles(const Vertices& vertices) {
    for (const ParticleSet& vertex : vertices) {
      for (TracerParticle* const particle : vertex.particles) {
        delete particle;
      }
    }
  }
} // namespace

#if defined(__NVCOMPILER)
#pragma diag_pop
#endif

exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest::InsertParticlesByCoordinatesAndDataTest() :
  tarch::tests::TestCase("exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest") {}

void exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest::run() {
  testMethod(testEveryCoordinateOfTheDataArrayEntersOneCellOnce);
  testMethod(testCoordinateOnALowerFaceEntersTheCellAboveTheDomainWallOnly);
}

/**
 * A coordinate inside the cell becomes a particle with its unknowns, a coordinate outside stays
 * out and a second call inserts no particle again.
 */
void exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest::
  testEveryCoordinateOfTheDataArrayEntersOneCellOnce() {
  constexpr int UnknownsPerParticle = 2;

  std::vector<double> particleData;
  for (const double position : {0.45, 0.9}) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      particleData.push_back(position);
    }
    particleData.push_back(position * 10.0);
    particleData.push_back(position * 100.0);
  }
  std::vector<bool> particleAlreadyInserted(2, false);

  Vertices vertices;
  auto     vertexEnumerator         = enumerateVertices(vertices);
  int      particleNumberOnThisTree = 0;

  const tarch::la::Vector<DIMENSIONS, double> domainOffset(0.0);
  for (int call = 0; call < 2; call++) {
    exahype2::particles::insertParticlesByCoordinatesAndData<TracerParticle, UnknownsPerParticle>(
      createCellMarker(),
      particleData,
      particleAlreadyInserted,
      domainOffset,
      vertexEnumerator,
      TreeId,
      particleNumberOnThisTree
    );
  }

  const std::vector<TracerParticle*> particles = collectParticles(vertices);
  validateEquals(static_cast<int>(particles.size()), 1);
  validateEquals(particleNumberOnThisTree, 1);
  validateTrue(particleAlreadyInserted[0]);
  validateFalse(particleAlreadyInserted[1]);
  validateEquals(particles[0]->getNumber(0), TreeId);
  validateEquals(particles[0]->getNumber(1), 0);
  validateNumericalEquals(particles[0]->getX()(0), 0.45);
  validateNumericalEquals(particles[0]->getData()[0], 4.5);
  validateNumericalEquals(particles[0]->getData()[1], 45.0);

  deleteParticles(vertices);
}

/**
 * A coordinate on the lower face of the cell enters the cell below it, unless that face is the
 * domain wall, which has no cell below.
 */
void exahype2::particles::tests::InsertParticlesByCoordinatesAndDataTest::
  testCoordinateOnALowerFaceEntersTheCellAboveTheDomainWallOnly() {
  constexpr int UnknownsPerParticle = 0;

  const std::vector<double> particleData(DIMENSIONS, LowerFace);

  Vertices          verticesInsideTheDomain;
  auto              insideEnumerator              = enumerateVertices(verticesInsideTheDomain);
  int               particleNumberInsideTheDomain = 0;
  std::vector<bool> insertedInsideTheDomain(1, false);

  exahype2::particles::insertParticlesByCoordinatesAndData<TracerParticle, UnknownsPerParticle>(
    createCellMarker(),
    particleData,
    insertedInsideTheDomain,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    insideEnumerator,
    TreeId,
    particleNumberInsideTheDomain
  );

  validateEquals(static_cast<int>(collectParticles(verticesInsideTheDomain).size()), 0);
  validateFalse(insertedInsideTheDomain[0]);

  Vertices          verticesAtTheDomainWall;
  auto              wallEnumerator                = enumerateVertices(verticesAtTheDomainWall);
  int               particleNumberAtTheDomainWall = 0;
  std::vector<bool> insertedAtTheDomainWall(1, false);

  exahype2::particles::insertParticlesByCoordinatesAndData<TracerParticle, UnknownsPerParticle>(
    createCellMarker(),
    particleData,
    insertedAtTheDomainWall,
    tarch::la::Vector<DIMENSIONS, double>(LowerFace),
    wallEnumerator,
    TreeId,
    particleNumberAtTheDomainWall
  );

  validateEquals(static_cast<int>(collectParticles(verticesAtTheDomainWall).size()), 1);
  validateTrue(insertedAtTheDomainWall[0]);

  deleteParticles(verticesAtTheDomainWall);
}
