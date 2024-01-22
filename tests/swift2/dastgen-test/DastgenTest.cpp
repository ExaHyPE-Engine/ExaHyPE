#include "DastgenTest.h"


void swift2::dastgenTest::checkBooleans(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  // First set some values, where applicable.
  const bool initval_bool = false;
  bool       var_bool;


  // default case booleans
  // ----------------------

  particle.setMyBoolean(initval_bool);
  var_bool = particle.getMyBoolean();
  assert(var_bool == initval_bool);

  var_bool = particle.getMyInitvalBoolean();
  assert(var_bool == true);
  // Test setter here too
  particle.setMyInitvalBoolean(true);
  var_bool = particle.getMyInitvalBoolean();
  assert(var_bool == true);

  particle.setMyStaticBoolean(initval_bool);
  var_bool = particle.getMyStaticBoolean();
  assert(var_bool == initval_bool);

  var_bool = particle.getMyStaticInitvalBoolean();
  assert(var_bool == true);
  // Test setter here too
  particle.setMyStaticInitvalBoolean(particle.getMyStaticInitvalBoolean());
  var_bool = particle.getMyStaticInitvalBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstStaticBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstExprBoolean();
  assert(var_bool == true);


  // booleans + ifdef
  // ----------------------

#if PeanoDebug > 0

  particle.setMyDebugBoolean(initval_bool);
  var_bool = particle.getMyDebugBoolean();
  assert(var_bool == initval_bool);

  var_bool = particle.getMyInitvalDebugBoolean();
  assert(var_bool == true);
  // Test setter here too
  particle.setMyInitvalDebugBoolean(true);
  var_bool = particle.getMyInitvalDebugBoolean();
  assert(var_bool == true);

  particle.setMyStaticDebugBoolean(initval_bool);
  var_bool = particle.getMyStaticDebugBoolean();
  assert(var_bool == initval_bool);

  var_bool = particle.getMyStaticInitvalDebugBoolean();
  assert(var_bool == true);
  // Test setter here too
  particle.setMyStaticInitvalDebugBoolean(
    particle.getMyStaticInitvalDebugBoolean()
  );
  var_bool = particle.getMyStaticInitvalDebugBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstDebugBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstStaticDebugBoolean();
  assert(var_bool == true);

  var_bool = particle.getMyConstExprDebugBoolean();
  assert(var_bool == true);

#endif
}


