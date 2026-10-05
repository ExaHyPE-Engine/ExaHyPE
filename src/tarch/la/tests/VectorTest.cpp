// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VectorTest.h"

#include "tarch/la/Vector.h"
#include "tarch/la/VectorCompare.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include <sycl/sycl.hpp>
#endif

tarch::la::tests::VectorTest::VectorTest() :
  tarch::tests::TestCase("tarch::la::tests::VectorTest") {}

void
tarch::la::tests::VectorTest::run() {
  testMethod(testConstruction);
  testMethod(testListConstructionIsAConstantExpression);
  testMethod(testDefaultConstructionZeroesEveryEntry);
  testMethod(testAssignment);
  testMethod(testVectorOperations);
  testMethod(testVectorScalarOperations);
  testMethod(testVectorVectorOperations);
  testMethod(testVectorCompare);
  testMethod(testVectorVectorCompare);
  testMethod(testSpecialisedVectorLayouts);
  testMethod(testAdditionalVectorOperations);
  testMethod(testSpecialisedVectorMembers);
  testMethod(testNaNComparesUnequal);
  testMethod(testSmallerComparisonsHonourTheTolerance);
  testMethod(testNorm2SquaredHoldsASwapOfTheFirstTwoComponents);
}

void
tarch::la::tests::VectorTest::testVectorVectorCompare() {
  std::map<tarch::la::Vector<2, double>, int, tarch::la::VectorCompare<2>> map;

  const tarch::la::Vector<2, double> a = {0.0, 0.0};
  const tarch::la::Vector<2, double> b = {0.0, 0.3};
  const tarch::la::Vector<2, double> c = {1.0, 0.0};

  validate(map.empty());

  map[a] = 1;
  map[b] = 2;
  map[c] = 3;

  validateEquals(static_cast<int>(map.size()), 3);
  validate(map.find(a) != map.end());
  validate(map.find(b) != map.end());
  validate(map.find(c) != map.end());
  validateEquals(map.begin()->second, 1);
}

void
tarch::la::tests::VectorTest::testConstruction() {
  const tarch::la::Vector<1, int> vector1(1);
  validateEquals(vector1[0], 1);

  const tarch::la::Vector<2, int> vector2(1);
  validateEquals(vector2[0], 1);
  validateEquals(vector2[1], 1);

  const tarch::la::Vector<2, int> vector3 = {1, 2};
  validateEquals(vector3[0], 1);
  validateEquals(vector3[1], 2);

  const tarch::la::Vector<3, int> vector4 = {1, 2, 3};
  validateEquals(vector4[0], 1);
  validateEquals(vector4[1], 2);
  validateEquals(vector4[2], 3);

  const tarch::la::Vector<3, int> vector5(vector4);
  validateEquals(vector5[0], 1);
  validateEquals(vector5[1], 2);
  validateEquals(vector5[2], 3);

  const tarch::la::Vector<3, int> vector6 = vector5;
  validateEquals(vector6[0], 1);
  validateEquals(vector6[1], 2);
  validateEquals(vector6[2], 3);
}

void
tarch::la::tests::VectorTest::testListConstructionIsAConstantExpression() {
  constexpr tarch::la::Vector<1, double> Vector1{1.5};
  constexpr tarch::la::Vector<2, double> Vector2{1.5, 2.5};
  constexpr tarch::la::Vector<3, int>    Vector3{1, 2, 3};
  constexpr tarch::la::Vector<4, bool>   Vector4{true, false, true, false};
  constexpr tarch::la::Vector<5, double> Vector5{1.5, 2.5, 3.5, 4.5, 5.5};
  validateNumericalEquals(Vector1[0], 1.5);
  for (int entry = 0; entry < 2; entry++) {
    validateNumericalEqualsWithParams1(Vector2[entry], entry + 1.5, entry);
  }
  for (int entry = 0; entry < 3; entry++) {
    validateEqualsWithParams1(Vector3[entry], entry + 1, entry);
  }
  for (int entry = 0; entry < 4; entry++) {
    const bool entryIsEven = entry % 2 == 0;
    validateEqualsWithParams1(Vector4[entry], entryIsEven, entry);
  }
  for (int entry = 0; entry < 5; entry++) {
    validateNumericalEqualsWithParams1(Vector5[entry], entry + 1.5, entry);
  }
}

