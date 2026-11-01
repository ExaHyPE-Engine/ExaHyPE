// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/limiter/SlopeLimiter.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * The forward and the backward difference of one unknown that a limiter receives.
   */
  template <typename Precision>
  struct DifferencePair final {
    Precision forwardDifference{};
    Precision backwardDifference{};
  };

  /**
   * Returns the limited difference of Limiter for the pair.
   */
  template <typename Limiter, typename Precision>
  Precision getLimitedDifferenceOf(const DifferencePair<Precision>& pair) {
    return Limiter::limitedDifference(pair.forwardDifference, pair.backwardDifference);
  }

  /**
   * Returns the number of sample pairs for which Limiter limits the negated differences in swapped
   * order to another value than the negated limited difference.
   */
  template <typename Limiter, typename Precision>
  int countMirroringMismatchesOf() {
    static constexpr Precision Samples[] = {
      Precision(-3.7e5),
      Precision(-2.5),
      Precision(-0.3),
      Precision(-1.0e-7),
      Precision(0),
      Precision(1.0e-7),
      Precision(0.1),
      Precision(0.7),
      Precision(1.7),
      Precision(2.2e5),
    };
    int mismatches = 0;
    for (const Precision forwardDifference : Samples) {
      for (const Precision backwardDifference : Samples) {
        const Precision limitedDifference  = Limiter::limitedDifference(forwardDifference, backwardDifference);
        const Precision mirroredDifference = Limiter::limitedDifference(-backwardDifference, -forwardDifference);
        if (mirroredDifference != -limitedDifference) {
          mismatches++;
        }
      }
    }
    return mismatches;
  }
} // namespace

UNIT_TEST("The zero limiter gives the slope zero") {
  GIVEN("The difference pairs three and one, minus two and five and zero and zero") {
    static constexpr DifferencePair<double> SameSigns{3.0, 1.0};
    static constexpr DifferencePair<double> OppositeSigns{-2.0, 5.0};
    static constexpr DifferencePair<double> Zeros{0.0, 0.0};

    WHEN("The zero limiter limits every pair") {
      const double limitedSameSigns     = getLimitedDifferenceOf<exahype2::fv::limiter::Zero>(SameSigns);
      const double limitedOppositeSigns = getLimitedDifferenceOf<exahype2::fv::limiter::Zero>(OppositeSigns);
      const double limitedZeros         = getLimitedDifferenceOf<exahype2::fv::limiter::Zero>(Zeros);

      THEN("Every limited difference is zero") {
        VALIDATE_NUMERICAL_EQUALS(limitedSameSigns, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedZeros, 0.0);
      }
    }
  }
}

UNIT_TEST("The average limiter takes the mean of both differences whatever their signs") {
  GIVEN("The difference pairs three and one, minus one and two and zero and zero") {
    static constexpr DifferencePair<double> SameSigns{3.0, 1.0};
    static constexpr DifferencePair<double> OppositeSigns{-1.0, 2.0};
    static constexpr DifferencePair<double> Zeros{0.0, 0.0};

    WHEN("The average limiter limits every pair") {
      const double limitedSameSigns     = getLimitedDifferenceOf<exahype2::fv::limiter::Average>(SameSigns);
      const double limitedOppositeSigns = getLimitedDifferenceOf<exahype2::fv::limiter::Average>(OppositeSigns);
      const double limitedZeros         = getLimitedDifferenceOf<exahype2::fv::limiter::Average>(Zeros);

      THEN("The limited differences are two, one half and zero") {
        VALIDATE_NUMERICAL_EQUALS(limitedSameSigns, 2.0);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, 0.5);
        VALIDATE_NUMERICAL_EQUALS(limitedZeros, 0.0);
      }
    }
  }
}