void swift2::dastgenTest::checkDoubles(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  // First set some values, where applicable.
  const double initval_double = 123.456;
  double       var_double;


  // default case Doubles
  // ----------------------

  particle.setMyDouble(initval_double);
  var_double = particle.getMyDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyInitvalDouble();
  assert(var_double == 4.);
  // Test setter here too
  particle.setMyInitvalDouble(4.);
  var_double = particle.getMyInitvalDouble();
  assert(var_double == 4.);

  particle.setMyStaticDouble(initval_double);
  var_double = particle.getMyStaticDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyStaticInitvalDouble();
  assert(var_double == 8.);
  // Test setter here too
  particle.setMyStaticInitvalDouble(particle.getMyStaticInitvalDouble());
  var_double = particle.getMyStaticInitvalDouble();
  assert(var_double == 8.);

  var_double = particle.getMyConstDouble();
  assert(var_double == 16.);

  var_double = particle.getMyConstStaticDouble();
  assert(var_double == 32.);

  var_double = particle.getMyConstExprDouble();
  assert(var_double == 64.);


  // Doubles + ifdefs
  // ----------------------

#if PeanoDebug > 0

  particle.setMyDebugDouble(initval_double);
  var_double = particle.getMyDebugDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyInitvalDebugDouble();
  assert(var_double == 4.);
  // Test setter here too
  particle.setMyInitvalDebugDouble(4.);
  var_double = particle.getMyInitvalDebugDouble();
  assert(var_double == 4.);

  particle.setMyStaticDebugDouble(initval_double);
  var_double = particle.getMyStaticDebugDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyStaticInitvalDebugDouble();
  assert(var_double == 8.);
  // Test setter here too
  particle.setMyStaticInitvalDebugDouble(particle.getMyStaticInitvalDebugDouble(
  ));
  var_double = particle.getMyStaticInitvalDebugDouble();
  assert(var_double == 8.);

  var_double = particle.getMyConstDebugDouble();
  assert(var_double == 16.);

  var_double = particle.getMyConstStaticDebugDouble();
  assert(var_double == 32.);

  var_double = particle.getMyConstExprDebugDouble();
  assert(var_double == 64.);
#endif


  // Doubles + compression
  // ----------------------

  particle.setMyCompressedDouble(initval_double);
  var_double = particle.getMyCompressedDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyInitvalCompressedDouble();
  assert(var_double == 4.);
  // Test setter here too
  particle.setMyInitvalCompressedDouble(4.);
  var_double = particle.getMyInitvalCompressedDouble();
  assert(var_double == 4.);

  particle.setMyStaticCompressedDouble(initval_double);
  var_double = particle.getMyStaticCompressedDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyStaticInitvalCompressedDouble();
  assert(var_double == 8.);
  // Test setter here too
  particle.setMyStaticInitvalCompressedDouble(
    particle.getMyStaticInitvalCompressedDouble()
  );
  var_double = particle.getMyStaticInitvalCompressedDouble();
  assert(var_double == 8.);

  var_double = particle.getMyConstCompressedDouble();
  assert(var_double == 16.);

  var_double = particle.getMyConstStaticCompressedDouble();
  assert(var_double == 32.);

  var_double = particle.getMyConstExprCompressedDouble();
  assert(var_double == 64.);


  // Doubles + ifdefs + compression
  // ------------------------------------

#if PeanoDebug > 0
  particle.setMyDebugCompressedDouble(initval_double);
  var_double = particle.getMyDebugCompressedDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyInitvalDebugCompressedDouble();
  assert(var_double == 4.);
  // Test setter here too
  particle.setMyInitvalDebugCompressedDouble(4.);
  var_double = particle.getMyInitvalDebugCompressedDouble();
  assert(var_double == 4.);

  particle.setMyStaticDebugCompressedDouble(initval_double);
  var_double = particle.getMyStaticDebugCompressedDouble();
  assert(var_double == initval_double);

  var_double = particle.getMyStaticInitvalDebugCompressedDouble();
  assert(var_double == 8.);
  // Test setter here too
  particle.setMyStaticInitvalDebugCompressedDouble(
    particle.getMyStaticInitvalDebugCompressedDouble()
  );
  var_double = particle.getMyStaticInitvalDebugCompressedDouble();
  assert(var_double == 8.);

  var_double = particle.getMyConstDebugCompressedDouble();
  assert(var_double == 16.);

  var_double = particle.getMyConstStaticDebugCompressedDouble();
  assert(var_double == 32.);

  var_double = particle.getMyConstExprDebugCompressedDouble();
  assert(var_double == 64.);
#endif
}


void swift2::dastgenTest::checkEnums(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  // Default enums
  // ----------------

  using MyEnum = tests::swift2::dastgenTest::globaldata::dummyPart::MyEnum;

  MyEnum e1 = MyEnum::Baz;
  particle.setMyEnum(e1);
  MyEnum e2 = particle.getMyEnum();
  assert(e2 == e1);

  // check casting to string for all MyEnumm variants
  std::string teststring;
  for (int i = 0; i != static_cast<int>(MyEnum::Last); i++) {
    MyEnum e   = static_cast<MyEnum>(i);
    teststring = tests::swift2::dastgenTest::globaldata::dummyPart::toString(e);
  }


  // MyEnumms + debug
  // ----------------

#if PeanoDebug > 0
  using MyEnumD = tests::swift2::dastgenTest::globaldata::dummyPart::
    MyDebugEnum;

  MyEnumD eD1 = MyEnumD::Baz;
  particle.setMyDebugEnum(eD1);
  MyEnumD eD2 = particle.getMyDebugEnum();
  assert(eD2 == eD1);

  // check casting to string for all MyEnumm variants
  for (int i = 0; i != static_cast<int>(MyEnumD::Last); i++) {
    MyEnumD e  = static_cast<MyEnumD>(i);
    teststring = tests::swift2::dastgenTest::globaldata::dummyPart::toString(e);
  }
#endif
}