void
tarch::la::tests::VectorTest::testDefaultConstructionZeroesEveryEntry() {
  constexpr tarch::la::Vector<1, int>    Vector1{};
  constexpr tarch::la::Vector<2, int>    Vector2{};
  constexpr tarch::la::Vector<3, double> Vector3{};
  constexpr tarch::la::Vector<4, int>    Vector4{};
  constexpr tarch::la::Vector<5, double> Vector5{};
  validateEquals(Vector1[0], 0);
  for (int entry = 0; entry < 2; entry++) {
    validateEqualsWithParams1(Vector2[entry], 0, entry);
  }
  for (int entry = 0; entry < 3; entry++) {
    validateEqualsWithParams1(Vector3[entry], 0.0, entry);
  }
  for (int entry = 0; entry < 4; entry++) {
    validateEqualsWithParams1(Vector4[entry], 0, entry);
  }
  for (int entry = 0; entry < 5; entry++) {
    validateEqualsWithParams1(Vector5[entry], 0.0, entry);
  }
}

void
tarch::la::tests::VectorTest::testAssignment() {
  tarch::la::Vector<3, int> vector = {1, 2, 3};
  validateEquals(vector[0], 1);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 3);

  vector = {1, 2, 3};
  validateEqualsWithParams1(vector[0], 1, ::toString(vector));
  validateEqualsWithParams1(vector[1], 2, ::toString(vector));
  validateEqualsWithParams1(vector[2], 3, ::toString(vector));
}

void
tarch::la::tests::VectorTest::testVectorOperations() {
  tarch::la::Vector<3, int> vector = {1, -2, 2};

  int result = tarch::la::sum(vector);
  validateEquals(result, 1);

  result = tarch::la::norm1(vector);
  validateEquals(result, 5.0);

  vector = {1, 2, 3};
  validateEquals(tarch::la::indexMin(vector), 0);
  validateEquals(tarch::la::indexMax(vector), 2);
  validateEquals(tarch::la::min(vector), 1);
  validateEquals(tarch::la::max(vector), 3);

  const tarch::la::Vector<3, int> vector3 = {1, 2, 3};
  std::ostringstream              stream;
  stream << vector3;
  const std::string vector3String(stream.str());
  validateEquals(vector3String, std::string("[1,2,3]"));

  const tarch::la::Vector<2, double> vector2 = {-1.5, 3.0};
  validateNumericalEquals(tarch::la::norm2Squared(vector2), 11.25);
  validateNumericalEquals(tarch::la::normMax(vector2), 3.0);
  validateEquals(tarch::la::count(tarch::la::Vector<2, int>{4, 4}, 4), 2);
  validate(tarch::la::allEntriesAreTheSame(tarch::la::Vector<2, int>{4, 4}));

  const tarch::la::Vector<4, int> vector4 = {3, -1, 5, 2};
  validateEquals(tarch::la::sum(vector4), 9);
  validateEquals(tarch::la::volume(tarch::la::Vector<4, int>{2, 3, 4, 5}), 120);
  validateEquals(tarch::la::indexMin(vector4), 1);
  validateEquals(tarch::la::indexMax(vector4), 2);
  validateEquals(tarch::la::min(vector4), -1);
  validateEquals(tarch::la::max(vector4), 5);
  validateEquals(tarch::la::maxAbs(vector4), 5);
  validate(tarch::la::contains(vector4, 2));
  validate(not tarch::la::contains(vector4, 7));
  validate(tarch::la::isEntryFinite(tarch::la::Vector<4, double>{1.0, 2.0, 3.0, 4.0}));

  const tarch::la::Vector<4, double> floorResult =
    tarch::la::floor(tarch::la::Vector<4, double>{1.2, -1.2, 3.99, 4.01});
  validateNumericalEquals(floorResult[0], 1.0);
  validateNumericalEquals(floorResult[1], -2.0);
  validateNumericalEquals(floorResult[2], 3.0);
  validateNumericalEquals(floorResult[3], 4.0);

  const tarch::la::Vector<3, double> inverted = tarch::la::invertEntries(tarch::la::Vector<3, double>{2.0, 4.0, 8.0});
  validateNumericalEquals(inverted[0], 0.5);
  validateNumericalEquals(inverted[1], 0.25);
  validateNumericalEquals(inverted[2], 0.125);

  const tarch::la::Vector<5, int> generic5 = {1, -2, 3, -4, 5};
  validateEquals(tarch::la::sum(generic5), 3);
  validateEquals(tarch::la::norm1(generic5), 15);
  validateEquals(tarch::la::norm2Squared(generic5), 55);
  validateEquals(tarch::la::normMax(generic5), 5);
  validateEquals(tarch::la::indexMin(generic5), 3);
  validateEquals(tarch::la::indexMax(generic5), 4);
  validateEquals(tarch::la::min(generic5), -4);
  validateEquals(tarch::la::max(generic5), 5);
  validateEquals(tarch::la::maxAbs(generic5), 5);
  validate(tarch::la::contains(generic5, 3));
  validate(not tarch::la::contains(generic5, 9));
  validateEquals(tarch::la::count(tarch::la::Vector<5, int>{1, 2, 1, 3, 1}, 1), 3);

  const tarch::la::Vector<7, double> generic7Floor =
    tarch::la::floor(tarch::la::Vector<7, double>{1.2, 2.9, -1.2, -3.0, 0.0, 4.01, 5.99});
  validateNumericalEquals(generic7Floor[0], 1.0);
  validateNumericalEquals(generic7Floor[1], 2.0);
  validateNumericalEquals(generic7Floor[2], -2.0);
  validateNumericalEquals(generic7Floor[3], -3.0);
  validateNumericalEquals(generic7Floor[4], 0.0);
  validateNumericalEquals(generic7Floor[5], 4.0);
  validateNumericalEquals(generic7Floor[6], 5.0);

  const tarch::la::Vector<5, double> generic5Inverted =
    tarch::la::invertEntries(tarch::la::Vector<5, double>{2.0, 4.0, 5.0, 8.0, 10.0});
  validateNumericalEquals(generic5Inverted[0], 0.5);
  validateNumericalEquals(generic5Inverted[1], 0.25);
  validateNumericalEquals(generic5Inverted[2], 0.2);
  validateNumericalEquals(generic5Inverted[3], 0.125);
  validateNumericalEquals(generic5Inverted[4], 0.1);
}