UNIT_TEST("Minmod takes the difference of smaller magnitude if both signs agree and zero otherwise") {
  GIVEN("Pairs of the same sign with the smaller difference forward or backward, of opposite signs and with a zero") {
    static constexpr DifferencePair<double> SmallerBackward{2.0, 1.0};
    static constexpr DifferencePair<double> SmallerForward{1.0, 2.0};
    static constexpr DifferencePair<double> BothNegative{-1.0, -3.0};
    static constexpr DifferencePair<double> OppositeSigns{-1.0, 2.0};
    static constexpr DifferencePair<double> ZeroBackward{2.0, 0.0};
    static constexpr DifferencePair<double> ZeroForward{0.0, 1.0};

    WHEN("Minmod limits every pair") {
      const double limitedSmallerBackward = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(SmallerBackward);
      const double limitedSmallerForward  = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(SmallerForward);
      const double limitedBothNegative    = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(BothNegative);
      const double limitedOppositeSigns   = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(OppositeSigns);
      const double limitedZeroBackward    = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(ZeroBackward);
      const double limitedZeroForward     = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(ZeroForward);

      THEN("The pairs of the same sign give their smaller difference and every other pair zero") {
        VALIDATE_NUMERICAL_EQUALS(limitedSmallerBackward, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedSmallerForward, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedBothNegative, -1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedZeroBackward, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedZeroForward, 0.0);
      }
    }
  }

  GIVEN("The differences 0.3 and 0.7 in both orders") {
    static constexpr DifferencePair<double> Ascending{0.3, 0.7};
    static constexpr DifferencePair<double> Descending{0.7, 0.3};

    WHEN("Minmod limits both pairs") {
      const double limitedAscending  = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(Ascending);
      const double limitedDescending = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(Descending);

      THEN("Both orders give the same limited difference") {
        VALIDATE_NUMERICAL_EQUALS(limitedAscending, limitedDescending);
      }
    }
  }
}

UNIT_TEST("Van Albada limits without a positivity clamp and differences of opposite signs give a small counter-slope") {
  GIVEN("A zero backward difference, the pairs one and one and two and one and differences of opposite signs") {
    static constexpr DifferencePair<double> ZeroBackward{1.0, 0.0};
    static constexpr DifferencePair<double> Equal{1.0, 1.0};
    static constexpr DifferencePair<double> SmallerBackward{2.0, 1.0};
    static constexpr DifferencePair<double> OppositeSigns{-1.0, 2.0};

    WHEN("Van Albada limits every pair") {
      const double limitedZeroBackward    = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(ZeroBackward);
      const double limitedEqual           = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(Equal);
      const double limitedSmallerBackward = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(SmallerBackward);
      const double limitedOppositeSigns   = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(OppositeSigns);

      THEN("The limited differences are zero, one, 1.2 and the counter-slope -0.4") {
        VALIDATE_NUMERICAL_EQUALS(limitedZeroBackward, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedEqual, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedSmallerBackward, 1.2);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, -0.4);
      }
    }
  }

  GIVEN("The differences one and three in both orders") {
    static constexpr DifferencePair<double> Ascending{1.0, 3.0};
    static constexpr DifferencePair<double> Descending{3.0, 1.0};

    WHEN("Van Albada limits both pairs") {
      const double limitedAscending  = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(Ascending);
      const double limitedDescending = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(Descending);

      THEN("Both orders give the same limited difference") {
        VALIDATE_NUMERICAL_EQUALS(limitedAscending, limitedDescending);
      }
    }
  }
}

UNIT_TEST("Superbee takes the larger difference up to twice the smaller one and gives opposite signs the slope zero") {
  GIVEN("A zero backward difference, the forward differences one, two, 0.5 and 0.25 over one and opposite signs") {
    static constexpr DifferencePair<double> ZeroBackward{1.0, 0.0};
    static constexpr DifferencePair<double> Equal{1.0, 1.0};
    static constexpr DifferencePair<double> LargerForward{2.0, 1.0};
    static constexpr DifferencePair<double> HalfForward{0.5, 1.0};
    static constexpr DifferencePair<double> QuarterForward{0.25, 1.0};
    static constexpr DifferencePair<double> OppositeSigns{-1.0, 1.0};

    WHEN("Superbee limits every pair") {
      const double limitedZeroBackward   = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(ZeroBackward);
      const double limitedEqual          = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(Equal);
      const double limitedLargerForward  = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(LargerForward);
      const double limitedHalfForward    = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(HalfForward);
      const double limitedQuarterForward = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(QuarterForward);
      const double limitedOppositeSigns  = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(OppositeSigns);

      THEN("The limited differences are zero, one, two, one, one half and zero") {
        VALIDATE_NUMERICAL_EQUALS(limitedZeroBackward, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedEqual, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedLargerForward, 2.0);
        VALIDATE_NUMERICAL_EQUALS(limitedHalfForward, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedQuarterForward, 0.5);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, 0.0);
      }
    }
  }
}

