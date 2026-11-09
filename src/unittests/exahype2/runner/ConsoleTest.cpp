// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/runner/Console.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * A framed report line spans the text and two frame columns on either side.
   */
  constexpr std::size_t FrameWidth = 97;
} // namespace

UNIT_TEST("A duration shows its two most significant units, the lower one padded with zeros to two digits") {
  GIVEN("The durations of 45, 63, 3661, 90000 and zero seconds") {
    constexpr double SecondsOnly       = 45.0;
    constexpr double MinutesAndSeconds = 63.0;
    constexpr double HoursAndMinutes   = 3661.0;
    constexpr double DaysAndHours      = 90000.0;
    constexpr double Zero              = 0.0;

    WHEN("The console renders the durations") {
      const std::string secondsOnlyText       = exahype2::runner::Format::getDuration(SecondsOnly);
      const std::string minutesAndSecondsText = exahype2::runner::Format::getDuration(MinutesAndSeconds);
      const std::string hoursAndMinutesText   = exahype2::runner::Format::getDuration(HoursAndMinutes);
      const std::string daysAndHoursText      = exahype2::runner::Format::getDuration(DaysAndHours);
      const std::string zeroText              = exahype2::runner::Format::getDuration(Zero);

      THEN("The texts read 45s, 1m 03s, 1h 01m, 1d 01h and 0s") {
        VALIDATE_EQUALS(secondsOnlyText, std::string("45s"));
        VALIDATE_EQUALS(minutesAndSecondsText, std::string("1m 03s"));
        VALIDATE_EQUALS(hoursAndMinutesText, std::string("1h 01m"));
        VALIDATE_EQUALS(daysAndHoursText, std::string("1d 01h"));
        VALIDATE_EQUALS(zeroText, std::string("0s"));
      }
    }
  }
}

UNIT_TEST("A negative duration renders as zero seconds and a duration beyond 999 days as the bounded text") {
  GIVEN("The durations of minus five seconds, 999 days, 1000 days and 1e18 seconds") {
    constexpr double Negative         = -5.0;
    constexpr double DaysAtTheCap     = 999.0 * 24.0 * 3600.0;
    constexpr double DaysBeyondTheCap = 1000.0 * 24.0 * 3600.0;
    constexpr double Huge             = 1.0e18;

    WHEN("The console renders the durations") {
      const std::string negativeText         = exahype2::runner::Format::getDuration(Negative);
      const std::string daysAtTheCapText     = exahype2::runner::Format::getDuration(DaysAtTheCap);
      const std::string daysBeyondTheCapText = exahype2::runner::Format::getDuration(DaysBeyondTheCap);
      const std::string hugeText             = exahype2::runner::Format::getDuration(Huge);

      THEN("The texts read 0s, 999d 00h, >999d and >999d") {
        VALIDATE_EQUALS(negativeText, std::string("0s"));
        VALIDATE_EQUALS(daysAtTheCapText, std::string("999d 00h"));
        VALIDATE_EQUALS(daysBeyondTheCapText, std::string(">999d"));
        VALIDATE_EQUALS(hugeText, std::string(">999d"));
      }
    }
  }
}

UNIT_TEST("A grouped number separates every group of three digits by a comma") {
  GIVEN("The numbers 0, 42, 999, 1000 and 1234567") {
    constexpr unsigned long Zero        = 0;
    constexpr unsigned long TwoDigits   = 42;
    constexpr unsigned long ThreeDigits = 999;
    constexpr unsigned long FourDigits  = 1000;
    constexpr unsigned long SevenDigits = 1234567;

    WHEN("The console groups the numbers") {
      const std::string zeroText        = exahype2::runner::Format::getGroupedNumber(Zero);
      const std::string twoDigitsText   = exahype2::runner::Format::getGroupedNumber(TwoDigits);
      const std::string threeDigitsText = exahype2::runner::Format::getGroupedNumber(ThreeDigits);
      const std::string fourDigitsText  = exahype2::runner::Format::getGroupedNumber(FourDigits);
      const std::string sevenDigitsText = exahype2::runner::Format::getGroupedNumber(SevenDigits);

      THEN("The texts read 0, 42, 999, 1,000 and 1,234,567") {
        VALIDATE_EQUALS(zeroText, std::string("0"));
        VALIDATE_EQUALS(twoDigitsText, std::string("42"));
        VALIDATE_EQUALS(threeDigitsText, std::string("999"));
        VALIDATE_EQUALS(fourDigitsText, std::string("1,000"));
        VALIDATE_EQUALS(sevenDigitsText, std::string("1,234,567"));
      }
    }
  }
}