void
tarch::la::tests::VectorTest::testVectorScalarOperations() {
  tarch::la::Vector<3, int> vector(0);

  vector += 1;
  validateEquals(vector[0], 1);
  validateEquals(vector[1], 1);
  validateEquals(vector[2], 1);

  vector -= 2;
  validateEquals(vector[0], -1);
  validateEquals(vector[1], -1);
  validateEquals(vector[2], -1);

  vector *= -2;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 2);

  vector /= 2;
  validateEquals(vector[0], 1);
  validateEquals(vector[1], 1);
  validateEquals(vector[2], 1);

  vector = vector * 2;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 2);

  vector = 2 * vector;
  validateEquals(vector[0], 4);
  validateEquals(vector[1], 4);
  validateEquals(vector[2], 4);

  vector = vector / 4;
  validateEquals(vector[0], 1);
  validateEquals(vector[1], 1);
  validateEquals(vector[2], 1);

  vector = vector + 1;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 2);

  vector = vector - 1;
  validateEqualsWithParams1(vector[0], 1, ::toString(vector));
  validateEqualsWithParams1(vector[1], 1, ::toString(vector));
  validateEqualsWithParams1(vector[2], 1, ::toString(vector));

  tarch::la::Vector<2, double> vector2 = {1.0, 2.0};
  vector2 += 0.5;
  validateNumericalEquals(vector2[0], 1.5);
  validateNumericalEquals(vector2[1], 2.5);
  vector2 -= 1.0;
  validateNumericalEquals(vector2[0], 0.5);
  validateNumericalEquals(vector2[1], 1.5);
  vector2 *= 4.0;
  validateNumericalEquals(vector2[0], 2.0);
  validateNumericalEquals(vector2[1], 6.0);
  vector2 /= 2.0;
  validateNumericalEquals(vector2[0], 1.0);
  validateNumericalEquals(vector2[1], 3.0);
  vector2 = vector2 + 2.0;
  validateNumericalEquals(vector2[0], 3.0);
  validateNumericalEquals(vector2[1], 5.0);
  vector2 = vector2 - 1.0;
  validateNumericalEquals(vector2[0], 2.0);
  validateNumericalEquals(vector2[1], 4.0);
  vector2 = 0.5 * vector2;
  validateNumericalEquals(vector2[0], 1.0);
  validateNumericalEquals(vector2[1], 2.0);
  vector2 = vector2 / 2.0;
  validateNumericalEquals(vector2[0], 0.5);
  validateNumericalEquals(vector2[1], 1.0);

  const tarch::la::Vector<4, int> vector4 = {2, 2, 3, 4};
  validate(not tarch::la::equals(vector4, 2));
  validate(tarch::la::oneEquals(vector4, 2));
  validate(tarch::la::oneGreater(vector4, 2));
  validate(tarch::la::oneGreaterEquals(vector4, 4));
  validate(not tarch::la::oneSmaller(vector4, 2));
  validate(tarch::la::oneSmallerEquals(vector4, 2));
  validate(not tarch::la::allGreater(vector4, 2));
  validate(tarch::la::allGreaterEquals(vector4, 2));
  validate(not tarch::la::allSmaller(vector4, 4));
  validate(tarch::la::allSmallerEquals(vector4, 4));

  const tarch::la::Vector<3, double> tolerant = {1.0 + 1e-15, 1.0 - 1e-15, 1.0};
  validate(tarch::la::equals(tolerant, 1.0, 1e-14));
  validate(tarch::la::oneEquals(tolerant, 1.0, 1e-14));
  validate(tarch::la::allGreaterEquals(tolerant, 1.0, 1e-14));
  validate(tarch::la::allSmallerEquals(tolerant, 1.0, 1e-14));

  tarch::la::Vector<5, int> generic5 = {1, 2, 3, 4, 5};
  generic5 += 2;
  validateEquals(generic5[0], 3);
  validateEquals(generic5[4], 7);
  generic5 -= 1;
  validateEquals(generic5[0], 2);
  validateEquals(generic5[4], 6);
  generic5 *= 3;
  validateEquals(generic5[0], 6);
  validateEquals(generic5[4], 18);
  generic5 /= 3;
  validateEquals(generic5[0], 2);
  validateEquals(generic5[4], 6);
  generic5 = generic5 + 1;
  validateEquals(generic5[0], 3);
  validateEquals(generic5[4], 7);
  generic5 = generic5 - 2;
  validateEquals(generic5[0], 1);
  validateEquals(generic5[4], 5);
  generic5 = 2 * generic5;
  validateEquals(generic5[0], 2);
  validateEquals(generic5[4], 10);
  generic5 = generic5 / 2;
  validateEquals(generic5[0], 1);
  validateEquals(generic5[4], 5);

  const tarch::la::Vector<5, int> generic5Compare = {2, 2, 3, 4, 5};
  validate(not tarch::la::equals(generic5Compare, 2));
  validate(tarch::la::oneEquals(generic5Compare, 2));
  validate(tarch::la::oneGreater(generic5Compare, 4));
  validate(tarch::la::oneGreaterEquals(generic5Compare, 5));
  validate(not tarch::la::oneSmaller(generic5Compare, 2));
  validate(tarch::la::oneSmallerEquals(generic5Compare, 2));
  validate(not tarch::la::allGreater(generic5Compare, 2));
  validate(tarch::la::allGreaterEquals(generic5Compare, 2));
  validate(not tarch::la::allSmaller(generic5Compare, 5));
  validate(tarch::la::allSmallerEquals(generic5Compare, 5));

  const tarch::la::Vector<7, double> generic7Compare = {1.0, 1.0 + 1e-15, 1.0 - 1e-15, 1.0, 1.0, 1.0, 1.0};
  validate(tarch::la::equals(generic7Compare, 1.0, 1e-14));
  validate(tarch::la::oneEquals(generic7Compare, 1.0, 1e-14));
  validate(tarch::la::allGreaterEquals(generic7Compare, 1.0, 1e-14));
  validate(tarch::la::allSmallerEquals(generic7Compare, 1.0, 1e-14));
}

