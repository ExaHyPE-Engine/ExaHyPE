// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterTest.h"

#include "exahype2/dg/Limiter.h"

exahype2::dg::tests::LimiterTest::LimiterTest() :
  tarch::tests::TestCase("exahype2::dg::tests::LimiterTest") {}

void exahype2::dg::tests::LimiterTest::run() {
  testMethod(testExtremaMerges);
  testMethod(testDiscreteMaximumPrinciple);
  testMethod(testObservableExtremaReduction);
  testMethod(testProjectedObservableExtremaReduction);
  testMethod(testExtremumQuantisation);
  testMethod(testHangingVertexFacet);
}

void exahype2::dg::tests::LimiterTest::testHangingVertexFacet() {
  // A hanging vertex on the lower boundary of the first axis and inside the
  // second axis lies on the father cell's left face, whose vertices are 0 and 2.
  const tarch::la::Vector<2, int> onLeftFace = {0, 1};
  validate(exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onLeftFace, 0));
  validate(not exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onLeftFace, 1));
  validate(exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onLeftFace, 2));
  validate(not exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onLeftFace, 3));

  // A hanging vertex that coincides with the upper-right coarse vertex lies on
  // that vertex alone.
  const tarch::la::Vector<2, int> onCorner = {3, 3};
  validate(not exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onCorner, 0));
  validate(not exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onCorner, 1));
  validate(not exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onCorner, 2));
  validate(exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onCorner, 3));

  // In three dimensions a hanging vertex inside an edge lies on the edge's two
  // vertices: Lower along the second axis, upper along the third.
  const tarch::la::Vector<3, int> onEdge = {2, 0, 3};
  for (int coarseVertexNumber = 0; coarseVertexNumber < 8; coarseVertexNumber++) {
    const bool onEdgeVertices = coarseVertexNumber == 4 or coarseVertexNumber == 5;
    validateEquals(
      exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(onEdge, coarseVertexNumber),
      onEdgeVertices
    );
  }
}

void exahype2::dg::tests::LimiterTest::testExtremumQuantisation() {
  constexpr double Value        = 0.6427813001938251;
  const double     lastBitNoise = std::nextafter(Value, 1.0);

  validateNumericalEquals(
    exahype2::dg::quantiseObservableExtremum(Value),
    exahype2::dg::quantiseObservableExtremum(lastBitNoise)
  );
  validateNumericalEquals(
    exahype2::dg::quantiseObservableExtremum(exahype2::dg::quantiseObservableExtremum(Value)),
    exahype2::dg::quantiseObservableExtremum(Value)
  );
  validateNumericalEquals(
    exahype2::dg::quantiseObservableExtremum(-Value),
    -exahype2::dg::quantiseObservableExtremum(Value)
  );
  validate(std::abs(exahype2::dg::quantiseObservableExtremum(Value) - Value) < 1.0e-11 * Value);
  validateNumericalEquals(exahype2::dg::quantiseObservableExtremum(0.0), 0.0);
  validate(std::isinf(exahype2::dg::quantiseObservableExtremum(std::numeric_limits<double>::infinity())));
  validate(std::isnan(exahype2::dg::quantiseObservableExtremum(std::numeric_limits<double>::quiet_NaN())));
}

