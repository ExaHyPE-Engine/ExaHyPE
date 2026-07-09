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

def ExahypeMergeMaxEigenvaluesArray : Pass<"exahype-merge-maxeigenvalues-array", "ModuleOp"> {
  let summary = "Detect and merge maxEigenvalues array patterns";
  let description = [{
    Diagnostic pass that detects and merges maxEigenvalues arrays
    including their outer and inner allocations and deallocations.
  }];
}

#endif // EXAHYPE_PASS
