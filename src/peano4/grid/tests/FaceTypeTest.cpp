// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceTypeTest.h"

#include "peano4/grid/FaceType.h"
#include "peano4/grid/VertexType.h"

#include <vector>

peano4::grid::tests::FaceTypeTest::FaceTypeTest():
  TestCase("peano4::grid::tests::FaceTypeTest") {}

void peano4::grid::tests::FaceTypeTest::run() { testMethod(testGetFaceType); }

void peano4::grid::tests::FaceTypeTest::testGetFaceType() {
  VertexType vertexTypes[TWO_POWER_D];

  auto fill = [&](VertexType t) {
    for (int i = 0; i < TWO_POWER_D; ++i)
      vertexTypes[i] = t;
  };

  auto isOnFace = [&](int vertexIndex, int faceNumber) -> bool {
    const int normal = faceNumber % DIMENSIONS;
    const int side   = (faceNumber >= DIMENSIONS) ? 1 : 0;
    return (((vertexIndex >> normal) & 1) == side);
  };

  auto faceVertices = [&](int faceNumber) {
    std::vector<int> indices;
    indices.reserve(TWO_POWER_D / 2);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (isOnFace(i, faceNumber))
        indices.push_back(i);
    }
    return indices;
  };

  // All vertices persistent -> all faces persistent
  fill(VertexType::Persistent);
  for (int face = 0; face < 2 * DIMENSIONS; ++face) {
    validateEquals(FaceType::Persistent, getFaceType(vertexTypes, face));
  }

  // The remaining tests are face-by-face and must be invariant to the opposite half of vertices.

  for (int face = 0; face < 2 * DIMENSIONS; ++face) {
    const auto fv = faceVertices(face);
    validateTrue(fv.size() >= 1);

    // All vertices on the face hanging, all others persistent -> face is Hanging
    fill(VertexType::Persistent);
    for (int idx : fv) {
      vertexTypes[idx] = VertexType::Hanging;
    }
    validateEquals(FaceType::Hanging, getFaceType(vertexTypes, face));

    // All vertices on the face are Hanging or New (at least one New) -> face is New
    fill(VertexType::Persistent);
    for (int idx : fv) {
      vertexTypes[idx] = VertexType::Hanging;
    }
    vertexTypes[fv[0]] = VertexType::New;
    validateEquals(FaceType::New, getFaceType(vertexTypes, face));

    // All vertices on the face are Hanging or Delete (at least one Delete) -> face is Delete
    fill(VertexType::Persistent);
    for (int idx : fv) {
      vertexTypes[idx] = VertexType::Hanging;
    }
    vertexTypes[fv[0]] = VertexType::Delete;
    validateEquals(FaceType::Delete, getFaceType(vertexTypes, face));

    // Mixture of New and Delete (and maybe Hanging), but no Persistent on face -> face is Persistent
    // (Because it is neither all New/Hanging nor all Delete/Hanging, and not all Hanging.)
    if (fv.size() >= 2) {
      fill(VertexType::Hanging);
      vertexTypes[fv[0]] = VertexType::New;
      vertexTypes[fv[1]] = VertexType::Delete;
      // Make opposite half "noisy" and ensure it does not matter.
      for (int i = 0; i < TWO_POWER_D; ++i) {
        if (!isOnFace(i, face)) {
          vertexTypes[i] = VertexType::Persistent;
        }
      }
      validateEquals(FaceType::Persistent, getFaceType(vertexTypes, face));
    }

    // Any Persistent on the face forces Persistent (early return in implementation)
    fill(VertexType::Hanging);
    vertexTypes[fv[0]] = VertexType::Persistent;
    // Again, set opposite half to Hanging to ensure only face-adjacent vertices matter.
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (!isOnFace(i, face)) {
        vertexTypes[i] = VertexType::Hanging;
      }
    }
    validateEquals(FaceType::Persistent, getFaceType(vertexTypes, face));
  }
}