UNIT_TEST("A percentage, a scientific number, a rate and a plain number render in their formats") {
  GIVEN("The percentages 50 and 100, the value 1234.5, two rates per second and the numbers 42 and 2.5") {
    constexpr double      FiftyPercent    = 50.0;
    constexpr double      HundredPercent  = 100.0;
    constexpr double      ScientificValue = 1234.5;
    constexpr double      CellRate        = 1000.0;
    constexpr const char* CellUnit        = "Cell";
    constexpr double      DoFRate         = 2.5e6;
    constexpr const char* DoFUnit         = "DoF";
    constexpr int         Integer         = 42;
    constexpr double      Fraction        = 2.5;

    WHEN("The format functions render every value") {
      const std::string fiftyPercentText   = exahype2::runner::Format::getPercentage(FiftyPercent);
      const std::string hundredPercentText = exahype2::runner::Format::getPercentage(HundredPercent);
      const std::string scientificText     = exahype2::runner::Format::getScientific(ScientificValue);
      const std::string cellRateText       = exahype2::runner::Format::getRate(CellRate, CellUnit);
      const std::string dofRateText        = exahype2::runner::Format::getRate(DoFRate, DoFUnit);
      const std::string integerText        = exahype2::runner::Format::getNumber(Integer);
      const std::string fractionText       = exahype2::runner::Format::getNumber(Fraction);

      THEN("The texts read 50.00 %, 100.00 %, 1.2345e+03, 1.0000e+03 Cell/s, 2.5000e+06 DoF/s, 42 and 2.5") {
        VALIDATE_EQUALS(fiftyPercentText, std::string("50.00 %"));
        VALIDATE_EQUALS(hundredPercentText, std::string("100.00 %"));
        VALIDATE_EQUALS(scientificText, std::string("1.2345e+03"));
        VALIDATE_EQUALS(cellRateText, std::string("1.0000e+03 Cell/s"));
        VALIDATE_EQUALS(dofRateText, std::string("2.5000e+06 DoF/s"));
        VALIDATE_EQUALS(integerText, std::string("42"));
        VALIDATE_EQUALS(fractionText, std::string("2.5"));
      }
    }
  }
}

UNIT_TEST("A wall clock time renders in seconds with four decimals and a memory size in its binary unit") {
  GIVEN("The times 0.05 and 1.5 seconds and the sizes of one kibibyte and three mebibytes") {
    constexpr double             ShortTime      = 0.05;
    constexpr double             LongTime       = 1.5;
    constexpr unsigned long long OneKibibyte    = 1024ULL;
    constexpr unsigned long long ThreeMebibytes = 3ULL * 1024ULL * 1024ULL;

    WHEN("The console renders the times and the sizes") {
      const std::string shortTimeText      = exahype2::runner::Format::getWallClock(ShortTime);
      const std::string longTimeText       = exahype2::runner::Format::getWallClock(LongTime);
      const std::string oneKibibyteText    = exahype2::runner::Format::getMemory(OneKibibyte);
      const std::string threeMebibytesText = exahype2::runner::Format::getMemory(ThreeMebibytes);

      THEN("The texts read (0.0500 s), (1.5000 s), 1.00 KiB and 3.00 MiB") {
        VALIDATE_EQUALS(shortTimeText, std::string("(0.0500 s)"));
        VALIDATE_EQUALS(longTimeText, std::string("(1.5000 s)"));
        VALIDATE_EQUALS(oneKibibyteText, std::string("1.00 KiB"));
        VALIDATE_EQUALS(threeMebibytesText, std::string("3.00 MiB"));
      }
    }
  }
}