void
tarch::la::tests::VectorTest::testVectorVectorOperations() {
  tarch::la::Vector<3, int> vector(1);

  vector += vector;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 2);

  vector -= vector;
  validateEquals(vector[0], 0);
  validateEquals(vector[1], 0);
  validateEquals(vector[2], 0);

  vector = {1, 2, 3};
  vector = vector + vector;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 4);
  validateEquals(vector[2], 6);

  vector                                 = {1, 2, 3};
  const tarch::la::Vector<3, int> copied = vector;
  vector                                 = vector - copied;
  validateEquals(vector[0], 0);
  validateEquals(vector[1], 0);
  validateEquals(vector[2], 0);

  vector        = {2, 3, 4};
  int dotResult = tarch::la::dot(vector, vector);
  validateEquals(dotResult, 29);
  dotResult = vector * vector;
  validateEquals(dotResult, 29);

  const tarch::la::Vector<3, int> b           = {2, 3, 4};
  const tarch::la::Vector<3, int> a           = {1, 2, 3};
  const tarch::la::Vector<3, int> crossResult = tarch::la::cross(a, b);
  validateEquals(crossResult[0], a[1] * b[2] - a[2] * b[1]);
  validateEquals(crossResult[1], a[2] * b[0] - a[0] * b[2]);
  validateEquals(crossResult[2], a[0] * b[1] - a[1] * b[0]);

  tarch::la::Vector<3, double> first = {1.0, 2.0, 3.0};
  tarch::la::Vector<3, double> second(first);
  validate(tarch::la::equals(first, second));
  validate(not tarch::la::oneGreater(first, second));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(not tarch::la::allGreater(first, second));
  validate(not tarch::la::firstGreater(first, second));

  first = {2.0, 2.0, 3.0};
  validate(not tarch::la::equals(first, second));
  validate(tarch::la::oneGreater(first, second));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(not tarch::la::allGreater(first, second));
  validate(tarch::la::firstGreater(first, second));

  first = {2.0, 3.0, 4.0};
  validate(not tarch::la::equals(first, second));
  validate(tarch::la::oneGreater(first, second));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(tarch::la::allGreater(first, second));

  constexpr double Tolerance = 1e-14;
  first(0)                   = second(0);
  first(1)                   = second(1);
  first(2)                   = second(2) + 0.99 * Tolerance;
  validateWithParams3(tarch::la::equals(first, second, Tolerance), first, second, first(2) - second(2));
  validate(not tarch::la::oneGreater(first, second, Tolerance));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(not tarch::la::allGreater(first, second, Tolerance));
  validateWithParams2(not tarch::la::firstGreater(first, second, Tolerance), first, second);

  first(2) = second(2) + 10.0 * Tolerance;
  validate(not tarch::la::equals(first, second, Tolerance));
  validate(tarch::la::oneGreater(first, second, Tolerance));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(not tarch::la::allGreater(first, second, Tolerance));
  validateWithParams3(tarch::la::firstGreater(first, second, Tolerance), first, second, first(2) - second(2));

  first = {1.0, 2.0, 3.0};
  first += 10.0 * Tolerance;
  validate(not tarch::la::equals(first, second, Tolerance));
  validate(tarch::la::oneGreater(first, second, Tolerance));
  validate(tarch::la::oneGreaterEquals(first, second));
  validate(tarch::la::allGreater(first, second, Tolerance));
  validate(tarch::la::firstGreater(first, second, Tolerance));

  first = {1.0, 2.0, 3.0};
  first -= 0.99 * Tolerance;
  validate(tarch::la::oneGreaterEquals(first, second));

  first = {1.0, 1.0, 4.0};
  validate(not tarch::la::firstGreater(first, second));
  first = {2.0, 0.0, 0.0};
  validate(tarch::la::firstGreater(first, second));
  first = {1.0, 2.0 + 10.0 * Tolerance, 2.0};
  validate(tarch::la::firstGreater(first, second, Tolerance));

  first           = {1.0, 2.0, 3.0 + 0.99 * Tolerance};
  second          = {1.0, 2.0, 3.0};
  const int index = tarch::la::equalsReturnIndex(first, second, Tolerance);
  validateEquals(index, -1);
}

