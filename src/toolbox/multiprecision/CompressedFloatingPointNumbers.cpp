#include "CompressedFloatingPointNumbers.h"
#include "tarch/Assertions.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/la/Scalar.h"

#include <bitset>


int toolbox::multiprecision::findMostAgressiveCompression(
  const double  values[],
  int           count,
  double        maxError
) {
  assertion(count>0);

  int result = 0;
  for (int i=0; i<count; i++) {
    result = std::max(result, findMostAgressiveCompression(values[i],maxError));
  }
  assertion(result>0);
  return result;
}


#if defined(CompilerICC)
int toolbox::multiprecision::findMostAgressiveCompression(
#elif defined(CompilerCLANG)
int __attribute__((optnone)) toolbox::multiprecision::findMostAgressiveCompression(
#else
int __attribute__((optimize("O0"))) toolbox::multiprecision::findMostAgressiveCompression(
#endif
  double        value,
  double        maxError
) {
  if (std::abs(value)<maxError) {
	return 0;
  }

  assertion(value==value);
  assertion1( sizeof(long int)*8>=64, sizeof(long int) );
  assertion(maxError>=0.0);

  // We may not use 7 though we use seven out of eight bits for the first byte.
  // If we shift by seven, we can end up with the highest byte set for
  // 0.0155759, e.g.
  int shiftExponent = 6;

  const long int sign = value < 0.0 ? -1 : 1;

  if (sign<0) {
    value = -value;
  }

  int           integerExponent;
  const double  significand          = std::frexp(value , &integerExponent);

  int    usedBytes = 0;
  double error     = std::numeric_limits<double>::max();
  while (
    usedBytes<6
    &&
    error > maxError
  ) {
    usedBytes++;

    const double shiftMantissa    = std::pow( 2.0,shiftExponent );

    int      exponent  = integerExponent-shiftExponent;
    long int mantissa  = static_cast<long int>( std::round(significand*shiftMantissa) );

    error     = std::abs( std::ldexp(mantissa,exponent) - value );

    shiftExponent+=8;
  }

  assertion(usedBytes>=1);
  assertion(usedBytes<=7);

  return usedBytes;
}



#ifdef CompilerICC
void toolbox::multiprecision::decomposeIntoEightVariants(
#elif defined(CompilerCLANG)
int __attribute__((optnone)) toolbox::multiprecision::findMostAgressiveCompression(
#else
void __attribute__((optimize("O0"))) toolbox::multiprecision::decomposeIntoEightVariants(
#endif
  double        value,
  char          exponent[8],
  long int      mantissa[8],
  double        error[8]
) {
  assertion(value==value);
  assertion1( sizeof(long int)*8>=64, sizeof(long int) );

  // We may not use 7 though we use seven out of eight bits for the first byte.
  // If we shift by seven, we can end up with the highest byte set for
  // 0.0155759, e.g.
  int shiftExponent = 6;

  const long int sign = value < 0.0 ? -1 : 1;

  if (sign<0) {
    value = -value;
  }

  int           integerExponent;
  const double  significand          = std::frexp(value , &integerExponent);

  for (int i=0; i<8; i++) {
    const double shiftMantissa    = std::pow( 2.0,shiftExponent );

    if (integerExponent-shiftExponent >= std::numeric_limits<char>::min() ) {
      exponent[i]  = static_cast<char>( integerExponent-shiftExponent );
      mantissa[i]  = static_cast<long int>( std::round(significand*shiftMantissa) );
    }
    else {
      exponent[i] = 0;
      mantissa[i] = 0;
    }
    error[i]     = std::abs( std::ldexp(mantissa[i],exponent[i]) - value );

    assertion5( mantissa[i]>=0, value, mantissa[i], exponent[i], error[i], sign );

    std::bitset<64>*  mantissaAsBitset = reinterpret_cast<std::bitset<64>*>( &(mantissa[i]) );

    #ifdef Asserts
    for (int j=(i+1)*8-1; j<64; j++) {
      assertion9(
        !(*mantissaAsBitset)[j],
        *mantissaAsBitset, value, static_cast<int>( exponent[i] ), mantissa[i], error[i], i, j, significand, integerExponent
      );
    }
    #endif

    if (sign<0) {
      assertion ( (*mantissaAsBitset)[ (i+1)*8-1 ]==false );
      mantissaAsBitset->flip( (i+1)*8-1 );
    }

    shiftExponent+=8;
  }
}


#ifdef CompilerICC
void toolbox::multiprecision::decompose(
#elif defined(CompilerCLANG)
int __attribute__((optnone)) toolbox::multiprecision::findMostAgressiveCompression(
#else
void __attribute__((optimize("O0"))) toolbox::multiprecision::decompose(
#endif
  double        value,
  char&         exponent,
  long int&     mantissa,
  int           bytesForMantissa
) {
  assertion(value==value);
  assertion1( sizeof(long int)*8>=64, sizeof(long int) );

  // We may not use 7 though we use seven out of eight bits for the first byte.
  // If we shift by seven, we can end up with the highest byte set for
  // 0.0155759, e.g.
  int shiftExponent = 6 + (bytesForMantissa-1)*8;

  const long int sign = value < 0.0 ? -1 : 1;

  if (sign<0) {
    value = -value;
  }

  int           integerExponent;
  const double  significand          = std::frexp(value , &integerExponent);

  int exponentAsInteger  = integerExponent-shiftExponent;

  if (exponentAsInteger <= std::numeric_limits<char>::min()) {
    exponent  = 0;
    mantissa  = 0;
  }
  else {
    exponent  = static_cast<char>( exponentAsInteger );

    const double shiftMantissa    = std::pow( 2.0,shiftExponent );
    mantissa  = static_cast<long int>( std::round(significand*shiftMantissa) );

    assertion4( mantissa>=0, value, mantissa, exponent, sign );

    std::bitset<64>*  mantissaAsBitset = reinterpret_cast<std::bitset<64>*>( &(mantissa) );

    if (sign<0) {
      assertion ( (*mantissaAsBitset)[ (bytesForMantissa)*8-1 ]==false );
      mantissaAsBitset->flip( (bytesForMantissa)*8-1 );
    }
  }
}



void toolbox::multiprecision::decomposeIntoFourVariants(
  double   value,
  char     exponent[4],
  int      mantissa[4],
  double   error[4]
) {
  assertion(value==value);
  assertion1( sizeof(int)*8>=32, sizeof(int) );

  // We may not use 7 though we use seven out of eight bits for the first byte.
  // If we shift by seven, we can end up with the highest byte set for
  // 0.0155759, e.g.
  int shiftExponent = 6;

  const long int sign = value < 0.0 ? -1 : 1;

  if (sign<0) {
    value = -value;
  }

  int           integerExponent;
  const double  significand          = std::frexp(value , &integerExponent);

  for (int i=0; i<4; i++) {
    const double shiftMantissa    = std::pow( 2.0,shiftExponent );

    if (integerExponent-shiftExponent >= std::numeric_limits<char>::min() ) {
      exponent[i]  = static_cast<char>( integerExponent-shiftExponent );
      mantissa[i]  = static_cast<int>( std::round(significand*shiftMantissa) );
    }
    else {
      exponent[i] = 0;
      mantissa[i] = 0;
    }
    error[i]     = std::abs( std::ldexp(mantissa[i],exponent[i]) - value );

    assertion5( mantissa[i]>=0, value, mantissa[i], exponent[i], error[i], sign );

    std::bitset< sizeof(int)*8 >*  mantissaAsBitset = reinterpret_cast<std::bitset< sizeof(int)*8 >*>( &(mantissa[i]) );

    #ifdef Asserts
    for (int j=(i+1)*8-1; j<static_cast<int>( sizeof(int) )*8; j++) {
      assertion9(
        !(*mantissaAsBitset)[j],
        *mantissaAsBitset, value, static_cast<int>( exponent[i] ), mantissa[i], error[i], i, j, significand, integerExponent
      );
    }
    #endif

    if (sign<0) {
      assertion ( (*mantissaAsBitset)[ (i+1)*8-1 ]==false );
      mantissaAsBitset->flip( (i+1)*8-1 );
    }

    shiftExponent+=8;
  }
}


double toolbox::multiprecision::compose(
  char         exponent,
  long int     mantissa,
  int          bytesUsed
) {
  assertion( bytesUsed>=1 );

  std::bitset<64>*  mantissaAsBitset = reinterpret_cast<std::bitset<64>*>( &mantissa );

  if ( (*mantissaAsBitset)[ bytesUsed*8-1 ] ) {
    mantissaAsBitset->flip( bytesUsed*8-1 );
    mantissa = -mantissa;
  }

  const double doubleMantissa = mantissa;
  const int    intExponent    = exponent;

  return std::ldexp(doubleMantissa,intExponent);
}