UNIT_TEST("The display width counts code points and a multi-byte glyph occupies one column") {
  GIVEN("The empty text, three letters, a lambda, the label lambda max, two middle dots and three block glyphs") {
    const std::string empty;
    const std::string threeLetters  = "abc";
    const std::string lambdaGlyph   = "λ";
    const std::string lambdaMax     = "λ_max";
    const std::string twoMiddleDots = "··";
    const std::string threeBlocks   = "███";

    WHEN("The console measures the display widths") {
      const std::size_t emptyWidth         = exahype2::runner::getDisplayWidth(empty);
      const std::size_t threeLettersWidth  = exahype2::runner::getDisplayWidth(threeLetters);
      const std::size_t lambdaGlyphWidth   = exahype2::runner::getDisplayWidth(lambdaGlyph);
      const std::size_t lambdaMaxWidth     = exahype2::runner::getDisplayWidth(lambdaMax);
      const std::size_t twoMiddleDotsWidth = exahype2::runner::getDisplayWidth(twoMiddleDots);
      const std::size_t threeBlocksWidth   = exahype2::runner::getDisplayWidth(threeBlocks);

      THEN("The widths are zero, three, one, five, two and three columns") {
        VALIDATE_EQUALS(emptyWidth, std::size_t(0));
        VALIDATE_EQUALS(threeLettersWidth, std::size_t(3));
        VALIDATE_EQUALS(lambdaGlyphWidth, std::size_t(1));
        VALIDATE_EQUALS(lambdaMaxWidth, std::size_t(5));
        VALIDATE_EQUALS(twoMiddleDotsWidth, std::size_t(2));
        VALIDATE_EQUALS(threeBlocksWidth, std::size_t(3));
      }
    }
  }
}

UNIT_TEST("A truncation to a display width keeps every glyph whole") {
  GIVEN("Six letters, two letters and five block glyphs") {
    const std::string sixLetters = "abcdef";
    const std::string twoLetters = "ab";
    const std::string fiveBlocks = "█████";

    WHEN("The console truncates the six letters to three columns, the two letters to five and the blocks to two") {
      const std::string truncatedSixLetters = exahype2::runner::truncateToDisplayWidth(sixLetters, 3);
      const std::string truncatedTwoLetters = exahype2::runner::truncateToDisplayWidth(twoLetters, 5);
      const std::string twoBlocks           = exahype2::runner::truncateToDisplayWidth(fiveBlocks, 2);
      const std::size_t twoBlocksWidth      = exahype2::runner::getDisplayWidth(twoBlocks);

      THEN("The texts read abc and ab and the two blocks occupy two columns in six bytes") {
        VALIDATE_EQUALS(truncatedSixLetters, std::string("abc"));
        VALIDATE_EQUALS(truncatedTwoLetters, std::string("ab"));
        VALIDATE_EQUALS(twoBlocksWidth, std::size_t(2));
        VALIDATE_EQUALS(twoBlocks.size(), std::size_t(6));
      }
    }
  }
}

UNIT_TEST("A padding widens a text to the display width with spaces and keeps a wider text") {
  GIVEN("Two letters, a lambda glyph and five letters") {
    const std::string twoLetters  = "ab";
    const std::string lambdaGlyph = "λ";
    const std::string fiveLetters = "abcde";

    WHEN("The console pads the two letters to five columns, the lambda to four and the five letters to three") {
      const std::string paddedTwoLetters       = exahype2::runner::padToWidth(twoLetters, 5);
      const std::string paddedLambdaGlyph      = exahype2::runner::padToWidth(lambdaGlyph, 4);
      const std::string paddedFiveLetters      = exahype2::runner::padToWidth(fiveLetters, 3);
      const std::size_t paddedTwoLettersWidth  = exahype2::runner::getDisplayWidth(paddedTwoLetters);
      const std::size_t paddedLambdaGlyphWidth = exahype2::runner::getDisplayWidth(paddedLambdaGlyph);

      THEN("The padded texts occupy five and four columns and the five letters stay unchanged") {
        VALIDATE_EQUALS(paddedTwoLettersWidth, std::size_t(5));
        VALIDATE_EQUALS(paddedLambdaGlyphWidth, std::size_t(4));
        VALIDATE_EQUALS(paddedFiveLetters, std::string("abcde"));
      }
    }
  }
}