void swift2::dastgenTest::checkIntegers(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  // First set some values, where applicable.
  const int initval_int = 4321;
  int       var_int;


  // default case integers
  // ----------------------

  particle.setMyInteger(initval_int);
  var_int = particle.getMyInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyInitvalInteger();
  assert(var_int == 123);
  // Test setter here too
  particle.setMyInitvalInteger(123);
  var_int = particle.getMyInitvalInteger();
  assert(var_int == 123);

  particle.setMyStaticInteger(initval_int);
  var_int = particle.getMyStaticInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyStaticInitvalInteger();
  assert(var_int == 1234);
  // Test setter here too
  particle.setMyStaticInitvalInteger(particle.getMyStaticInitvalInteger());
  var_int = particle.getMyStaticInitvalInteger();
  assert(var_int == 1234);

  var_int = particle.getMyConstInteger();
  assert(var_int == 12345);

  var_int = particle.getMyConstStaticInteger();
  assert(var_int == 123456);

  var_int = particle.getMyConstExprInteger();
  assert(var_int == 1234567);


  // integers + ifdefs
  // ----------------------

#if PeanoDebug > 0
  particle.setMyDebugInteger(initval_int);
  var_int = particle.getMyDebugInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyInitvalDebugInteger();
  assert(var_int == 123);
  // Test setter here too
  particle.setMyInitvalDebugInteger(123);
  var_int = particle.getMyInitvalDebugInteger();
  assert(var_int == 123);

  particle.setMyStaticDebugInteger(initval_int);
  var_int = particle.getMyStaticDebugInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyStaticInitvalDebugInteger();
  assert(var_int == 1234);
  // Test setter here too
  particle.setMyStaticInitvalDebugInteger(
    particle.getMyStaticInitvalDebugInteger()
  );
  var_int = particle.getMyStaticInitvalDebugInteger();
  assert(var_int == 1234);

  var_int = particle.getMyConstDebugInteger();
  assert(var_int == 12345);

  var_int = particle.getMyConstStaticDebugInteger();
  assert(var_int == 123456);

  var_int = particle.getMyConstExprDebugInteger();
  assert(var_int == 1234567);
#endif


  // integers + compression
  // ----------------------

  particle.setMyCompressedInteger(initval_int);
  var_int = particle.getMyCompressedInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyInitvalCompressedInteger();
  assert(var_int == 123);
  // Test setter here too
  particle.setMyInitvalCompressedInteger(123);
  var_int = particle.getMyInitvalCompressedInteger();
  assert(var_int == 123);

  particle.setMyStaticCompressedInteger(initval_int);
  var_int = particle.getMyStaticCompressedInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyStaticInitvalCompressedInteger();
  assert(var_int == 1234);
  // Test setter here too
  particle.setMyStaticInitvalCompressedInteger(
    particle.getMyStaticInitvalCompressedInteger()
  );
  var_int = particle.getMyStaticInitvalCompressedInteger();
  assert(var_int == 1234);

  var_int = particle.getMyConstCompressedInteger();
  assert(var_int == 12345);

  var_int = particle.getMyConstStaticCompressedInteger();
  assert(var_int == 123456);

  var_int = particle.getMyConstExprCompressedInteger();
  assert(var_int == 1234567);


  // integers + ifdefs + compression
  // ------------------------------------

#if PeanoDebug > 0
  particle.setMyDebugCompressedInteger(initval_int);
  var_int = particle.getMyDebugCompressedInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyInitvalDebugCompressedInteger();
  assert(var_int == 123);
  // Test setter here too
  particle.setMyInitvalDebugCompressedInteger(123);
  var_int = particle.getMyInitvalDebugCompressedInteger();
  assert(var_int == 123);

  particle.setMyStaticDebugCompressedInteger(initval_int);
  var_int = particle.getMyStaticDebugCompressedInteger();
  assert(var_int == initval_int);

  var_int = particle.getMyStaticInitvalDebugCompressedInteger();
  assert(var_int == 1234);
  // Test setter here too
  particle.setMyStaticInitvalDebugCompressedInteger(
    particle.getMyStaticInitvalDebugCompressedInteger()
  );
  var_int = particle.getMyStaticInitvalDebugCompressedInteger();
  assert(var_int == 1234);

  var_int = particle.getMyConstDebugCompressedInteger();
  assert(var_int == 12345);

  var_int = particle.getMyConstStaticDebugCompressedInteger();
  assert(var_int == 123456);

  var_int = particle.getMyConstExprDebugCompressedInteger();
  assert(var_int == 1234567);
#endif
}