void
tarch::la::tests::VectorTest::testSpecialisedVectorLayouts() {
  static_assert(std::is_trivially_copyable_v<tarch::la::Vector<1, int>>);
  static_assert(std::is_trivially_copyable_v<tarch::la::Vector<2, int>>);
  static_assert(std::is_trivially_copyable_v<tarch::la::Vector<1, double>>);
  static_assert(std::is_trivially_copyable_v<tarch::la::Vector<2, double>>);
  static_assert(sizeof(tarch::la::Vector<2, int>) == 8);
  static_assert(alignof(tarch::la::Vector<2, int>) == 8);
  static_assert(std::is_standard_layout_v<tarch::la::Vector<2, int>>);
  static_assert(sizeof(tarch::la::Vector<2, double>) == 16);
  static_assert(sizeof(tarch::la::Vector<3, int>) == 12);
  static_assert(alignof(tarch::la::Vector<3, int>) == alignof(int));
  static_assert(std::is_standard_layout_v<tarch::la::Vector<3, int>>);
  static_assert(sizeof(tarch::la::Vector<3, double>) == 24);
  static_assert(sizeof(tarch::la::Vector<4, int>) == 16);
  static_assert(alignof(tarch::la::Vector<4, int>) == 16);
  static_assert(std::is_standard_layout_v<tarch::la::Vector<4, int>>);
  static_assert(sizeof(tarch::la::Vector<4, double>) == 32);
  static_assert(std::is_trivially_copyable_v<tarch::la::Vector<3, double>>);
  static_assert(sizeof(tarch::la::VectorCompare<2>) == sizeof(double));

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
  static_assert(sycl::is_device_copyable_v<tarch::la::Vector<1, int>>);
  static_assert(sycl::is_device_copyable_v<tarch::la::Vector<2, int>>);
  static_assert(sycl::is_device_copyable_v<tarch::la::Vector<1, double>>);
  static_assert(sycl::is_device_copyable_v<tarch::la::Vector<2, double>>);
#endif

  const tarch::la::Vector<2, double> vector2 = {1.5, 2.5};
  validateEquals(vector2.getSize(), 2);
  validateEquals(vector2.getData()[0], 1.5);
  validateEquals(vector2.getData()[1], 2.5);

  const tarch::la::Vector<3, int> vector3 = {4, 5, 6};
  validateEquals(vector3.getSize(), 3);
  validateEquals(vector3.getData()[0], 4);
  validateEquals(vector3.getData()[1], 5);
  validateEquals(vector3.getData()[2], 6);
  validateEquals(vector3[0], 4);
  validateEquals(vector3[1], 5);
  validateEquals(vector3[2], 6);

  const tarch::la::Vector<4, int> vector4(7);
  validateEquals(vector4.getSize(), 4);
  validateEquals(vector4.getData()[0], 7);
  validateEquals(vector4.getData()[1], 7);
  validateEquals(vector4.getData()[2], 7);
  validateEquals(vector4.getData()[3], 7);
}

