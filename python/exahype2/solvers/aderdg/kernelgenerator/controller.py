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
# Controller of the kernel generator
#
# @note
# requires python3


import os
import copy
import subprocess
import errno
import time

from .configuration import Configuration
from .argumentParser import ArgumentParser

from .models import *


class Controller:
    """Main Controller

    Read the input from the public API, validate them and generate a base
    context for the models.

    Use generateCode() to run the models with the base context.

    Can generate gemms with generateGemms(outputFile, matmulconfig), will be done
    automatically when using generateCode().
    """

    def __init__(self, inputConfig=None):
        """Initialize the base config from the command line inputs"""
        if inputConfig == None:
            args = ArgumentParser.parseArgs()
        else:
            ArgumentParser.validateInputConfig(inputConfig)
            args = inputConfig

        self.commandLine = ArgumentParser.buildCommandLineFromConfig(args)

        # Generate the base config from the args input
        self.config = {
            "kernelType": args["kernelType"],
            "pathToOptKernel": args["pathToOptKernel"],
            "pathToOutputDirectory": os.path.join(
                args["pathToApplication"], args["pathToOptKernel"]
            ),  # os.path.join(Configuration.pathToExaHyPERoot, args["pathToApplication"], args["pathToOptKernel"]),
            "solverName": args["solverName"],
            "codeNamespace": args["namespace"],
            "tempVarsOnStack": args["tempVarsOnStack"],
            "architecture": args["architecture"],
            "useLibxsmm": args["useLibxsmm"]
            if "useLibxsmm" in args
            else Configuration.useLibxsmm,
            "pathToLibxsmmGemmGenerator": Configuration.pathToLibxsmmGemmGenerator,
            "useBLIS": args["useBLIS"]
            if "useBLIS" in args
            else Configuration.useBLIS,
            "useEigen": args["useEigen"]
            if "useEigen" in args
            else Configuration.useEigen,
            "useLibxsmmJIT": args["useLibxsmmJIT"]
            if "useLibxsmmJIT" in args
            else Configuration.useLibxsmmJIT,
            "runtimeDebug": Configuration.runtimeDebug,  # for debug
        }

        if self.config["kernelType"] == "aderdg":
            self.config.update(
                {
                    "numerics": args["numerics"],
                    "nVar": args["numberOfVariables"],
                    "nPar": args["numberOfParameters"],
                    "nData": args["numberOfVariables"] + args["numberOfParameters"],
                    "nDof": (args["order"]) + 1,
                    "nDim": args["dimension"],
                    "useFlux": (args["useFlux"] or args["useFluxVect"])
                    or args["useViscousFlux"],
                    "useFluxVect": args["useFluxVect"],
                    "useViscousFlux": args["useViscousFlux"],
                    "useNCP": (args["useNCP"] or args["useNCPVect"]),
                    "useNCPVect": args["useNCPVect"],
                    "useSource": (
                        args["useSource"]
                        or args["useSourceVect"]
                        or args["useFusedSource"]
                        or args["useFusedSourceVect"]
                    ),
                    "useSourceVect": args["useSourceVect"],
                    "useFusedSource": (
                        args["useFusedSource"] or args["useFusedSourceVect"]
                    ),
                    "useFusedSourceVect": args["useFusedSourceVect"],
                    "nPointSources": args["usePointSources"],
                    "usePointSources": args["usePointSources"] > 0,
                    "useMaterialParam": (
                        args["useMaterialParam"] or args["useMaterialParamVect"]
                    ),
                    "useMaterialParamVect": args["useMaterialParamVect"],
                    "quadratureType": (
                        "Gauss-Lobatto" if args["useGaussLobatto"] else "Gauss-Legendre"
                    ),
                    "useCERKGuess": args["useCERKGuess"],
                    "useSplitCK": args["useSplitCK"],
                    "useVectPDE": args["useVectPDE"],
                    "predictorRecompute": args["predictorRecompute"],
                    "initialGuess": "mixedPicard",  # TODO JMG put as proper toolkit arg
                    # "initialGuess"          : "default" #TODO JMG put as proper toolkit arg
                    "predictorComputePrecisions": args["predictorComputePrecisions"]
                    if "predictorComputePrecisions" in args
                    else "double",
                    "predictorStoragePrecision": args["predictorStoragePrecision"]
                    if "predictorStoragePrecision" in args
                    else "double",
                    "correctorComputePrecision": args["correctorComputePrecision"]
                    if "correctorComputePrecision" in args
                    else "double",
                    "correctorStoragePrecision": args["correctorStoragePrecision"]
                    if "correctorStoragePrecision" in args
                    else "double",
                  "usePicardIterationSplitPrecisions": True
                  if "usePicardIterationSplitPrecisions" in args and args["usePicardIterationSplitPrecisions"]
                  else False,
                  "picardIterationPrecision": args["picardIterationPrecision"]
                  if "picardIterationPrecision" in args
                  else ""
                }
            )
            self.config["computePrecisions"] = self.config["predictorComputePrecisions"][:]
            if self.config["correctorComputePrecision"] not in self.config["computePrecisions"]:
                self.config["computePrecisions"].append(self.config["correctorComputePrecision"])
            if self.config["usePicardIterationSplitPrecisions"] and self.config["picardIterationPrecision"] not in self.config["computePrecisions"]:
                self.config["computePrecisions"].append(self.config["picardIterationPrecision"])
            self.config["useSourceOrNCP"] = (
                self.config["useSource"] or self.config["useNCP"]
            )
        elif self.config["kernelType"] == "limiter":
            self.config.update(
                {
                    "nVar": args["numberOfVariables"],
                    "nPar": args["numberOfParameters"],
                    "nData": args["numberOfVariables"] + args["numberOfParameters"],
                    "nDof": (args["order"]) + 1,
                    "nDofLim": args["limPatchSize"]
                    if args["limPatchSize"] >= 0
                    else 2 * args["order"] + 1,
                    "nDim": args["dimension"],
                    "nObs": args["numberOfObservable"],
                    "ghostLayerWidth": args["ghostLayerWidth"],
                    "quadratureType": (
                        "Gauss-Lobatto" if args["useGaussLobatto"] else "Gauss-Legendre"
                    ),
                }
            )
            self.config["computePrecisions"] = ["double"]
        elif self.config["kernelType"] == "fv":
            self.config.update(
                {
                    "nVar": args["numberOfVariables"],
                    "nPar": args["numberOfParameters"],
                    "nData": args["numberOfVariables"] + args["numberOfParameters"],
                    "nDof": args["patchSize"],
                    "nDim": args["dimension"],
                    "useFlux": args["useFlux"] or args["useViscousFlux"],
                    "useViscousFlux": args["useViscousFlux"],
                    "useNCP": args["useNCP"],
                    "useSource": args["useSource"] or args["useFusedSource"],
                    "useFusedSource": args["useFusedSource"],
                    "useRobustDiagLim": args["useRobustDiagLim"],
                    "nPointSources": args["usePointSources"],
                    "usePointSources": args["usePointSources"] >= 0,
                    "useMaterialParam": args["useMaterialParam"],
                    "finiteVolumesType": args["finiteVolumesType"],
                    "ghostLayerWidth": 2,  # hard coded musclhancock value
                    "slopeLimiter": args["slopeLimiter"],
                }
            )
            self.config["useSourceOrNCP"] = (
                self.config["useSource"] or self.config["useNCP"]
            )

        self.validateConfig(Configuration.simdWidth.keys())
        self.config["vectSize"] = Configuration.simdWidth[
            self.config["architecture"]
        ]  # only initialize once architecture has been validated
        self.config["alignmentSize"] = Configuration.alignmentPerArchitectures[
            self.config["architecture"]
        ]  # only initialize once architecture has been validated
        self.baseContext = (
            self.generateBaseContext()
        )  # default context build from config
        self.gemmList = (
            []
        )  # list to store the name of all generated gemms (used for gemmsCPPModel)

    def validateConfig(self, validArchitectures):
        """Ensure the configuration fit some constraint, raise errors if not"""
        if not (self.config["architecture"] in validArchitectures):
            raise ValueError(
                "Architecture not recognized. Available architecture: "
                + str(validArchitectures)
            )
        if self.config["kernelType"] == "aderdg":
            if not (
                self.config["numerics"] == "linear"
                or self.config["numerics"] == "nonlinear"
            ):
                raise ValueError("numerics has to be linear or nonlinear")
            if self.config["nVar"] < 0:
                raise ValueError("Number of variables must be >=0 ")
            if self.config["nPar"] < 0:
                raise ValueError("Number of parameters must be >= 0")
            if self.config["nDim"] < 2 or self.config["nDim"] > 3:
                raise ValueError("Number of dimensions must be 2 or 3")
            if (
                self.config["nDof"] < 1 or self.config["nDof"] > 15 + 1
            ):  # nDof = order+1
                raise ValueError("Order has to be between 0 and 15 (inclusive)")

            for precision in self.config["computePrecisions"]:
              if precision not in Configuration.validPrecisions:
                raise ValueError(
                    "One of the chosen computation precisions was not a valid choice, you have specified: " + precision
                    + ", valid choices are: " + ", ".join(Configuration.validPrecisions.keys())
                )
            if self.config["predictorStoragePrecision"] not in Configuration.validPrecisions:
              raise ValueError(
                "One of the chosen storage precisions was not a valid choice, valid choices are: "
                + ", ".join(Configuration.validPrecisions.keys())
              )
        if self.config["kernelType"] == "fv":
            if self.config["finiteVolumesType"] != "musclhancock":
                raise ValueError("Only musclhancock scheme is supported")

    def printConfig(self):
        print(self.config)

    def generateBaseContext(self):
        """Generate a base context for the models from the config (use hard copy)"""
        context = copy.copy(self.config)
        context["nVarPad"] = self.getSizeWithPadding(context["nVar"])
        context["nParPad"] = self.getSizeWithPadding(context["nPar"])
        context["nDataPad"] = self.getSizeWithPadding(context["nData"])
        context["nDofPad"] = self.getSizeWithPadding(context["nDof"])
        context["nDof3D"] = 1 if context["nDim"] == 2 else context["nDof"]
        context["solverHeader"] = (
            "../../../../" + context["solverName"].split("::")[-1] + ".h"
        )
        context["codeNamespaceList"] = context["codeNamespace"].split("::")
        context["guardNamespace"] = "_".join(context["codeNamespaceList"]).upper()
        if self.config["kernelType"] == "limiter":
            context["nDofLimPad"] = self.getSizeWithPadding(context["nDofLim"])
            context["nDofLim3D"] = 1 if context["nDim"] == 2 else context["nDofLim"]
            context["ghostLayerWidth3D"] = (
                0 if context["nDim"] == 2 else context["ghostLayerWidth"]
            )
        elif self.config["kernelType"] == "aderdg":
            context["isLinear"] = context["numerics"] == "linear"
            context["useVectPDEs"] = (
                context["useFluxVect"] or True
            )  # TODO JMG add other vect
        elif self.config["kernelType"] == "fv":
            context["ghostLayerWidth3D"] = (
                0 if context["nDim"] == 2 else context["ghostLayerWidth"]
            )
            context["nDofG"] = context["ghostLayerWidth"] * 2 + context["nDof"]
            context["nDofG3D"] = 1 if context["nDim"] == 2 else context["nDofG"]
        return context

    def getSizeWithPadding(self, sizeWithoutPadding):
        """Return the size of the input with the architecture specific padding added"""
        return self.config["vectSize"] * int(
            (sizeWithoutPadding + (self.config["vectSize"] - 1))
            / self.config["vectSize"]
        )

    def getPadSize(self, sizeWithoutPadding):
        """Return the size of padding required for its input"""
        return self.getSizeWithPadding(sizeWithoutPadding) - sizeWithoutPadding

    def generateCode(self):
        """Main method: call the models to generate the code"""

        # create directory for output files if not existing
        try:
            os.makedirs(self.config["pathToOutputDirectory"])
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise

        # remove all .cpp, .cpph, .c and .h files (we are in append mode!)
        for fileName in os.listdir(self.config["pathToOutputDirectory"]):
            _, ext = os.path.splitext(fileName)
            if ext in [".cpp", ".cpph", ".c", ".h"]:
                os.remove(self.config["pathToOutputDirectory"] + "/" + fileName)

        # run the models new files

        self.runModel(
            "kernelsHeader", kernelsHeaderModel.KernelsHeaderModel(self.baseContext)
        )
        self.runModel(
            "configurationParameters",
            configurationParametersModel.ConfigurationParametersModel(self.baseContext),
        )

        if self.config["kernelType"] in ["aderdg", "limiter"]:
            self.runModel(
                "quadrature", quadratureModel.QuadratureModel(self.baseContext, self)
            )

        if self.config["kernelType"] == "aderdg":
            self.runModel("converter", converterModel.ConverterModel(self.baseContext))
            self.runModel(
                "amrRoutines", amrRoutinesModel.AMRRoutinesModel(self.baseContext, self)
            )
            self.runModel(
                "deltaDistribution",
                deltaDistributionModel.DeltaDistributionModel(self.baseContext),
            )
            self.runModel(
                "faceIntegral", faceIntegralModel.FaceIntegralModel(self.baseContext)
            )
            self.runModel(
                "fusedSTPVI",
                fusedSpaceTimePredictorVolumeIntegralModel.FusedSpaceTimePredictorVolumeIntegralModel(
                    self.baseContext, self
                ),
            )
            self.runModel(
                "matrixUtils", matrixUtilsModel.MatrixUtilsModel(self.baseContext)
            )
            self.runModel("dgMatrix", dgMatrixModel.DGMatrixModel(self.baseContext))
            self.runModel(
                "solutionUpdate",
                solutionUpdateModel.SolutionUpdateModel(self.baseContext),
            )
            self.runModel(
                "surfaceIntegral",
                surfaceIntegralModel.SurfaceIntegralModel(self.baseContext),
            )

        if self.config["kernelType"] == "limiter":
            self.runModel("limiter", limiterModel.LimiterModel(self.baseContext, self))

        if self.config["kernelType"] == "fv":
            self.runModel(
                "ghostLayerFilling",
                fvGhostLayerFillingModel.FVGhostLayerFillingModel(self.baseContext),
            )
            self.runModel(
                "ghostLayerFillingAtBoundary",
                fvGhostLayerFillingAtBoundaryModel.FVGhostLayerFillingAtBoundaryModel(
                    self.baseContext
                ),
            )
            self.runModel(
                "boundaryLayerExtraction",
                fvBoundaryLayerExtractionModel.FVBoundaryLayerExtractionModel(
                    self.baseContext
                ),
            )
            self.runModel(
                "solutionUpdate",
                fvSolutionUpdateModel.FVSolutionUpdateModel(self.baseContext, self),
            )

        if self.config["kernelType"] in ["aderdg", "fv"]:
            self.runModel(
                "boundaryConditions",
                boundaryConditionsModel.BoundaryConditionsModel(self.baseContext),
            )
            self.runModel(
                "stableTimeStepSize",
                stableTimeStepSizeModel.StableTimeStepSizeModel(self.baseContext),
            )
            self.runModel(
                "adjustSolution",
                adjustSolutionModel.AdjustSolutionModel(self.baseContext),
            )
            self.runModel("riemannSolver", riemannModel.RiemannModel(self.baseContext))

        ## must be run only after all gemm's configurations have been generated
        gemmsContext = copy.copy(self.baseContext)
        gemmsContext["gemmList"] = self.gemmList
        self.runModel("gemmsCPP", gemmsCPPModel.GemmsCPPModel(gemmsContext))

    def runModel(self, name, model):
        """Run the given model and if debug then print runtime"""
        start = time.perf_counter()
        model.generateCode()
        if self.config["runtimeDebug"]:
            t = time.perf_counter() - start
            print(name + ": " + str(value))

    def generateGemms(self, outputFileName, matmulConfigList):
        """Generate the gemms with the given config list using LIBXSMM"""
        for matmul in matmulConfigList:
            for current_precision in matmul.precision:
                # add the gemm name to the list of generated gemm
                self.gemmList.append((matmul.baseroutinename, current_precision))
                # for plain assembly code (rather than inline assembly) choose dense_asm
                commandLineArguments = (
                    " "
                    + "dense"
                    + " "
                    + os.path.join(self.config["pathToOutputDirectory"], outputFileName)
                    + " "
                    + self.config["codeNamespace"]
                    + "::"
                    + matmul.baseroutinename
                    + " "
                    + str(matmul.M)
                    + " "
                    + str(matmul.N)
                    + " "
                    + str(matmul.K)
                    + " "
                    + str(matmul.LDA)
                    + " "
                    + str(matmul.LDB)
                    + " "
                    + str(matmul.LDC)
                    + " "
                    + str(matmul.alpha)
                    + " "
                    + str(matmul.beta)
                    + " "
                    + str(matmul.alignment_A)
                    + " "
                    + str(matmul.alignment_C)
                    + " "
                    + self.config["architecture"]
                    + " "
                    + matmul.prefetchStrategy
                    + " "
                    + current_precision
                )  # SP (single-precision), DP (double-precision) or l16 (only dense)
                bashCommand = (
                    self.config["pathToLibxsmmGemmGenerator"] + commandLineArguments
                )
                subprocess.call(bashCommand.split())