UNIT_TEST("The monotonised central limiter caps the mean at twice the smaller difference and zeroes opposite signs") {
  GIVEN("A zero backward difference, the forward differences one, four and 0.2 over one and opposite signs") {
    static constexpr DifferencePair<double> ZeroBackward{1.0, 0.0};
    static constexpr DifferencePair<double> Equal{1.0, 1.0};
    static constexpr DifferencePair<double> LargerForward{4.0, 1.0};
    static constexpr DifferencePair<double> SmallerForward{0.2, 1.0};
    static constexpr DifferencePair<double> OppositeSigns{-1.0, 1.0};

    WHEN("The monotonised central limiter limits every pair") {
      const double limitedZeroBackward   = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(ZeroBackward);
      const double limitedEqual          = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(Equal);
      const double limitedLargerForward  = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(LargerForward);
      const double limitedSmallerForward = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(SmallerForward);
      const double limitedOppositeSigns  = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(OppositeSigns);

      THEN("The limited differences are zero, one, two, 0.4 and zero") {
        VALIDATE_NUMERICAL_EQUALS(limitedZeroBackward, 0.0);
        VALIDATE_NUMERICAL_EQUALS(limitedEqual, 1.0);
        VALIDATE_NUMERICAL_EQUALS(limitedLargerForward, 2.0);
        VALIDATE_NUMERICAL_EQUALS(limitedSmallerForward, 0.4);
        VALIDATE_NUMERICAL_EQUALS(limitedOppositeSigns, 0.0);
      }
    }
  }
}

