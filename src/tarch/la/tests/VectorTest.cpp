// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/tests/VectorTest.h"

#include "tarch/la/Vector.h"
#include "tarch/la/VectorCompare.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/la/VectorScalarOperations.h"
#include "tarch/la/VectorVectorOperations.h"

tarch::la::tests::VectorTest::VectorTest():
  TestCase("tarch::la::tests::VectorTest") {}

void tarch::la::tests::VectorTest::run() {
  testMethod(testConstruction);
  testMethod(testAssignment);
  testMethod(testVectorOperations);
  testMethod(testVectorScalarOperations);
  testMethod(testVectorVectorOperations);
  testMethod(testVectorCompare);

  testMethod(testVectorVectorCompare);
  testMethod(testSpecialisedVectorLayouts);
  testMethod(testAdditionalVectorOperations);
  testMethod(testSpecialisedVectorMembers);
}

void tarch::la::tests::VectorTest::testVectorVectorCompare() {
  std::map<tarch::la::Vector<2, double>, int, tarch::la::VectorCompare<2>> map;

  tarch::la::Vector<2, double> a = {0.0, 0.0};
  tarch::la::Vector<2, double> b = {0.0, 0.3};
  tarch::la::Vector<2, double> c = {1.0, 0.0};

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

void tarch::la::tests::VectorTest::testConstruction() {
  // Test construction with uniform initialization
  Vector<1, int> vector1(1);
  validateEquals(vector1[0], 1);

  Vector<2, int> vector2(1);
  validateEquals(vector2[0], 1);
  validateEquals(vector2[1], 1);

  // Test construction with direct individual initialization
  Vector<2, int> vector3 = {1, 2};
  validateEquals(vector3[0], 1);
  validateEquals(vector3[1], 2);

  Vector<3, int> vector4 = {1, 2, 3};
  validateEquals(vector4[0], 1);
  validateEquals(vector4[1], 2);
  validateEquals(vector4[2], 3);

  // Test copy construction
  Vector<3, int> vector5(vector4);
  validateEquals(vector5[0], 1);
  validateEquals(vector5[1], 2);
  validateEquals(vector5[2], 3);

  Vector<3, int> vector6 = vector5;
  validateEquals(vector6[0], 1);
  validateEquals(vector6[1], 2);
  validateEquals(vector6[2], 3);
}

void tarch::la::tests::VectorTest::testAssignment() {
  Vector<3, int> vector = {1, 2, 3};
  validateEquals(vector[0], 1);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 3);

  vector = {1, 2, 3};
  validateEqualsWithParams1(vector[0], 1, ::toString(vector));
  validateEqualsWithParams1(vector[1], 2, ::toString(vector));
  validateEqualsWithParams1(vector[2], 3, ::toString(vector));
}

