#!/usr/bin/env python3

"""
%prog TESTNAME NAMESPACE [options]

Example: %prog CopyPatch exahype2::fv -o src/exahype2/fv/tests
"""

TEMPLATE_CPP="""
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{TESTNAME}Test.h"

{NAMESPACE}::tests::{TESTNAME}Test::{TESTNAME}Test():
  TestCase ("{NAMESPACE}::tests::{TESTNAME}Test") {{
}}

void {NAMESPACE}::tests::{TESTNAME}Test::run() {{
  testMethod (testMe);
}}

void {NAMESPACE}::tests::{TESTNAME}Test::testMe() {{
   /// Add implementation of testcase here
}}
"""

TEMPLATE_H ="""
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

{FORWARDDECL}

class {NAMESPACE}::tests::{TESTNAME}Test : public tarch::tests::TestCase
{{
private:

  void testMe();

public:

  /**
   * Constructor.
   */
  {TESTNAME}Test();

  /**
   * Destructor, empty.
   */
  virtual ~{TESTNAME}Test() {{}}

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {{}}
}};
"""

def forwardDeclaration(vdict):
    """
    Generate the nested namespace forward declaration
    """
    s=""
    for num, nsp in enumerate(vdict["NAMESPACE"].split("::")):
        s+= num*2*" " + "namespace {} {{\n".format(nsp)

    LEN = len(vdict["NAMESPACE"].split("::"))

    s+= "{}namespace tests {{\n{}class {}Test;\n{}}}\n".format(LEN*"  ", (LEN+1)*"  ", vdict["TESTNAME"], LEN*"  ")

    for num in range(LEN):
        s+= (LEN-num-1)*"  " + "}\n"

    return s


def instantiate(template, vdict): return template.format(**vdict)


if __name__ == "__main__":

    import optparse, os, sys
    op = optparse.OptionParser(usage=__doc__)
    op.add_option("-v", "--debug", dest="DEBUG", action="store_true", default=False, help="Turn on some debug messages")
    op.add_option("-o", dest="OUTDIR", default=None, help="Output directory (default: %default)")
    op.add_option("-f", "--force", dest="FORCE", default=False,action="store_true", help="Allow overwriting of existing files (default: %default)")
    opts, args = op.parse_args()

    if len(args)!=2:
        print("\nERROR: Must have exactly 2 command line arguments, exiting.\n")
        op.print_help()
        sys.exit(1)

    testd = {"TESTNAME":args[0], "NAMESPACE":args[1]}
    testd["FORWARDDECL"]  = forwardDeclaration(testd)

    if opts.OUTDIR is None:
        print("Implementation:\n\n")
        print(instantiate(TEMPLATE_CPP, testd))
        print("Header:\n\n")
        print(instantiate(TEMPLATE_H, testd))

    else:
        fout = os.path.join(opts.OUTDIR, testd["TESTNAME"]+"Test")
        fout_cpp = fout+".cpp"
        fout_h   = fout+".h"
        print("Writing to {} and {}.".format(fout_cpp, fout_h))

        if not opts.FORCE:
            if os.path.exists(fout_cpp) or os.path.exists(fout_h):
                print("\nERROR: output files already exist. Use --force to allow overwriting.\n")
                op.print_help()
                sys.exit(1)


        with open(fout_cpp, "w") as f:
            f.write(instantiate(TEMPLATE_CPP, testd))

        with open(fout_h,   "w") as f:
            f.write(instantiate(TEMPLATE_H,   testd))


    targetfile = "src/{}/UnitTests.cpp".format(args[1].split("::")[0])
    testinclude = "#include {}".format(fout_cpp)
    print("Done. Register the test in {} by adding {}.".format(targetfile, testinclude))
    print("And something like result->addTestCase( new {}::tests::{}Test() );".format(args[1], testd["TESTNAME"]))