void exahype2::dg::tests::LimiterTest::testDiscreteMaximumPrinciple() {
  constexpr double NoRelaxation = 0.0;
  constexpr double NoScaling    = 0.0;

  validate(exahype2::dg::satisfiesDiscreteMaximumPrinciple(0.4, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(not exahype2::dg::satisfiesDiscreteMaximumPrinciple(-0.1, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(not exahype2::dg::satisfiesDiscreteMaximumPrinciple(0.4, 1.1, 0.0, 1.0, NoRelaxation, NoScaling));

  validate(exahype2::dg::satisfiesDiscreteMaximumPrinciple(-0.05, 0.6, 0.0, 1.0, 0.1, NoScaling));
  validate(exahype2::dg::satisfiesDiscreteMaximumPrinciple(-0.05, 0.6, 0.0, 1.0, NoRelaxation, 0.1));
  validate(exahype2::dg::satisfiesDiscreteMaximumPrinciple(-0.15, 0.6, 0.0, 1.0, 0.2, 0.1));
  validate(not exahype2::dg::satisfiesDiscreteMaximumPrinciple(-0.25, 0.6, 0.0, 1.0, 0.2, 0.1));

  constexpr double NaN              = std::numeric_limits<double>::quiet_NaN();
  constexpr double PositiveInfinity = std::numeric_limits<double>::infinity();
  validate(not exahype2::dg::satisfiesDiscreteMaximumPrinciple(NaN, 0.6, 0.0, 1.0, NoRelaxation, NoScaling));
  validate(not exahype2::dg::satisfiesDiscreteMaximumPrinciple(
    0.4,
    0.6,
    PositiveInfinity,
    -PositiveInfinity,
    NoRelaxation,
    NoScaling
  ));
}

void exahype2::dg::tests::LimiterTest::testObservableExtremaReduction() {
  constexpr int NumberOfObservables                = 2;
  constexpr int NumberOfNodes                      = 3;
  constexpr int Stride                             = 2;
  const double  nodalState[NumberOfNodes * Stride] = {1.0, 2.0, 3.0, 4.0, 0.0, 5.0};

  constexpr double PositiveInfinity             = std::numeric_limits<double>::infinity();
  double           minimum[NumberOfObservables] = {PositiveInfinity, PositiveInfinity};
  double           maximum[NumberOfObservables] = {-PositiveInfinity, -PositiveInfinity};

  exahype2::dg::reduceObservableExtrema<NumberOfObservables>(
    minimum,
    maximum,
    nodalState,
    NumberOfNodes,
    Stride,
    [](const double* const state, double* const observables) {
      observables[0] = state[0];
      observables[1] = state[1];
    }
  );

  validateNumericalEquals(minimum[0], 0.0);
  validateNumericalEquals(minimum[1], 2.0);
  validateNumericalEquals(maximum[0], 3.0);
  validateNumericalEquals(maximum[1], 5.0);
}

void exahype2::dg::tests::LimiterTest::testProjectedObservableExtremaReduction() {
  constexpr int    NumberOfObservables   = 1;
  constexpr int    InputDofs             = 2;
  constexpr int    NumberOfData          = 2;
  constexpr double PositiveInfinity      = std::numeric_limits<double>::infinity();
  const auto       mapStateToObservables = [](const double* const state, double* const observables) {
    observables[0] = state[0];
  };

  // An identity sampling matrix reproduces the nodal values, so the projected
  // extrema equal the plain nodal extrema.
  const double identity[InputDofs * InputDofs]                  = {1.0, 0.0, 0.0, 1.0};
  const double nodalState[InputDofs * InputDofs * NumberOfData] = {1.0, 0.0, -2.0, 0.0, 3.0, 0.0, 0.5, 0.0};
  double       minimum[NumberOfObservables]                     = {PositiveInfinity};
  double       maximum[NumberOfObservables]                     = {-PositiveInfinity};
  exahype2::dg::reduceProjectedObservableExtrema<2, InputDofs, InputDofs, NumberOfData, NumberOfObservables>(
    minimum,
    maximum,
    nodalState,
    identity,
    mapStateToObservables
  );
  validateNumericalEquals(minimum[0], -2.0);
  validateNumericalEquals(maximum[0], 3.0);

  // Every column of the sampling matrix sums to one, so a constant field
  // samples to the constant on every sample point.
  constexpr int    SampleDofs                               = 3;
  constexpr double Constant                                 = 7.0;
  const double     columnStochastic[InputDofs * SampleDofs] = {0.25, 0.5, 0.75, 0.75, 0.5, 0.25};
  const double     constantState[InputDofs * InputDofs * NumberOfData] =
    {Constant, 0.0, Constant, 0.0, Constant, 0.0, Constant, 0.0};
  double constantMinimum[NumberOfObservables] = {PositiveInfinity};
  double constantMaximum[NumberOfObservables] = {-PositiveInfinity};
  exahype2::dg::reduceProjectedObservableExtrema<2, InputDofs, SampleDofs, NumberOfData, NumberOfObservables>(
    constantMinimum,
    constantMaximum,
    constantState,
    columnStochastic,
    mapStateToObservables
  );
  validateNumericalEquals(constantMinimum[0], Constant);
  validateNumericalEquals(constantMaximum[0], Constant);
}

void exahype2::dg::tests::LimiterTest::testExtremaMerges() {
  constexpr double        FiniteValue         = 4.0;
  constexpr double        SmallerFiniteValue  = -2.0;
  constexpr double        PositiveZero        = 0.0;
  constexpr double        NegativeZero        = -0.0;
  constexpr double        PositiveInfinity    = std::numeric_limits<double>::infinity();
  constexpr double        NegativeInfinity    = -PositiveInfinity;
  constexpr std::uint64_t NoncanonicalNaNBits = 0x7ff8000000000042ULL;
  constexpr std::uint64_t CanonicalNaNBits    = std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN());
  constexpr double        NoncanonicalNaN     = std::bit_cast<double>(NoncanonicalNaNBits);

  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(FiniteValue, SmallerFiniteValue),
    SmallerFiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(SmallerFiniteValue, FiniteValue),
    SmallerFiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(FiniteValue, SmallerFiniteValue),
    FiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(SmallerFiniteValue, FiniteValue),
    FiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(PositiveInfinity, FiniteValue),
    FiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(FiniteValue, PositiveInfinity),
    FiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NegativeInfinity, FiniteValue),
    FiniteValue
  );
  validateNumericalEquals(
    exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(FiniteValue, NegativeInfinity),
    FiniteValue
  );
  validateNumericalEquals(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(FiniteValue, FiniteValue), FiniteValue);
  validateNumericalEquals(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(FiniteValue, FiniteValue), FiniteValue);
  validate(std::isinf(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(PositiveInfinity, PositiveInfinity)));
  validate(not std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(PositiveInfinity, PositiveInfinity)));
  validate(std::isinf(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NegativeInfinity, NegativeInfinity)));
  validate(std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NegativeInfinity, NegativeInfinity)));

  const auto leftMinimumNaN  = exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(NoncanonicalNaN, FiniteValue);
  const auto rightMinimumNaN = exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(FiniteValue, NoncanonicalNaN);
  const auto leftMaximumNaN  = exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NoncanonicalNaN, FiniteValue);
  const auto rightMaximumNaN = exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(FiniteValue, NoncanonicalNaN);
  validateEquals(std::bit_cast<std::uint64_t>(leftMinimumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(rightMinimumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(leftMaximumNaN), CanonicalNaNBits);
  validateEquals(std::bit_cast<std::uint64_t>(rightMaximumNaN), CanonicalNaNBits);
  validateEquals(
    std::bit_cast<std::uint64_t>(
      exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(leftMinimumNaN, SmallerFiniteValue)
    ),
    CanonicalNaNBits
  );
  validateEquals(
    std::bit_cast<std::uint64_t>(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(leftMaximumNaN, FiniteValue)),
    CanonicalNaNBits
  );

  validate(std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(PositiveZero, NegativeZero)));
  validate(std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(NegativeZero, PositiveZero)));
  validate(not std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(PositiveZero, NegativeZero)));
  validate(not std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NegativeZero, PositiveZero)));
  validate(not std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(PositiveZero, PositiveZero)));
  validate(std::signbit(exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(NegativeZero, NegativeZero)));
}