void swift2::dastgenTest::checkStrings(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  std::string var_str;
  std::string initval_str = "In a hole in the ground there lived a hobbit";


  // Default strings
  // -----------------

  particle.setMyString(initval_str);
  var_str = particle.getMyString();
  assert(var_str.compare(initval_str) == 0);

  var_str = particle.getMyInitvalString();
  assert(var_str.compare("initval\0") == 0);
  // Test setter here too
  particle.setMyInitvalString("initval\0");
  var_str = particle.getMyInitvalString();
  assert(var_str.compare("initval\0") == 0);

  var_str = particle.getMyStaticInitvalString();
  assert(var_str.compare("static_initval\0") == 0);
  // Test setter here too
  particle.setMyStaticInitvalString("static_initval\0");
  var_str = particle.getMyStaticInitvalString();
  assert(var_str.compare("static_initval\0") == 0);

  var_str = particle.getMyConstString();
  assert(var_str.compare("const_initval\0") == 0);

  var_str = particle.getMyConstStaticString();
  assert(var_str.compare("const_static_initval\0") == 0);

  // constexpr left out for now. Not working yet. Not needed atm.


  // strings + ifdefs
  // -----------------

#if PeanoDebug > 0
  particle.setMyDebugString(initval_str);
  var_str = particle.getMyDebugString();
  assert(var_str.compare(initval_str) == 0);

  var_str = particle.getMyInitvalDebugString();
  assert(var_str.compare("initval\0") == 0);
  // Test setter here too
  particle.setMyInitvalDebugString("initval\0");
  var_str = particle.getMyInitvalDebugString();
  assert(var_str.compare("initval\0") == 0);

  var_str = particle.getMyStaticInitvalDebugString();
  assert(var_str.compare("static_initval\0") == 0);
  // Test setter here too
  particle.setMyStaticInitvalDebugString("static_initval\0");
  var_str = particle.getMyStaticInitvalDebugString();
  assert(var_str.compare("static_initval\0") == 0);

  var_str = particle.getMyConstDebugString();
  assert(var_str.compare("const_initval\0") == 0);

  var_str = particle.getMyConstStaticDebugString();
  assert(var_str.compare("const_static_initval\0") == 0);

  // constexpr left out for now. Not working yet. Not needed atm.
#endif
}


void checkUserDefinedType(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  auto a = particle.getMyStaticUserDefinedAttribute();
#if PeanoDebug > 0
  auto b = particle.getMyStaticDebugUserDefinedAttribute();
#endif
  // We currently have no setters for User Defined Attributes.
}


void swift2::dastgenTest::checkBooleanArrays(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  std::bitset<8> init_bool = static_cast<std::bitset<8>>(511);
  std::bitset<8> bool_var;

  // Default Bool Array
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyBooleanArray(i, true);
    assert(particle.getMyBooleanArray(i));
  }

  // getter/setter with array
  particle.setMyBooleanArray(init_bool);
  bool_var = particle.getMyBooleanArray();
  for (int i = 0; i < 8; i++) {
    assert(bool_var[i] == particle.getMyBooleanArray(i));
  }


  // Bool Array + debug
  // -------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDebugBooleanArray(i, true);
    assert(particle.getMyDebugBooleanArray(i));
  }

  // getter/setter with array
  particle.setMyDebugBooleanArray(init_bool);
  bool_var = particle.getMyDebugBooleanArray();
  for (int i = 0; i < 8; i++) {
    assert(bool_var[i] == particle.getMyDebugBooleanArray(i));
  }
