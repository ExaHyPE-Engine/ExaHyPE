// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/instrumentation/CommandLineSink.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Redirects stream into a string buffer for the lifetime of the object.
   */
  class StreamCapture final {
  public:
    explicit StreamCapture(std::ostream& stream) :
      _stream(&stream),
      _originalBuffer(stream.rdbuf(_buffer.rdbuf())) {}
    StreamCapture(const StreamCapture& other)            = delete;
    StreamCapture(StreamCapture&& other)                 = delete;
    StreamCapture& operator=(const StreamCapture& other) = delete;
    StreamCapture& operator=(StreamCapture&& other)      = delete;
    ~StreamCapture() { _stream->rdbuf(_originalBuffer); }

    [[nodiscard]] std::string getText() const { return _buffer.str(); }

  private:
    std::ostringstream _buffer{};
    std::ostream*      _stream{nullptr};
    std::streambuf*    _originalBuffer{nullptr};
  };
} // namespace

UNIT_TEST("A plain message reaches the standard output or the standard error stream without escape codes") {
  GIVEN("The command line sink") {
    using Style = tarch::instrumentation::CommandLineSink::TerminalStyle;
    auto& sink  = tarch::instrumentation::CommandLineSink::getInstance();

    WHEN("The sink writes a plain line to the standard output and one to the standard error stream") {
      std::string standardOutput;
      std::string standardError;
      {
        const StreamCapture outputCapture(std::cout);
        const StreamCapture errorCapture(std::cerr);
        sink.writeToStdOut("output line\n", true, Style::Plain);
        sink.writeToStdErr("error line\n", true, Style::Plain);
        standardOutput = outputCapture.getText();
        standardError  = errorCapture.getText();
      }

      THEN("Each stream receives its line unchanged") {
        VALIDATE_EQUALS(standardOutput, std::string("output line\n"));
        VALIDATE_EQUALS(standardError, std::string("error line\n"));
      }
    }
  }
}
