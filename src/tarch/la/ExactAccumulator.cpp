// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExactAccumulator.h"

/**
 * Returns the running sum rounded to the nearest double, with ties to even.
 * The limbs hold the exact sum, so the result is the same for every order in
 * which the addends arrived. A result outside the finite double range becomes
 * signed infinity.
 */
double tarch::la::ExactAccumulator::round() const {
  std::int64_t digits[NumberOfLimbs];
  std::int64_t carry = 0;
  for (int limb = 0; limb < NumberOfLimbs; limb++) {
    const std::int64_t total = _limbs[limb] + carry;
    digits[limb]             = total & LimbMask;
    carry                    = total >> BitsPerLimb;
  }
  assertion(carry == 0 or carry == -1);

  const auto isNegative = carry == -1;
  if (isNegative) {
    std::int64_t borrow = 1;
    for (int limb = 0; limb < NumberOfLimbs; limb++) {
      const std::int64_t total = (~digits[limb] & LimbMask) + borrow;
      digits[limb]             = total & LimbMask;
      borrow                   = total >> BitsPerLimb;
    }
  }

  int topLimb = NumberOfLimbs - 1;
  while (topLimb >= 0 and digits[topLimb] == 0) {
    topLimb--;
  }
  if (topLimb < 0) {
    return 0.0;
  }

  const auto topDigit      = static_cast<std::uint64_t>(digits[topLimb]);
  const auto digitBelow    = topLimb >= 1 ? static_cast<std::uint64_t>(digits[topLimb - 1]) : 0;
  const auto digitFurther  = topLimb >= 2 ? static_cast<std::uint64_t>(digits[topLimb - 2]) : 0;
  const auto leadingZeros  = std::countl_zero(topDigit) - BitsPerLimb;
  const auto exponentOfMsb = topLimb * BitsPerLimb + (BitsPerLimb - 1 - leadingZeros) - 1074;
  const auto mantissaBits  = std::min(53, exponentOfMsb + 1075);

  auto remainderIsNonZero = false;
  for (int limb = 0; limb < topLimb - 2; limb++) {
    remainderIsNonZero = remainderIsNonZero or digits[limb] != 0;
  }

  // The top two digits joined hold at least 33 significant bits, so together
  // with one further digit they cover the mantissa, the guard bit, and the
  // start of the sticky range for every possible mantissa width.
  const auto highBits          = (topDigit << BitsPerLimb) | digitBelow;
  const auto significantLength = 2 * BitsPerLimb - leadingZeros;
  const auto dropped           = significantLength - mantissaBits;

  std::uint64_t mantissa;
  bool          guard;
  if (dropped > 0) {
    mantissa = highBits >> dropped;
    guard    = ((highBits >> (dropped - 1)) & 1) != 0;
    remainderIsNonZero =
      remainderIsNonZero or digitFurther != 0 or (highBits & ((std::uint64_t{1} << (dropped - 1)) - 1)) != 0;
  } else {
    // The third digit is partly consumed by the mantissa and the guard bit, so
    // only its remaining lower bits feed the sticky decision.
    mantissa = (highBits << -dropped) | (digitFurther >> (BitsPerLimb + dropped));
    guard    = ((digitFurther >> (BitsPerLimb + dropped - 1)) & 1) != 0;
    remainderIsNonZero =
      remainderIsNonZero or (digitFurther & ((std::uint64_t{1} << (BitsPerLimb + dropped - 1)) - 1)) != 0;
  }
  if (guard and (remainderIsNonZero or (mantissa & 1) != 0)) {
    mantissa++;
  }

  // Assembling the fields keeps a subnormal result exact, as scaling a
  // significand into the subnormal range raises an underflow exception.
  constexpr std::uint64_t InfinityBits         = 0x7ff0000000000000ULL;
  const auto              exponentField        = static_cast<std::uint64_t>(exponentOfMsb - mantissaBits + 1075);
  const auto              magnitudeBits        = (exponentField << 52) + mantissa;
  const auto              roundedMagnitudeBits = std::min(magnitudeBits, InfinityBits);
  const auto              signField            = isNegative ? std::uint64_t{1} << 63 : std::uint64_t{0};
  return std::bit_cast<double>(signField | roundedMagnitudeBits);
}