void tarch::la::tests::VectorTest::testVectorOperations() {
  Vector<3, int> vector = {1, -2, 2};

  int result = sum(vector);
  validateEquals(result, 1);

  result = norm1(vector);
  validateEquals(result, 5.0);

  // Test indexMin, indexMax, min, and max methods
  vector = {1, 2, 3};
  validateEquals(indexMin(vector), 0);
  validateEquals(indexMax(vector), 2);
  validateEquals(min(vector), 1);
  validateEquals(max(vector), 3);

  Vector<3, int>     vector3 = {1, 2, 3};
  std::ostringstream stream;
  stream << vector3;
  std::string vector3string(stream.str());
  validateEquals(vector3string, std::string("[1,2,3]"));

  Vector<2, double> vector2 = {-1.5, 3.0};
  validateNumericalEquals(norm2Squared(vector2), 11.25);
  validateNumericalEquals(normMax(vector2), 3.0);
  validateEquals(count(Vector<2, int>{4, 4}, 4), 2);
  validate(allEntriesAreTheSame(Vector<2, int>{4, 4}));

  Vector<4, int> vector4 = {3, -1, 5, 2};
  validateEquals(sum(vector4), 9);
  validateEquals(volume(Vector<4, int>{2, 3, 4, 5}), 120);
  validateEquals(indexMin(vector4), 1);
  validateEquals(indexMax(vector4), 2);
  validateEquals(min(vector4), -1);
  validateEquals(max(vector4), 5);
  validateEquals(maxAbs(vector4), 5);
  validate(contains(vector4, 2));
  validate(!contains(vector4, 7));
  validate(isEntryFinite(Vector<4, double>{1.0, 2.0, 3.0, 4.0}));

  Vector<4, double> floorResult = floor(Vector<4, double>{1.2, -1.2, 3.99, 4.01});
  validateNumericalEquals(floorResult[0], 1.0);
  validateNumericalEquals(floorResult[1], -2.0);
  validateNumericalEquals(floorResult[2], 3.0);
  validateNumericalEquals(floorResult[3], 4.0);

  Vector<3, double> inverted = invertEntries(Vector<3, double>{2.0, 4.0, 8.0});
  validateNumericalEquals(inverted[0], 0.5);
  validateNumericalEquals(inverted[1], 0.25);
  validateNumericalEquals(inverted[2], 0.125);

  Vector<5, int> generic5 = {1, -2, 3, -4, 5};
  validateEquals(sum(generic5), 3);
  validateEquals(norm1(generic5), 15);
  validateEquals(norm2Squared(generic5), 55);
  validateEquals(normMax(generic5), 5);
  validateEquals(indexMin(generic5), 3);
  validateEquals(indexMax(generic5), 4);
  validateEquals(min(generic5), -4);
  validateEquals(max(generic5), 5);
  validateEquals(maxAbs(generic5), 5);
  validate(contains(generic5, 3));
  validate(!contains(generic5, 9));
  validateEquals(count(Vector<5, int>{1, 2, 1, 3, 1}, 1), 3);

  Vector<7, double> generic7Floor = floor(Vector<7, double>{1.2, 2.9, -1.2, -3.0, 0.0, 4.01, 5.99});
  validateNumericalEquals(generic7Floor[0], 1.0);
  validateNumericalEquals(generic7Floor[1], 2.0);
  validateNumericalEquals(generic7Floor[2], -2.0);
  validateNumericalEquals(generic7Floor[3], -3.0);
  validateNumericalEquals(generic7Floor[4], 0.0);
  validateNumericalEquals(generic7Floor[5], 4.0);
  validateNumericalEquals(generic7Floor[6], 5.0);

  Vector<5, double> generic5Inv = invertEntries(Vector<5, double>{2.0, 4.0, 5.0, 8.0, 10.0});
  validateNumericalEquals(generic5Inv[0], 0.5);
  validateNumericalEquals(generic5Inv[1], 0.25);
  validateNumericalEquals(generic5Inv[2], 0.2);
  validateNumericalEquals(generic5Inv[3], 0.125);
  validateNumericalEquals(generic5Inv[4], 0.1);
}

void tarch::la::tests::VectorTest::testVectorScalarOperations() {
  Vector<3, int> vector(0);

  // Compound assignment operators
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

  // Arithmetic operators
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

  Vector<2, double> vector2 = {1.0, 2.0};
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

  Vector<4, int> vector4 = {2, 2, 3, 4};
  validate(!equals(vector4, 2));
  validate(oneEquals(vector4, 2));
  validate(oneGreater(vector4, 2));
  validate(oneGreaterEquals(vector4, 4));
  validate(!oneSmaller(vector4, 2));
  validate(oneSmallerEquals(vector4, 2));
  validate(!allGreater(vector4, 2));
  validate(allGreaterEquals(vector4, 2));
  validate(!allSmaller(vector4, 4));
  validate(allSmallerEquals(vector4, 4));

  Vector<3, double> tolerant = {1.0 + 1e-15, 1.0 - 1e-15, 1.0};
  validate(equals(tolerant, 1.0, 1e-14));
  validate(oneEquals(tolerant, 1.0, 1e-14));
  validate(allGreaterEquals(tolerant, 1.0, 1e-14));
  validate(allSmallerEquals(tolerant, 1.0, 1e-14));

  Vector<5, int> generic5 = {1, 2, 3, 4, 5};
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

  Vector<5, int> generic5Compare = {2, 2, 3, 4, 5};
  validate(!equals(generic5Compare, 2));
  validate(oneEquals(generic5Compare, 2));
  validate(oneGreater(generic5Compare, 4));
  validate(oneGreaterEquals(generic5Compare, 5));
  validate(!oneSmaller(generic5Compare, 2));
  validate(oneSmallerEquals(generic5Compare, 2));
  validate(!allGreater(generic5Compare, 2));
  validate(allGreaterEquals(generic5Compare, 2));
  validate(!allSmaller(generic5Compare, 5));
  validate(allSmallerEquals(generic5Compare, 5));

  Vector<7, double> generic7Compare = {1.0, 1.0 + 1e-15, 1.0 - 1e-15, 1.0, 1.0, 1.0, 1.0};
  validate(equals(generic7Compare, 1.0, 1e-14));
  validate(oneEquals(generic7Compare, 1.0, 1e-14));
  validate(allGreaterEquals(generic7Compare, 1.0, 1e-14));
  validate(allSmallerEquals(generic7Compare, 1.0, 1e-14));
}

