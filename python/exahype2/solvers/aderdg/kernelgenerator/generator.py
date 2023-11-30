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
# Starting point of the kernel generator
#
# @note
# requires python3

import sys
import os
from .controller import Controller

# numerics must be linear or nonlinear


def generate_aderdg_kernels(
    solverName,
    numerics,
    numberOfVariables,
    numberOfParameters,
    order,
    dimensions,
    useFlux=False,
    useFluxVect=False,
    useNCP=False,
    useNCPVect=False,
    useSource=False,
    useSourceVect=False,
    usePointSources=False,
    useMaterialParam=False,
    useMaterialParamVect=False,
    useGaussLobatto=False,
    useVectPDE=False,
    useLibxsmm=False,
    useBLIS=False,
    useEigen=False,
    useLibxsmmJIT=False,
    architecture="noarch",
    useCERKGuess=False,
    useSplitCK=False,
    predictorRecompute=False,
    predictorComputePrecisions=["double"],
    predictorStoragePrecision="double",
    correctorComputePrecision="double",
    correctorStoragePrecision="double",
    precomputePicardPrecision=False
):
    InputConfig = {
        "kernelType": "aderdg",
        "pathToOptKernel": ".",
        "pathToApplication": "generated/kernels/aderdg/" + numerics,
        "pathToOutputDirectory": "generated/kernels/aderdg/" + numerics,
        "solverName": solverName,
        "namespace": "generated::kernels::AderDG::" + numerics,
        "useLibxsmm": useLibxsmm,
        "useBLIS": useBLIS,
        "useEigen": useEigen,
        "useLibxsmmJIT": useLibxsmmJIT,
        "architecture": architecture,
    }

    InputConfig.update(
        {
            "numerics": numerics,
            "numberOfVariables": numberOfVariables,
            "numberOfParameters": numberOfParameters,
            "order": order,
            "dimension": dimensions,
            "useFlux": useFlux or useFluxVect,
            "useFluxVect": useFluxVect,
            "useViscousFlux": "",  # false
            "useNCP": useNCP or useNCPVect,
            "useNCPVect": useNCPVect,
            "useSource": useSource or useSourceVect,
            "useSourceVect": useSourceVect,
            "useFusedSource": "",
            "useFusedSourceVect": "",
            "nPointSources": usePointSources,
            "usePointSources": usePointSources,
            "useMaterialParam": useMaterialParam or useMaterialParamVect,
            "useMaterialParamVect": useMaterialParamVect,
            "useGaussLobatto": useGaussLobatto,
            "useCERKGuess": useCERKGuess,
            "useSplitCK": useSplitCK,
            "useVectPDE": useVectPDE,
            "predictorRecompute": predictorRecompute,
            "initialGuess": "mixedPicard",
            "predictorComputePrecisions": predictorComputePrecisions,
            "predictorStoragePrecision": predictorStoragePrecision,
            "correctorComputePrecision": correctorComputePrecision,
            "correctorStoragePrecision": correctorStoragePrecision,
            "usePicardIterationSplitPrecisions": precomputePicardPrecision if precomputePicardPrecision!=False else False,
            "picardIterationPrecision": precomputePicardPrecision
        }
    )

    control = Controller(InputConfig)
    control.generateCode()


def generate_fv_kernels():
    raise Exception("FV kernel generation not yet implemented")
    control = Controller()
    control.generateCode()


def generate_limiter_kernels(
    solverName,
    numberOfVariables,
    numberOfParameters,
    order,
    dimensions,
    limPatchSize=-1,
    numberOfObservable=None,
    ghostLayerWidth=1,
    useGaussLobatto=False,
    useLibxsmm=False,
    useBLIS=False,
    useEigen=False,
    useLibxsmmJIT=False,
    architecture="noarch",
):
    
    if numberOfObservable is None:
        numberOfObservable=numberOfVariables+numberOfParameters

    InputConfig = {
        "kernelType": "limiter",
        "pathToOptKernel": ".",
        "pathToApplication": "generated/kernels/limiter/fv/",
        "pathToOutputDirectory": "generated/kernels/limiter/fv/",
        "solverName": solverName,
        "namespace": "generated::kernels::limiter",
        "useLibxsmm": useLibxsmm,
        "useBLIS": useBLIS,
        "useEigen": useEigen,
        "useLibxsmmJIT": useLibxsmmJIT,
        "architecture": architecture,
    }

    InputConfig.update(
        {
          "numberOfVariables": numberOfVariables,
          "numberOfParameters": numberOfParameters,
          "order": order,
          "limPatchSize": limPatchSize,
          "dimension": dimensions,
          "numberOfObservable": numberOfObservable,
          "ghostLayerWidth": ghostLayerWidth,
          "useGaussLobatto": useGaussLobatto
        }
    )

    control = Controller(InputConfig)
    control.generateCode()
