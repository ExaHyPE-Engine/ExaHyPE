//===- ExahypePasses.td - Exahype dialect passes -------*- tablegen -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef EXAHYPE_PASS
#define EXAHYPE_PASS

include "mlir/Pass/PassBase.td"

def ExahypeMergeMaxEigenvaluesArray : Pass<"dslhype-merge-maxeigenvalues-array", "ModuleOp"> {
  let summary = "Detect and merge maxEigenvalues array patterns";
  let description = [{
    Detects and merges maxEigenvalues arrays including their outer 
    and inner allocations and associated deallocations
  }];
}

def ExahypeLinearizeNestedMemrefs : Pass<"dslhype-linearize-nested-memrefs", "ModuleOp"> {
  let summary = "Convert nested memref allocations to 2D flat memrefs";
  let description = [{
    Transforms nested memref<?xmemref<?xT>> allocations to flat memref<?x?xT> 
  }];
}

def ExahypeIncludePhysics : Pass<"dslhype-include-physics", "ModuleOp"> {
  let summary = "Merge physics function implementations from external file";
  let description = [{
    Loads physics function definitions from an external MLIR file and merges them
    into the kernel module:
    - Reads physics file specified by 'physics-file' option
    - Parses physics function definitions
    - Replaces physics function declarations with actual implementations
    - Converts memref parameters to !llvm.ptr 
  }];
  
  let options = [
    Option<"physicsFile", "physics-file", "std::string", "\"\"",
      "Path to MLIR file containing physics function implementations (required)">,
    Option<"memSpace", "mem-space", "std::string", "\"\"",
      "Target memory space for device allocations: global, shared or private (optional)">,
    Option<"simplifyKernelSig", "simplify-kernel-signature", "bool", "false",
      "Simplify the kernel function signature for GPU lowering (optional)">
  ];
}

def ExahypeAddFastmath : Pass<"dslhype-add-fastmath", "ModuleOp"> {
  let summary = "Add fastmath<fast> attributes to all arith operations";
  let description = [{
    Adds fastmath<fast> attributes to all relevant floating-point arithmetic 
    operations in the module for optimization
  }];
}


def ExahypeAddOpenMPSchedule : Pass<"exahype-add-openmp-schedule", "ModuleOp"> {
  let summary = "Add a schedule to `omp.wsloop` ops";
  let description = [{
    Adds a schedule modifier to all `omp.wsloop` operations in the module
  }];
}

def ExahypeHostToDeviceMemspace : Pass<"dslhype-host-to-device-memspace", "ModuleOp"> {
  let summary = "Convert host space memrefs and pointers to device space";
  let description = [{
    Converts host memrefs to device memrefs for GPU lowering
    i.e. memref<?xT> to memref<?xT, 1> and !llvm.ptr to !llvm.ptr<1>
  }];

  let options = [
    Option<"memSpace", "mem-space", "std::string", "\"\"",
      "Target memory space for device allocations: global, shared or private (required)">
  ];
}

#endif // EXAHYPE_PASS