void
tarch::la::tests::VectorTest::testAdditionalVectorOperations() {
  const tarch::la::Vector<3, double> vector = {-1.2, 2.0, 3.0};
  validateNumericalEquals(tarch::la::norm2Squared(vector), 14.44);
  validateNumericalEquals(tarch::la::norm2(vector), std::sqrt(14.44));
  validateNumericalEquals(tarch::la::normMax(vector), 3.0);
  validateNumericalEquals(tarch::la::volume(vector), -7.2);
  validateNumericalEquals(tarch::la::average(vector), 1.2666666666666666);
  validateNumericalEquals(tarch::la::maxAbs(vector), 3.0);

  const auto absolute = tarch::la::abs(vector);
  validateNumericalEquals(absolute[0], 1.2);
  validateNumericalEquals(absolute[1], 2.0);
  validateNumericalEquals(absolute[2], 3.0);

  const tarch::la::Vector<3, double> floored = tarch::la::floor(vector);
  validateEquals(floored[0], -2.0);
  validateEquals(floored[1], 2.0);
  validateEquals(floored[2], 3.0);

  const tarch::la::Vector<3, double> inverted = tarch::la::invertEntries(tarch::la::Vector<3, double>{2.0, 4.0, 0.5});
  validateNumericalEquals(inverted[0], 0.5);
  validateNumericalEquals(inverted[1], 0.25);
  validateNumericalEquals(inverted[2], 2.0);

  validate(tarch::la::contains(tarch::la::Vector<3, int>{1, 2, 3}, 2));
  validateEquals(tarch::la::count(tarch::la::Vector<4, int>{1, 2, 2, 3}, 2), 2);
  validate(tarch::la::allEntriesAreTheSame(tarch::la::Vector<3, int>{5, 5, 5}));
  validate(not tarch::la::allEntriesAreTheSame(tarch::la::Vector<3, int>{5, 6, 5}));
  validateEquals(tarch::la::isEntryFinite(tarch::la::Vector<3, double>{1.0, 2.0, 3.0}), 1);
  validateEquals(
    tarch::la::isEntryNan(tarch::la::Vector<3, double>{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}),
    1
  );

  const tarch::la::Vector<3, int> ints      = {1, 2, 3};
  const auto                      converted = tarch::la::convertScalar<double>(ints);
  validateEquals(converted[0], 1.0);
  validateEquals(converted[1], 2.0);
  validateEquals(converted[2], 3.0);

  const tarch::la::Vector<3, double> comparedVector = {1.0, 2.0, 3.0};
  validate(tarch::la::equals(tarch::la::Vector<3, int>{3, 3, 3}, 3));
  validate(tarch::la::oneEquals(tarch::la::Vector<3, int>{1, 2, 3}, 2));
  validate(tarch::la::oneGreater(tarch::la::Vector<3, int>{1, 4, 3}, 3));
  validate(tarch::la::oneGreaterEquals(tarch::la::Vector<3, int>{1, 3, 2}, 3));
  validate(tarch::la::oneSmaller(tarch::la::Vector<3, int>{1, 4, 3}, 2));
  validate(tarch::la::oneSmallerEquals(tarch::la::Vector<3, int>{1, 4, 3}, 1));
  validate(tarch::la::allGreater(tarch::la::Vector<3, int>{4, 5, 6}, 3));
  validate(tarch::la::allGreaterEquals(tarch::la::Vector<3, int>{3, 4, 5}, 3));
  validate(tarch::la::allSmaller(tarch::la::Vector<3, int>{1, 2, 3}, 4));
  validate(tarch::la::allSmallerEquals(tarch::la::Vector<3, int>{1, 2, 3}, 3));

  validate(tarch::la::equals(comparedVector, 1.0, 2.1));
  validate(tarch::la::oneEquals(comparedVector, 2.0));
  validate(tarch::la::oneGreater(comparedVector, 2.5));
  validate(tarch::la::oneGreaterEquals(comparedVector, 3.0));
  validate(tarch::la::oneSmaller(comparedVector, 1.5));
  validate(tarch::la::oneSmallerEquals(comparedVector, 1.1));
  validate(tarch::la::allGreater(tarch::la::Vector<3, double>{2.0, 3.0, 4.0}, 1.5));
  validate(tarch::la::allGreaterEquals(tarch::la::Vector<3, double>{2.0, 3.0, 4.0}, 2.0));
  validate(tarch::la::allSmaller(tarch::la::Vector<3, double>{1.0, 2.0, 3.0}, 4.0));
  validate(tarch::la::allSmallerEquals(tarch::la::Vector<3, double>{1.0, 2.0, 3.0}, 3.0));

  const tarch::la::Vector<3, int> left          = {1, 4, 2};
  const tarch::la::Vector<3, int> right         = {3, 1, 2};
  const auto                      maxVector     = tarch::la::max(left, right);
  const auto                      minVector     = tarch::la::min(left, right);
  const auto                      productVector = tarch::la::multiplyComponents(left, right);
  const auto                      quotientVector =
    tarch::la::divideComponents(tarch::la::Vector<3, int>{8, 9, 10}, tarch::la::Vector<3, int>{2, 3, 5});
  validateEquals(maxVector[0], 3);
  validateEquals(maxVector[1], 4);
  validateEquals(minVector[0], 1);
  validateEquals(minVector[1], 1);
  validateEquals(productVector[0], 3);
  validateEquals(productVector[1], 4);
  validateEquals(quotientVector[0], 4);
  validateEquals(quotientVector[1], 3);
  validate(left != right);
  validateEquals(tarch::la::equalsReturnIndex(left, right), 0);

  const tarch::la::Vector<3, double> remainderVector =
    tarch::la::remainder(tarch::la::Vector<3, double>{5.5, -0.5, 10.0}, 2.0);
  validateNumericalEquals(remainderVector[0], 1.5);
  validateNumericalEquals(remainderVector[1], 1.5);
  validateNumericalEquals(remainderVector[2], 0.0);
}

