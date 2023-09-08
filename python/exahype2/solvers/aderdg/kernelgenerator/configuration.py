#!/bin/env python
##
# @file This file is part of the ExaHyPE project.
# @author ExaHyPE Group (exahype@lists.lrz.de)
#
# @section LICENSE
#
# Copyright (c) 2016  http://exahype.eu
# All rights reserved.
#
# The project has received funding from the European Union's Horizon
# 2020 research and innovation programme under grant agreement
# No 671698. For copyrights and licensing, please consult the webpage.
#
# Released under the BSD 3 Open Source License.
# For the full license text, see LICENSE.txt
#
#
# @section DESCRIPTION
#
# Static configuration parameters for the module
#
# @note
# requires python3

import sys
import os

class Configuration:

    ######################################
    ###### Configuration parameters ######
    ######################################
    # Change them if required

    # path to the root of ExaHyPe from this file
    pathToExaHyPERoot          = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", "..", ".."))

    # path to the gemm generator from this file
    pathToLibxsmmGemmGenerator = os.path.abspath(os.path.join(pathToExaHyPERoot, "submodules", "libxsmm", "bin", "libxsmm_gemm_generator"))
    
    # path to jinja2
    pathToJinja2               = os.path.abspath(os.path.join(pathToExaHyPERoot, "submodules", "jinja", "src"))
    
    # path to markupsafe
    pathToMarkupsafe           = os.path.abspath(os.path.join(pathToExaHyPERoot, "submodules", "markupsafe", "src"))
    
    # simd size of the accepted architectures
    simdWidth = { "noarch" : 1,
                  "wsm"    : 2,
                  "snb"    : 4,
                  "hsw"    : 4,
                  "knc"    : 8,
                  "knl"    : 8,
                  "skx"    : 8
                }
    
    # alignment size of the accepted architectures
    
    alignmentPerArchitectures  = {
        "noarch" : 16,
        "wsm"    : 16,
        "snb"    : 32, 
        "hsw"    : 32, 
        "knc"    : 64, 
        "knl"    : 64,
        "skx"    : 64,
    }

    # accepted precisions

    validPrecisions = [
        "float",
        "double",
        "long double",
        "std::float16_t",
        "std::bfloat16_t"
    ]


    # set to false to use standard loops instead of libxsmm
#    useLibxsmm = True;
    useLibxsmm = False;

    useBLIS = False;

    useEigen = False;

    useLibxsmmJIT = False;

    # set to true to print models runtime
    runtimeDebug = False;



def checkPythonVersion():
    """check version. Python 3.6 required"""
    requiredVersion = (3,6)
    currentVersion  = sys.version_info
    if(requiredVersion > currentVersion):
        sys.exit("Requires Python 3.6 or newer. Abort.")



def checkDependencies():
    """check all dependencies are reachable from the configuration path"""
    # Check jinja
    sys.path.insert(1, Configuration.pathToJinja2)
    sys.path.insert(1, Configuration.pathToMarkupsafe)
    import jinja2
    # Remove added path
    sys.path.remove(Configuration.pathToJinja2)
    sys.path.remove(Configuration.pathToMarkupsafe)