UNIT_TEST("A wrapped line breaks after a separator and every continuation starts at the continuation column") {
  GIVEN("A line of three entries between separators and a line of one entry") {
    const std::string threeEntries = "Label    alpha = 1 · beta = 2 · gamma = 3";
    const std::string oneEntry     = "Label    alpha = 1";

    WHEN("Both lines wrap at the width 24 with the continuation column nine") {
      const auto lines   = exahype2::runner::wrapToDisplayWidth(threeEntries, 24, 9);
      const auto fitting = exahype2::runner::wrapToDisplayWidth(oneEntry, 24, 9);

      THEN("The three entries take one line each with the separator at its end and the one entry fits one line") {
        VALIDATE_EQUALS(lines.size(), std::size_t(3));
        VALIDATE_EQUALS(lines[0], std::string("Label    alpha = 1 ·"));
        VALIDATE_EQUALS(lines[1], std::string("         beta = 2 ·"));
        VALIDATE_EQUALS(lines[2], std::string("         gamma = 3"));
        VALIDATE_EQUALS(fitting.size(), std::size_t(1));
        VALIDATE_EQUALS(fitting[0], std::string("Label    alpha = 1"));
      }
    }
  }
}

UNIT_TEST("A line without a separator breaks at a space and a line without a space breaks at the width") {
  GIVEN("Three words without a separator") {
    const std::string threeWords = "aaaa bbbb cccc";

    WHEN("The words wrap at the width nine with the continuation column two") {
      const auto wrappedWords = exahype2::runner::wrapToDisplayWidth(threeWords, 9, 2);

      THEN("Every word takes one line and every continuation starts at column two") {
        VALIDATE_EQUALS(wrappedWords.size(), std::size_t(3));
        VALIDATE_EQUALS(wrappedWords[0], std::string("aaaa"));
        VALIDATE_EQUALS(wrappedWords[1], std::string("  bbbb"));
        VALIDATE_EQUALS(wrappedWords[2], std::string("  cccc"));
      }
    }
  }

  GIVEN("Ten letters without a space") {
    const std::string tenLetters = "abcdefghij";

    WHEN("The letters wrap at the width four with the continuation column four") {
      const auto wrappedLetters = exahype2::runner::wrapToDisplayWidth(tenLetters, 4, 4);

      THEN("Every line takes four letters and the continuations start at the first column") {
        VALIDATE_EQUALS(wrappedLetters.size(), std::size_t(3));
        VALIDATE_EQUALS(wrappedLetters[0], std::string("abcd"));
        VALIDATE_EQUALS(wrappedLetters[1], std::string("efgh"));
        VALIDATE_EQUALS(wrappedLetters[2], std::string("ij"));
      }
    }
  }
}

UNIT_TEST("A table indents a section title by one level and its entries by two, aligned across sections") {
  GIVEN("A table with one entry and a table with two sections whose labels differ in length") {
    exahype2::runner::Table table;
    table.add("Grid", "Cells", "6,561");
    exahype2::runner::Table twoSections;
    twoSections.add("A", "x", "1");
    twoSections.add("B", "long", "2");

    WHEN("Both tables render") {
      const auto lines   = table.render();
      const auto aligned = twoSections.render();

      THEN("The titles sit at the block indent, the entries one level deeper and the values align across sections") {
        VALIDATE_EQUALS(lines.size(), std::size_t(2));
        VALIDATE_EQUALS(lines[0], std::string("  Grid"));
        VALIDATE_EQUALS(lines[1], std::string("    Cells  6,561"));
        VALIDATE_EQUALS(aligned.size(), std::size_t(4));
        VALIDATE_EQUALS(aligned[0], std::string("  A"));
        VALIDATE_EQUALS(aligned[2], std::string("  B"));
        VALIDATE_EQUALS(aligned[1].find('1'), aligned[3].find('2'));
      }
    }
  }
}

