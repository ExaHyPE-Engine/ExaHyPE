# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class CompileMode(Enum):
    """!
    The compile mode is required in my get_library() routines.
    You can use CompileModes for a list (overview) of all available modes.
    string_to_mode() finally helps you to translate a string representation into an actual modus.
    """

    Debug = 0
    Trace = 1
    Asserts = 2
    Release = 3


"""!
Overview over all compile modes available.
"""
CompileModes = ["Release", "Trace", "Asserts", "Debug"]

"""!
Convert a string into a compile mode
"""


def string_to_mode(mode: CompileMode):
    if mode == CompileModes[0]:
        return CompileMode.Release
    if mode == CompileModes[1]:
        return CompileMode.Trace
    if mode == CompileModes[2]:
        return CompileMode.Asserts
    if mode == CompileModes[3]:
        return CompileMode.Debug
    raise Exception("Compile mode {} not supported".format(mode))


def mode_to_string(mode: CompileMode):
    if mode == CompileMode.Release:
        return CompileModes[0]
    if mode == CompileMode.Trace:
        return CompileModes[1]
    if mode == CompileMode.Asserts:
        return CompileModes[2]
    if mode == CompileMode.Debug:
        return CompileModes[3]
    raise Exception("Compile mode {} not supported".format(mode))