void tarch::la::tests::VectorTest::testVectorVectorOperations() {
  Vector<3, int> vector(1);

  // Addition assignment
  vector += vector;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 2);
  validateEquals(vector[2], 2);

  // Subtraction assignment
  vector -= vector;
  validateEquals(vector[0], 0);
  validateEquals(vector[1], 0);
  validateEquals(vector[2], 0);

  // Addition
  vector = {1, 2, 3};
  vector = vector + vector;
  validateEquals(vector[0], 2);
  validateEquals(vector[1], 4);
  validateEquals(vector[2], 6);

  // Subtraction
  vector = {1, 2, 3};
  vector = vector - vector;
  validateEquals(vector[0], 0);
  validateEquals(vector[1], 0);
  validateEquals(vector[2], 0);

  // Dot product
  vector        = {2, 3, 4};
  int dotresult = dot(vector, vector);
  validateEquals(dotresult, 29);
  validateEquals(dotresult, 29);
  dotresult = vector * vector;
  validateEquals(dotresult, 29);

  // Cross product (only works for same types)
  Vector<3, int> b = {2, 3, 4};
  Vector<3, int> a = {1, 2, 3};
  Vector<3, int> crossresult(0);
  crossresult = cross(a, b);
  validateEquals(crossresult[0], a[1] * b[2] - a[2] * b[1]);
  validateEquals(crossresult[1], a[2] * b[0] - a[0] * b[2]);
  validateEquals(crossresult[2], a[0] * b[1] - a[1] * b[0]);

  // Comparisons
  Vector<3, double> vec0 = {1.0, 2.0, 3.0};
  Vector<3, double> vec1(vec0);
  validate(equals(vec0, vec1));
  validate(!oneGreater(vec0, vec1));
  validate(oneGreaterEquals(vec0, vec1));
  validate(!allGreater(vec0, vec1));
  validate(!firstGreater(vec0, vec1));

  vec0 = {2.0, 2.0, 3.0};
  validate(!equals(vec0, vec1));
  validate(oneGreater(vec0, vec1));
  validate(oneGreaterEquals(vec0, vec1));
  validate(!allGreater(vec0, vec1));
  validate(firstGreater(vec0, vec1));

  vec0 = {2.0, 3.0, 4.0};
  validate(!equals(vec0, vec1));
  validate(oneGreater(vec0, vec1));
  validate(oneGreaterEquals(vec0, vec1));
  validate(allGreater(vec0, vec1));

  // up to here vec1=vec0
  const double tolerance = 1e-14;
  vec0(0)                = vec1(0);
  vec0(1)                = vec1(1);
  vec0(2)                = vec1(2) + 0.99 * tolerance;
  validateWithParams3(equals(vec0, vec1, tolerance), vec0, vec1, vec0(2) - vec1(2));
  validate(!oneGreater(vec0, vec1, tolerance));
  validate(oneGreaterEquals(vec0, vec1));
  validate(!allGreater(vec0, vec1, tolerance));
  validateWithParams2(!firstGreater(vec0, vec1, tolerance), vec0, vec1);

  vec0(2) = vec1(2) + 10.0 * tolerance;
  validate(!equals(vec0, vec1, tolerance));
  validate(oneGreater(vec0, vec1, tolerance));
  validate(oneGreaterEquals(vec0, vec1));
  validate(!allGreater(vec0, vec1, tolerance));
  validateWithParams3(firstGreater(vec0, vec1, tolerance), vec0, vec1, vec0(2) - vec1(2));

  vec0 = {1.0, 2.0, 3.0};
  vec0 += 10.0 * tolerance;
  validate(!equals(vec0, vec1, tolerance));
  validate(oneGreater(vec0, vec1, tolerance));
  validate(oneGreaterEquals(vec0, vec1));
  validate(allGreater(vec0, vec1, tolerance));
  validate(firstGreater(vec0, vec1, tolerance));

  vec0 = {1.0, 2.0, 3.0};
  vec0 -= 0.99 * tolerance;
  validate(oneGreaterEquals(vec0, vec1));

  vec0 = {1.0, 1.0, 4.0};
  validate(!firstGreater(vec0, vec1));
  vec0 = {2.0, 0.0, 0.0};
  validate(firstGreater(vec0, vec1));
  vec0 = {1.0, 2.0 + 10.0 * tolerance, 2.0};
  validate(firstGreater(vec0, vec1, tolerance));

  // Test equalsReturnIndex
  vec0  = {1.0, 2.0, 3.0 + 0.99 * tolerance};
  vec1  = {1.0, 2.0, 3.0};
  int i = equalsReturnIndex(vec0, vec1, tolerance);
  validateEquals(i, -1);
}