UNIT_TEST("An empty table renders zero lines") {
  GIVEN("An empty table") {
    const exahype2::runner::Table table;

    WHEN("The table renders") {
      const auto lines = table.render();

      THEN("render() returns zero lines") { VALIDATE_EQUALS(lines.size(), std::size_t(0)); }
    }
  }
}

UNIT_TEST("A report frames its lines, merges consecutive rules, drops empty lines and closes with one border") {
  GIVEN("A report that collects its lines") {
    std::vector<std::string> lines;
    exahype2::runner::Report report([&lines](const std::string& line) { lines.push_back(line); });

    WHEN("The report writes a line, two rules, an empty and a second line, closes twice and writes a last line") {
      report.line("Peano");
      report.rule();
      report.rule();
      report.line("");
      report.line("  Grid");
      report.close();
      report.close();
      report.line("Goodbye");
      std::vector<std::size_t> widthOfLine;
      for (const auto& line : lines) {
        widthOfLine.push_back(exahype2::runner::getDisplayWidth(line));
      }

      THEN("The first line opens the frame, one rule and one border follow and the last line opens a new frame") {
        VALIDATE_EQUALS(lines.size(), std::size_t(7));
        for (const std::size_t width : widthOfLine) {
          VALIDATE_EQUALS(width, FrameWidth);
        }
        VALIDATE(lines[0].starts_with("┌─") and lines[0].ends_with("─┐"));
        VALIDATE_EQUALS(lines[1], "│ " + exahype2::runner::padToWidth("Peano", FrameWidth - 4) + " │");
        VALIDATE(lines[2].starts_with("├─") and lines[2].ends_with("─┤"));
        VALIDATE_EQUALS(lines[3], "│ " + exahype2::runner::padToWidth("  Grid", FrameWidth - 4) + " │");
        VALIDATE(lines[4].starts_with("└─") and lines[4].ends_with("─┘"));
        VALIDATE_EQUALS(lines[5], lines[0]);
        VALIDATE_EQUALS(lines[6], "│ " + exahype2::runner::padToWidth("Goodbye", FrameWidth - 4) + " │");
      }
    }
  }
}

UNIT_TEST("A line wider than the frame continues on a framed line at the continuation column") {
  GIVEN("A report that collects its lines and a line of six entries wider than the frame") {
    std::vector<std::string> lines;
    exahype2::runner::Report report([&lines](const std::string& line) { lines.push_back(line); });
    const std::string        wideLine =
      "    Solver  a = 1.0000e+00 · b = 1.0000e+00 · c = 1.0000e+00 · d = 1.0000e+00 · e = 1.0000e+00 · f = 1.0000e+00";

    WHEN("The report writes the line with the continuation column twelve") {
      report.line(wideLine, 12);

      THEN("The first four entries fill the first framed line and the last two continue at column twelve") {
        VALIDATE_EQUALS(lines.size(), std::size_t(3));
        VALIDATE_EQUALS(
          lines[1],
          "│ "
            + exahype2::runner::padToWidth(
              "    Solver  a = 1.0000e+00 · b = 1.0000e+00 · c = 1.0000e+00 · d = 1.0000e+00 ·",
              FrameWidth - 4
            )
            + " │"
        );
        VALIDATE_EQUALS(
          lines[2],
          "│ " + exahype2::runner::padToWidth("            e = 1.0000e+00 · f = 1.0000e+00", FrameWidth - 4) + " │"
        );
      }
    }
  }
}
