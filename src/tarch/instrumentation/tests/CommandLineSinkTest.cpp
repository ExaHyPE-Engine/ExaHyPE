// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommandLineSinkTest.h"

#include "tarch/instrumentation/CommandLineSink.h"

namespace {
  /**
   * Redirects stream into a string buffer for the lifetime of the object.
   */
  class StreamCapture final {
  public:
    explicit StreamCapture(std::ostream& stream) :
      _stream(&stream),
      _originalBuffer(stream.rdbuf(_buffer.rdbuf())) {}
    StreamCapture(const StreamCapture&)            = delete;
    StreamCapture(StreamCapture&&)                 = delete;
    StreamCapture& operator=(const StreamCapture&) = delete;
    StreamCapture& operator=(StreamCapture&&)      = delete;
    ~StreamCapture() { _stream->rdbuf(_originalBuffer); }

    [[nodiscard]] std::string getText() const { return _buffer.str(); }

  private:
    std::ostringstream _buffer{};
    std::ostream*      _stream{nullptr};
    std::streambuf*    _originalBuffer{nullptr};
  };
} // namespace

tarch::instrumentation::tests::CommandLineSinkTest::CommandLineSinkTest() :
  tarch::tests::TestCase("tarch::instrumentation::tests::CommandLineSinkTest") {}

void
tarch::instrumentation::tests::CommandLineSinkTest::run() {
  testMethod(testAPlainMessageReachesItsStreamUnchanged);
}

/**
 * A plain message reaches the standard output or the standard error stream without escape codes.
 */
void
tarch::instrumentation::tests::CommandLineSinkTest::testAPlainMessageReachesItsStreamUnchanged() {
  using Style = tarch::instrumentation::CommandLineSink::TerminalStyle;
  auto& sink  = tarch::instrumentation::CommandLineSink::getInstance();

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

  validateEquals(standardOutput, std::string("output line\n"));
  validateEquals(standardError, std::string("error line\n"));
}