void tarch::la::tests::VectorTest::testSpecialisedVectorLayouts() {
  static_assert(sizeof(Vector<2, int>) == 8);
  static_assert(alignof(Vector<2, int>) == 8);
  static_assert(sizeof(Vector<3, int>) == 12);
  static_assert(alignof(Vector<3, int>) == alignof(int));
  static_assert(sizeof(Vector<4, int>) == 16);
  static_assert(alignof(Vector<4, int>) == 16);
  static_assert(std::is_trivially_copyable<Vector<3, double>>::value);

  Vector<2, double> vector2 = {1.5, 2.5};
  validateEquals(vector2.size(), 2);
  validateEquals(vector2.data()[0], 1.5);
  validateEquals(vector2.data()[1], 2.5);

  Vector<3, int> vector3 = {4, 5, 6};
  validateEquals(vector3.size(), 3);
  validateEquals(vector3.data()[0], 4);
  validateEquals(vector3.data()[1], 5);
  validateEquals(vector3.data()[2], 6);
  validateEquals(vector3[0], 4);
  validateEquals(vector3[1], 5);
  validateEquals(vector3[2], 6);

  Vector<4, int> vector4(7);
  validateEquals(vector4.size(), 4);
  validateEquals(vector4.data()[0], 7);
  validateEquals(vector4.data()[1], 7);
  validateEquals(vector4.data()[2], 7);
  validateEquals(vector4.data()[3], 7);
}

void tarch::la::tests::VectorTest::testAdditionalVectorOperations() {
  Vector<3, double> vector = {-1.2, 2.0, 3.0};
  validateNumericalEquals(norm2Squared(vector), 14.44);
  validateNumericalEquals(norm2(vector), std::sqrt(14.44));
  validateNumericalEquals(normMax(vector), 3.0);
  validateNumericalEquals(volume(vector), -7.2);
  validateNumericalEquals(average(vector), 1.2666666666666666);
  validateNumericalEquals(maxAbs(vector), 3.0);

  auto absolute = abs(vector);
  validateNumericalEquals(absolute[0], 1.2);
  validateNumericalEquals(absolute[1], 2.0);
  validateNumericalEquals(absolute[2], 3.0);

  Vector<3, double> floored = floor(vector);
  validateEquals(floored[0], -2.0);
  validateEquals(floored[1], 2.0);
  validateEquals(floored[2], 3.0);

  Vector<3, double> inverted = invertEntries(Vector<3, double>{2.0, 4.0, 0.5});
  validateNumericalEquals(inverted[0], 0.5);
  validateNumericalEquals(inverted[1], 0.25);
  validateNumericalEquals(inverted[2], 2.0);

  validate(contains(Vector<3, int>{1, 2, 3}, 2));
  validateEquals(count(Vector<4, int>{1, 2, 2, 3}, 2), 2);
  validate(allEntriesAreTheSame(Vector<3, int>{5, 5, 5}));
  validate(!allEntriesAreTheSame(Vector<3, int>{5, 6, 5}));
  validateEquals(isEntryFinite(Vector<3, double>{1.0, 2.0, 3.0}), 1);
  validateEquals(isEntryNan(Vector<3, double>{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}), 1);

  std::vector<int> stlVector = toSTLVector(Vector<3, int>{4, 5, 6});
  validateEquals(static_cast<int>(stlVector.size()), 3);
  validateEquals(stlVector[0], 4);
  validateEquals(stlVector[2], 6);

  Vector<3, int> ints      = {1, 2, 3};
  auto           converted = tarch::la::convertScalar<double>(ints);
  validateEquals(converted[0], 1.0);
  validateEquals(converted[1], 2.0);
  validateEquals(converted[2], 3.0);

  Vector<3, double> scalarCmp = {1.0, 2.0, 3.0};
  validate(equals(Vector<3, int>{3, 3, 3}, 3));
  validate(oneEquals(Vector<3, int>{1, 2, 3}, 2));
  validate(oneGreater(Vector<3, int>{1, 4, 3}, 3));
  validate(oneGreaterEquals(Vector<3, int>{1, 3, 2}, 3));
  validate(oneSmaller(Vector<3, int>{1, 4, 3}, 2));
  validate(oneSmallerEquals(Vector<3, int>{1, 4, 3}, 1));
  validate(allGreater(Vector<3, int>{4, 5, 6}, 3));
  validate(allGreaterEquals(Vector<3, int>{3, 4, 5}, 3));
  validate(allSmaller(Vector<3, int>{1, 2, 3}, 4));
  validate(allSmallerEquals(Vector<3, int>{1, 2, 3}, 3));

  validate(equals(scalarCmp, 1.0, 2.1));
  validate(oneEquals(scalarCmp, 2.0));
  validate(oneGreater(scalarCmp, 2.5));
  validate(oneGreaterEquals(scalarCmp, 3.0));
  validate(oneSmaller(scalarCmp, 1.5));
  validate(oneSmallerEquals(scalarCmp, 1.1));
  validate(allGreater(Vector<3, double>{2.0, 3.0, 4.0}, 1.5));
  validate(allGreaterEquals(Vector<3, double>{2.0, 3.0, 4.0}, 2.0));
  validate(allSmaller(Vector<3, double>{1.0, 2.0, 3.0}, 4.0));
  validate(allSmallerEquals(Vector<3, double>{1.0, 2.0, 3.0}, 3.0));

  Vector<3, int> lhs       = {1, 4, 2};
  Vector<3, int> rhs       = {3, 1, 2};
  auto           maxVector = max(lhs, rhs);
  auto           minVector = min(lhs, rhs);
  auto           mulVector = multiplyComponents(lhs, rhs);
  auto           divVector = divideComponents(Vector<3, int>{8, 9, 10}, Vector<3, int>{2, 3, 5});
  validateEquals(maxVector[0], 3);
  validateEquals(maxVector[1], 4);
  validateEquals(minVector[0], 1);
  validateEquals(minVector[1], 1);
  validateEquals(mulVector[0], 3);
  validateEquals(mulVector[1], 4);
  validateEquals(divVector[0], 4);
  validateEquals(divVector[1], 3);
  validate(operator!=(lhs, rhs));
  validateEquals(equalsReturnIndex(lhs, rhs), 0);
  validateEquals(countEqualEntries(Vector<3, int>{1, 2, 3}, Vector<3, int>{1, 4, 3}), 2);
  validateEquals(
    countEqualEntries(Vector<3, double>{1.0, 2.0, 3.0}, Vector<3, double>{1.0, 2.0 + 1e-15, 5.0}, 1e-14),
    2
  );

  Vector<3, double> remainderVector = remainder(Vector<3, double>{5.5, 7.25, 10.0}, 2.0);
  validateNumericalEquals(remainderVector[0], 8.25);
  validateNumericalEquals(remainderVector[1], 9.0625);
  validateNumericalEquals(remainderVector[2], 0.0);
}