void
tarch::la::tests::VectorTest::testSpecialisedVectorMembers() {
  const int                       raw2[] = {7, 8};
  const tarch::la::Vector<2, int> vector2(raw2);
  validateEquals(vector2.getSize(), 2);
  validateEquals(vector2.getData()[0], 7);
  validateEquals(vector2(1), 8);

  const tarch::la::Vector<2, int> bits2(std::bitset<2>(std::string("10")));
  validateEquals(bits2[0], 0);
  validateEquals(bits2[1], 1);

  const double                       raw3[] = {1.5, 2.5, 3.5};
  const tarch::la::Vector<3, double> vector3(raw3);
  validateNumericalEquals(vector3.getData()[2], 3.5);
  validateNumericalEquals(vector3[1], 2.5);

  const tarch::la::Vector<3, int> bits3(std::bitset<3>(std::string("101")));
  validateEquals(bits3[0], 1);
  validateEquals(bits3[1], 0);
  validateEquals(bits3[2], 1);

  const int                 raw4[] = {1, 2, 3, 4};
  tarch::la::Vector<4, int> vector4(raw4);
  validateEquals(vector4.getSize(), 4);
  validateEquals(vector4(3), 4);
  vector4[0] = 9;
  validateEquals(vector4.getData()[0], 9);

  const tarch::la::Vector<4, int> bits4(std::bitset<4>(std::string("1101")));
  validateEquals(bits4[0], 1);
  validateEquals(bits4[1], 0);
  validateEquals(bits4[2], 1);
  validateEquals(bits4[3], 1);
}

void
tarch::la::tests::VectorTest::testVectorCompare() {
  const tarch::la::VectorCompare<2> compare(1.0e-12);

  const tarch::la::Vector<2, double> left  = {1.0, 2.0};
  const tarch::la::Vector<2, double> same  = {1.0 + 1.0e-14, 2.0};
  const tarch::la::Vector<2, double> right = {1.0, 3.0};

  validate(not compare(left, same));
  validate(not compare(same, left));
  validate(compare(left, right));
  validate(not compare(right, left));
}

/**
 * A NaN component makes every tolerance comparison of vectors report a difference, whatever the tolerance.
 */
