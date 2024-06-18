# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Controller import Controller

def generate_aderdg_kernels(
    namespace,
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
        "pathToApplication": "kernels/" + solverName,
        "pathToOutputDirectory": "kernels/" + solverName,
        "solverName": namespace + solverName,
        "namespace": "kernels::" + solverName,
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
            "usePicardIterationSplitPrecisions": ( precomputePicardPrecision if 
              precomputePicardPrecision!=False and (len(predictorComputePrecisions)!=1 or precomputePicardPrecision!=predictorComputePrecisions[0])
              else False),
            "picardIterationPrecision": precomputePicardPrecision
        }
    )

    control = Controller(InputConfig)
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
        "pathToApplication": "kernels/limiter/fv/",
        "pathToOutputDirectory": "kernels/limiter/fv/",
        "solverName": solverName,
        "namespace": "kernels::limiter",
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