#endif

  // Bool Array + compress
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedBooleanArray(i, true);
    assert(particle.getMyCompressedBooleanArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedBooleanArray(init_bool);
  bool_var = particle.getMyCompressedBooleanArray();
  for (int i = 0; i < 8; i++) {
    assert(bool_var[i] == particle.getMyCompressedBooleanArray(i));
  }


  // Bool Array + debug + compression
  // ------------------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDebugBooleanArray(i, true);
    assert(particle.getMyCompressedDebugBooleanArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDebugBooleanArray(init_bool);
  bool_var = particle.getMyCompressedDebugBooleanArray();
  for (int i = 0; i < 8; i++) {
    assert(bool_var[i] == particle.getMyCompressedDebugBooleanArray(i));
  }
#endif
}


void swift2::dastgenTest::checkDoubleArrays(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  double init_double[8] = {1., 2., 3., 4., 5., 6., 7., 8.};

  // Default Double Array
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDoubleArray(i, i + 10.);
    assert(particle.getMyDoubleArray(i) == i + 10.);
  }

  // getter/setter with array
  particle.setMyDoubleArray(init_double);
  const double* double_var1 = particle.getMyDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var1[i] == particle.getMyDoubleArray(i));
  }


  // Double Array + debug
  // -------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDebugDoubleArray(i, i + 11.);
    assert(particle.getMyDebugDoubleArray(i) == i + 11.);
  }

  // getter/setter with array
  particle.setMyDebugDoubleArray(init_double);
  const double* double_var2 = particle.getMyDebugDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var2[i] == particle.getMyDebugDoubleArray(i));
  }
#endif

  // Double Array + compress
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDoubleArray(i, i + 12.);
    assert(particle.getMyCompressedDoubleArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDoubleArray(init_double);
  const double* double_var3 = particle.getMyCompressedDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var3[i] == particle.getMyCompressedDoubleArray(i));
  }


  // Double Array + debug + compression
  // ------------------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDebugDoubleArray(i, i + 13.);
    assert(particle.getMyCompressedDebugDoubleArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDebugDoubleArray(init_double);
  const double* double_var4 = particle.getMyCompressedDebugDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var4[i] == particle.getMyCompressedDebugDoubleArray(i));
  }
#endif
}


void swift2::dastgenTest::checkIntegerArrays(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  int init_integer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  // Default Integer Array
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyIntegerArray(i, i + 10);
    assert(particle.getMyIntegerArray(i) == i + 10);
  }

  // getter/setter with array
  particle.setMyIntegerArray(init_integer);
  const int* integer_var1 = particle.getMyIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var1[i] == particle.getMyIntegerArray(i));
  }


  // Integer Array + debug
  // -------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDebugIntegerArray(i, i + 11);
    assert(particle.getMyDebugIntegerArray(i) == i + 11);
  }

  // getter/setter with array
  particle.setMyDebugIntegerArray(init_integer);
  const int* integer_var2 = particle.getMyDebugIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var2[i] == particle.getMyDebugIntegerArray(i));
  }
#endif

  // Integer Array + compress
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedIntegerArray(i, i + 12);
    assert(particle.getMyCompressedIntegerArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedIntegerArray(init_integer);
  const int* integer_var3 = particle.getMyCompressedIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var3[i] == particle.getMyCompressedIntegerArray(i));
  }


  // Integer Array + debug + compression
  // ------------------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDebugIntegerArray(i, i + 13);
    assert(particle.getMyCompressedDebugIntegerArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDebugIntegerArray(init_integer);
  const int* integer_var4 = particle.getMyCompressedDebugIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var4[i] == particle.getMyCompressedDebugIntegerArray(i));
  }
