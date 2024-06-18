# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
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
        "std::bfloat16_t",
        "_Float16",
        "__bf16"
    ]


    # set to false to use standard loops instead of libxsmm
#    useLibxsmm = True;
    useLibxsmm = False

    useBLIS = False

    useEigen = False

    useLibxsmmJIT = False

    # set to true to print models runtime
    runtimeDebug = False