UNIT_TEST("A monotone limiter stays within the total variation bounds") {
  GIVEN("Minmod, Superbee and the monotonised central limiter and eight sample differences from -2.5 to three") {
    using ScalarLimiter                               = double (*)(double forwardDifference, double backwardDifference);
    static constexpr ScalarLimiter MonotoneLimiters[] = {
      static_cast<ScalarLimiter>(&exahype2::fv::limiter::Minmod::limitedDifference<double>),
      static_cast<ScalarLimiter>(&exahype2::fv::limiter::Superbee::limitedDifference<double>),
      static_cast<ScalarLimiter>(&exahype2::fv::limiter::MinmodCentral::limitedDifference<double>),
    };
    static constexpr double Samples[] = {-2.5, -1.0, -0.3, 0.0, 0.4, 1.0, 1.7, 3.0};

    WHEN("Every limiter limits every pair of samples") {
      std::vector<DifferencePair<double>> limitedPairs;
      std::vector<double>                 limitedDifferences;
      for (const ScalarLimiter limiterFunction : MonotoneLimiters) {
        for (const double forwardDifference : Samples) {
          for (const double backwardDifference : Samples) {
            limitedPairs.push_back({forwardDifference, backwardDifference});
            limitedDifferences.push_back(limiterFunction(forwardDifference, backwardDifference));
          }
        }
      }

      THEN("Opposite signs or a zero give zero and any other pair keeps its sign within twice its smaller magnitude") {
        for (std::size_t pair = 0; pair < limitedPairs.size(); pair++) {
          const double forwardDifference  = limitedPairs[pair].forwardDifference;
          const double backwardDifference = limitedPairs[pair].backwardDifference;
          const double limitedDifference  = limitedDifferences[pair];
          if (forwardDifference * backwardDifference <= 0.0) {
            VALIDATE_NUMERICAL_EQUALS(limitedDifference, 0.0);
          } else {
            const double smallerMagnitude = std::min(std::abs(forwardDifference), std::abs(backwardDifference));
            VALIDATE(
              limitedDifference * backwardDifference >= 0.0,
              forwardDifference,
              backwardDifference,
              limitedDifference
            );
            VALIDATE(
              std::abs(limitedDifference) <= 2.0 * smallerMagnitude + 1e-12,
              forwardDifference,
              backwardDifference,
              limitedDifference
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("Every limiter limits single-precision differences in single precision") {
  GIVEN("The single-precision difference pairs three and two, three and one, two and one and four and one") {
    static constexpr DifferencePair<float> ThreeAndTwo{3.0F, 2.0F};
    static constexpr DifferencePair<float> ThreeAndOne{3.0F, 1.0F};
    static constexpr DifferencePair<float> TwoAndOne{2.0F, 1.0F};
    static constexpr DifferencePair<float> FourAndOne{4.0F, 1.0F};

    WHEN("Every limiter limits one of the pairs") {
      const float zero          = getLimitedDifferenceOf<exahype2::fv::limiter::Zero>(ThreeAndTwo);
      const float average       = getLimitedDifferenceOf<exahype2::fv::limiter::Average>(ThreeAndOne);
      const float minmod        = getLimitedDifferenceOf<exahype2::fv::limiter::Minmod>(TwoAndOne);
      const float vanAlbada     = getLimitedDifferenceOf<exahype2::fv::limiter::VanAlbada>(TwoAndOne);
      const float superbee      = getLimitedDifferenceOf<exahype2::fv::limiter::Superbee>(TwoAndOne);
      const float minmodCentral = getLimitedDifferenceOf<exahype2::fv::limiter::MinmodCentral>(FourAndOne);

      THEN("The limited differences are the single-precision values of the double-precision results") {
        VALIDATE_EQUALS(zero, 0.0F);
        VALIDATE_EQUALS(average, 2.0F);
        VALIDATE_EQUALS(minmod, 1.0F);
        VALIDATE_EQUALS(vanAlbada, 1.2F);
        VALIDATE_EQUALS(superbee, 2.0F);
        VALIDATE_EQUALS(minmodCentral, 2.0F);
      }
    }
  }
}

UNIT_TEST("Every limiter returns the negated limited difference for the negated differences in swapped order") {
  GIVEN("Ten differences of both signs between 1e-7 and 3.7e5 in magnitude and every pair of them") {
    WHEN("Every limiter limits every pair and the negated pair in swapped order in double and single precision") {
      const int average             = countMirroringMismatchesOf<exahype2::fv::limiter::Average, double>();
      const int minmod              = countMirroringMismatchesOf<exahype2::fv::limiter::Minmod, double>();
      const int vanAlbada           = countMirroringMismatchesOf<exahype2::fv::limiter::VanAlbada, double>();
      const int superbee            = countMirroringMismatchesOf<exahype2::fv::limiter::Superbee, double>();
      const int minmodCentral       = countMirroringMismatchesOf<exahype2::fv::limiter::MinmodCentral, double>();
      const int singleVanAlbada     = countMirroringMismatchesOf<exahype2::fv::limiter::VanAlbada, float>();
      const int singleSuperbee      = countMirroringMismatchesOf<exahype2::fv::limiter::Superbee, float>();
      const int singleMinmodCentral = countMirroringMismatchesOf<exahype2::fv::limiter::MinmodCentral, float>();

      THEN("Every mirrored pair returns the negated limited difference") {
        VALIDATE_EQUALS(average, 0);
        VALIDATE_EQUALS(minmod, 0);
        VALIDATE_EQUALS(vanAlbada, 0);
        VALIDATE_EQUALS(superbee, 0);
        VALIDATE_EQUALS(minmodCentral, 0);
        VALIDATE_EQUALS(singleVanAlbada, 0);
        VALIDATE_EQUALS(singleSuperbee, 0);
        VALIDATE_EQUALS(singleMinmodCentral, 0);
      }
    }
  }
}