#endif
}


void swift2::dastgenTest::checkPeanoDoubleArrays(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  double init_double[8] = {1., 2., 3., 4., 5., 6., 7., 8.};

  // Default Double Array
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyPeanoDoubleArray(i, i + 10.);
    assert(particle.getMyPeanoDoubleArray(i) == i + 10.);
  }

  // getter/setter with array
  particle.setMyPeanoDoubleArray(init_double);
  const tarch::la::Vector<8, double>
    double_var1 = particle.getMyPeanoDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var1[i] == particle.getMyPeanoDoubleArray(i));
  }


  // Double Array + debug
  // -------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDebugDoubleArray(i, i + 11.);
    assert(particle.getMyDebugDoubleArray(i) == i + 11.);
  }

  // getter/setter with array
  particle.setMyDebugDoubleArray(init_double);
  const tarch::la::Vector<8, double>
    double_var2 = particle.getMyDebugDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var2[i] == particle.getMyDebugDoubleArray(i));
  }
#endif

  // Double Array + compress
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDoubleArray(i, i + 12.);
    assert(particle.getMyCompressedDoubleArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDoubleArray(init_double);
  const tarch::la::Vector<8, double>
    double_var3 = particle.getMyCompressedDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var3[i] == particle.getMyCompressedDoubleArray(i));
  }


  // Double Array + debug + compression
  // ------------------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDebugDoubleArray(i, i + 13.);
    assert(particle.getMyCompressedDebugDoubleArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDebugDoubleArray(init_double);
  const tarch::la::Vector<8, double>
    double_var4 = particle.getMyCompressedDebugDoubleArray();
  for (int i = 0; i < 8; i++) {
    assert(double_var4[i] == particle.getMyCompressedDebugDoubleArray(i));
  }
#endif
}


void swift2::dastgenTest::checkPeanoIntegerArrays(
  tests::swift2::dastgenTest::globaldata::dummyPart& particle
) {

  int init_integer[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  // Default PeanoInteger Array
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyPeanoIntegerArray(i, i + 10);
    assert(particle.getMyPeanoIntegerArray(i) == i + 10);
  }

  // getter/setter with array
  particle.setMyPeanoIntegerArray(init_integer);
  const tarch::la::Vector<8, int>
    integer_var1 = particle.getMyPeanoIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var1[i] == particle.getMyPeanoIntegerArray(i));
  }


  // PeanoInteger Array + debug
  // -------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyDebugPeanoIntegerArray(i, i + 11);
    assert(particle.getMyDebugPeanoIntegerArray(i) == i + 11);
  }

  // getter/setter with array
  particle.setMyDebugPeanoIntegerArray(init_integer);
  const tarch::la::Vector<8, int>
    integer_var2 = particle.getMyDebugPeanoIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var2[i] == particle.getMyDebugPeanoIntegerArray(i));
  }
#endif

  // PeanoInteger Array + compress
  // -------------------------

  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedPeanoIntegerArray(i, i + 12);
    assert(particle.getMyCompressedPeanoIntegerArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedPeanoIntegerArray(init_integer);
  const tarch::la::Vector<8, int>
    integer_var3 = particle.getMyCompressedPeanoIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(integer_var3[i] == particle.getMyCompressedPeanoIntegerArray(i));
  }


  // PeanoInteger Array + debug + compression
  // ------------------------------------

#if PeanoDebug > 0
  // getter/setter with index
  for (int i = 0; i < 8; i++) {
    particle.setMyCompressedDebugPeanoIntegerArray(i, i + 13);
    assert(particle.getMyCompressedDebugPeanoIntegerArray(i));
  }

  // getter/setter with array
  particle.setMyCompressedDebugPeanoIntegerArray(init_integer);
  const tarch::la::Vector<8, int>
    integer_var4 = particle.getMyCompressedDebugPeanoIntegerArray();
  for (int i = 0; i < 8; i++) {
    assert(
      integer_var4[i] == particle.getMyCompressedDebugPeanoIntegerArray(i)
    );
  }