void tarch::la::tests::VectorTest::testSpecialisedVectorMembers() {
  const int      raw2[] = {7, 8};
  Vector<2, int> vector2(raw2);
  validateEquals(vector2.size(), 2);
  validateEquals(vector2.data()[0], 7);
  validateEquals(vector2(1), 8);

  Vector<2, int> bits2(std::bitset<2>(std::string("10")));
  validateEquals(bits2[0], 0);
  validateEquals(bits2[1], 1);

  const double      raw3[] = {1.5, 2.5, 3.5};
  Vector<3, double> vector3(raw3);
  validateNumericalEquals(vector3.data()[2], 3.5);
  validateNumericalEquals(vector3[1], 2.5);

  Vector<3, int> bits3(std::bitset<3>(std::string("101")));
  validateEquals(bits3[0], 1);
  validateEquals(bits3[1], 0);
  validateEquals(bits3[2], 1);

  const int      raw4[] = {1, 2, 3, 4};
  Vector<4, int> vector4(raw4);
  validateEquals(vector4.size(), 4);
  validateEquals(vector4(3), 4);
  vector4[0] = 9;
  validateEquals(vector4.data()[0], 9);

  Vector<4, int> bits4(std::bitset<4>(std::string("1101")));
  validateEquals(bits4[0], 1);
  validateEquals(bits4[1], 0);
  validateEquals(bits4[2], 1);
  validateEquals(bits4[3], 1);
}

void tarch::la::tests::VectorTest::testVectorCompare() {
  tarch::la::VectorCompare<2> compare(1.0e-12);

  tarch::la::Vector<2, double> left  = {1.0, 2.0};
  tarch::la::Vector<2, double> same  = {1.0 + 1.0e-14, 2.0};
  tarch::la::Vector<2, double> right = {1.0, 3.0};

  validate(!compare(left, same));
  validate(!compare(same, left));
  validate(compare(left, right));
  validate(!compare(right, left));
}