void
tarch::la::tests::VectorTest::testNaNComparesUnequal() {
#if not defined(PEANO_WITH_FPE)
  // A build with floating-point traps aborts at an ordered comparison with a NaN.
  constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();
  constexpr double Tolerance  = std::numeric_limits<double>::max();

  const tarch::la::Vector<3, double> finite  = {1.0, 2.0, 3.0};
  const tarch::la::Vector<3, double> withNaN = {1.0, NotANumber, 3.0};
  validate(tarch::la::equals(finite, finite, Tolerance));
  validate(not tarch::la::equals(finite, withNaN, Tolerance));
  validate(not tarch::la::equals(withNaN, finite, Tolerance));
  validate(not tarch::la::equals(withNaN, 1.0, Tolerance));
  validateEquals(tarch::la::equalsReturnIndex(finite, withNaN, Tolerance), 1);

  const tarch::la::Vector<5, double> generic = {1.0, 1.0, 1.0, NotANumber, 1.0};
  validate(not tarch::la::equals(generic, 1.0, Tolerance));
  validate(not tarch::la::equals(generic, generic, Tolerance));
  validateEquals(tarch::la::equalsReturnIndex(generic, generic, Tolerance), 3);
#endif
}

/**
 * oneSmaller() and oneSmallerEquals() treat a component within the tolerance of the comparison
 * value as equal to it.
 */
void
tarch::la::tests::VectorTest::testSmallerComparisonsHonourTheTolerance() {
  constexpr double Tolerance   = 1.0e-10;
  constexpr double InsideAbove = 1.0 + 0.5 * Tolerance;
  constexpr double InsideBelow = 1.0 - 0.5 * Tolerance;
  constexpr double BeyondAbove = 1.0 + 2.0 * Tolerance;
  constexpr double BeyondBelow = 1.0 - 2.0 * Tolerance;

  const tarch::la::Vector<2, double> vector2 = {1.0, 2.0};
  const tarch::la::Vector<3, double> vector3 = {1.0, 2.0, 3.0};
  const tarch::la::Vector<4, double> vector4 = {1.0, 2.0, 3.0, 4.0};
  const tarch::la::Vector<5, double> vector5 = {1.0, 2.0, 3.0, 4.0, 5.0};

  validate(not tarch::la::oneSmaller(vector2, InsideAbove, Tolerance));
  validate(not tarch::la::oneSmaller(vector3, InsideAbove, Tolerance));
  validate(not tarch::la::oneSmaller(vector4, InsideAbove, Tolerance));
  validate(not tarch::la::oneSmaller(vector5, InsideAbove, Tolerance));
  validate(tarch::la::oneSmaller(vector2, BeyondAbove, Tolerance));
  validate(tarch::la::oneSmaller(vector3, BeyondAbove, Tolerance));
  validate(tarch::la::oneSmaller(vector4, BeyondAbove, Tolerance));
  validate(tarch::la::oneSmaller(vector5, BeyondAbove, Tolerance));

  validate(tarch::la::oneSmallerEquals(vector2, InsideBelow, Tolerance));
  validate(tarch::la::oneSmallerEquals(vector3, InsideBelow, Tolerance));
  validate(tarch::la::oneSmallerEquals(vector4, InsideBelow, Tolerance));
  validate(tarch::la::oneSmallerEquals(vector5, InsideBelow, Tolerance));
  validate(not tarch::la::oneSmallerEquals(vector2, BeyondBelow, Tolerance));
  validate(not tarch::la::oneSmallerEquals(vector3, BeyondBelow, Tolerance));
  validate(not tarch::la::oneSmallerEquals(vector4, BeyondBelow, Tolerance));
  validate(not tarch::la::oneSmallerEquals(vector5, BeyondBelow, Tolerance));
}

/**
 * Swapping the first two components of a vector keeps its squared norm bitwise.
 */
void
tarch::la::tests::VectorTest::testNorm2SquaredHoldsASwapOfTheFirstTwoComponents() {
  for (int step = 0; step < 2000; step++) {
    tarch::la::Vector<2, double> plane;
    plane(0) = -0.298 + 0.001 * step;
    plane(1) = -0.29676 + 0.000617 * step;
    tarch::la::Vector<2, double> swappedPlane;
    swappedPlane(0) = plane(1);
    swappedPlane(1) = plane(0);
    validateEqualsWithParams1(tarch::la::norm2Squared(plane), tarch::la::norm2Squared(swappedPlane), step);

    tarch::la::Vector<3, double> space;
    space(0) = plane(0);
    space(1) = plane(1);
    space(2) = 0.173 + 0.000431 * step;
    tarch::la::Vector<3, double> swappedSpace;
    swappedSpace(0) = space(1);
    swappedSpace(1) = space(0);
    swappedSpace(2) = space(2);
    validateEqualsWithParams1(tarch::la::norm2Squared(space), tarch::la::norm2Squared(swappedSpace), step);
  }
}