#endif
}


void swift2::dastgenTest::checkConstructors(void) {

  using Part = tests::swift2::dastgenTest::globaldata::dummyPart;

  // Prepare some dummy arguments
  tarch::la::Vector<Dimensions, double> dummyDimDoubleVector = {0.1, 0.2, 0.3};
  tarch::la::Vector<8, double>          dummyDoubleVector    = {
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
  tarch::la::Vector<8, int> dummyIntVector = {1, 2, 3, 4, 5, 6, 7, 8};
  std::bitset<8>            dummyBitset    = 0b11111111;
  double dummyDoubleArr[8] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
  int    dummyIntArr[8]    = {1, 2, 3, 4, 5, 6, 7, 8};
  char   dummyString[80]   = "Hello there";

  // First the copy constructor
  Part p1 = Part();
  // set a handful of values for checks later
  // Should be the same as for the Part p2 below
  p1.setMyDouble(2.);
  p1.setMyInteger(1);
  p1.setMyString(dummyString);
  p1.setMyBoolean(true);

  // Now the full-arg constructor
  // For the values where we provided initial values in the dastgen script,
  // copy them over from p1. Otherwise, checks further below will fail.
  // I'm manually setting the string initial values here because they were a
  // hassle to copy correctly and pass the subsequent tests.
  char initvalString[80]      = "initval\0";
  int  initLen                = 8;
  char initvalDebugString[80] = "initval\0";
  int  initLenDebug           = 8;


  Part p2 = Part(
    /* tarch::la::Vector<Dimensions,double>  __debugX = */ dummyDimDoubleVector,
    /* tarch::la::Vector<Dimensions,double>  __debugH = */ dummyDimDoubleVector
      * 2.,
    /* tarch::la::Vector<Dimensions,double>  __x = */ dummyDimDoubleVector * 3.,
    /* tarch::la::Vector<Dimensions,double>  __cellH = */ dummyDimDoubleVector
      * 4.,
    /* double  __searchRadius = */ 1.,
    /* ParallelState  __ParallelState = */ Part::ParallelState::Local,
    /* MoveState  __MoveState = */ Part::MoveState::New,
    /* bool  __CellHasUpdatedParticle = */ false,
    /* bool  __myBoolean = */ true,
    /* bool  __myInitvalBoolean = */ p1.getMyInitvalBoolean(),
#if PeanoDebug > 0
    /* bool  __myDebugBoolean = */ true,
    /* bool  __myInitvalDebugBoolean = */ p1.getMyInitvalDebugBoolean(),
#endif
    /* double  __myDouble =  */ 2.,
    /* double  __myInitvalDouble = */ p1.getMyInitvalDouble(),
#if PeanoDebug > 0
    /* double  __myDebugDouble = */ 4.,
    /* double  __myInitvalDebugDouble = */ p1.getMyInitvalDebugDouble(),
#endif
    /* double  __myCompressedDouble = */ 6.,
    /* double  __myInitvalCompressedDouble = */
    p1.getMyInitvalCompressedDouble(),
#if PeanoDebug > 0
    /* double  __myDebugCompressedDouble = */ 8.,
    /* double  __myInitvalDebugCompressedDouble = */
    p1.getMyInitvalDebugCompressedDouble(),
#endif
    /* MyEnum  __myEnum = */ Part::MyEnum::Foo,
#if PeanoDebug > 0
    /* MyDebugEnum  __myDebugEnum = */ Part::MyDebugEnum::Bar,
#endif
    /* int  __myInteger = */ 1,
    /* int  __myInitvalInteger = */ p1.getMyInitvalInteger(),
#if PeanoDebug > 0
    /* int  __myDebugInteger = */ 3,
    /* int  __myInitvalDebugInteger = */ p1.getMyInitvalDebugInteger(),
#endif
    /* int  __myCompressedInteger = */ 5,
    /* int  __myInitvalCompressedInteger = */ p1.getMyInitvalCompressedInteger(),
#if PeanoDebug > 0
    /* int  __myDebugCompressedInteger = */ 7,
    /* int  __myInitvalDebugCompressedInteger = */
    p1.getMyInitvalDebugCompressedInteger(),
#endif
    /* char  __myString[80] = */ dummyString,
    /* int  __myStringLength = */ 11,
    /* char  __myInitvalString[80] = */ initvalString,
    /* int  __myInitvalStringLength = */ initLen,
#if PeanoDebug > 0
    /* char  __myDebugString[80] = */ dummyString,
    /* int  __myDebugStringLength = */ 11,
    /* char  __myInitvalDebugString[80] = */ initvalDebugString,
    /* int  __myInitvalDebugStringLength = */ initLenDebug,
#endif
    /* std::bitset<8>  __myBooleanArray = */ dummyBitset,
    /* std::bitset<8>  __myCompressedBooleanArray = */ dummyBitset,
#if PeanoDebug > 0
    /* std::bitset<8>  __myDebugBooleanArray = */ dummyBitset,
    /* std::bitset<8>  __myDebugCompressedBooleanArray = */ dummyBitset,
#endif
    /* double  __myDoubleArray[8] = */ dummyDoubleArr,
    /* double  __myCompressedDoubleArray[8] = */ dummyDoubleArr,
#if PeanoDebug > 0
    /* double  __myDebugDoubleArray[8] = */ dummyDoubleArr,
    /* double  __myDebugCompressedDoubleArray[8] = */ dummyDoubleArr,
#endif
    /* int  __myIntegerArray[8] = */ dummyIntArr,
    /* int  __myCompressedIntegerArray[8] = */ dummyIntArr,
#if PeanoDebug > 0
    /* int  __myDebugIntegerArray[8] = */ dummyIntArr,
    /* int  __myDebugCompressedIntegerArray[8] = */ dummyIntArr,
#endif
    /* tarch::la::Vector<8,double>  __myPeanoDoubleArray = */ dummyDoubleVector,
    /* tarch::la::Vector<8,double>  __myCompressedPeanoDoubleArray = */
    dummyDoubleVector,
#if PeanoDebug > 0
    /* tarch::la::Vector<8,double>  __myDebugPeanoDoubleArray = */
    dummyDoubleVector,
    /* tarch::la::Vector<8,double>  __myDebugCompressedPeanoDoubleArray = */
    dummyDoubleVector,
#endif
    /* tarch::la::Vector<8,int>  __myPeanoIntegerArray = */ dummyIntVector,
    /* tarch::la::Vector<8,int>  __myCompressedPeanoIntegerArray = */
    dummyIntVector,
#if PeanoDebug > 0
    /* tarch::la::Vector<8,int>  __myDebugPeanoIntegerArray = */ dummyIntVector,
    /* tarch::la::Vector<8,int>  __myDebugCompressedPeanoIntegerArray); */
    dummyIntVector
#endif
  );


  // Sporadically check that everything has been set correctly

  assert(p1.getMyDouble() == p2.getMyDouble());
  assert(p1.getMyInteger() == p2.getMyInteger());
  assert(p1.getMyBoolean() == p2.getMyBoolean());


  // Run the same checks as for the other particles
  checkBooleans(p1);
  checkDoubles(p1);
  checkEnums(p1);
  checkIntegers(p1);
  checkStrings(p1);

  checkBooleanArrays(p1);
  checkDoubleArrays(p1);
  checkIntegerArrays(p1);

  checkPeanoDoubleArrays(p1);
  checkPeanoIntegerArrays(p1);


  checkBooleans(p2);
  checkDoubles(p2);
  checkEnums(p2);
  checkIntegers(p2);
  checkStrings(p2);

  checkBooleanArrays(p2);
  checkDoubleArrays(p2);
  checkIntegerArrays(p2);

  checkPeanoDoubleArrays(p2);
  checkPeanoIntegerArrays(p2);
}
