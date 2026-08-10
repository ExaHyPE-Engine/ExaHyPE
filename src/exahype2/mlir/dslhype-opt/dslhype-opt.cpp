//===- dslhype-opt.cpp ---------------------------------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifdef BUILD_MLIR_EXAHYPE

#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllExtensions.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Target/LLVMIR/Dialect/All.h"          
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

#include "Exahype/ExahypePasses.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);
  mlir::registerAllExtensions(registry);
  mlir::registerAllToLLVMIRTranslations(registry);   
  mlir::registerAllPasses();

  mlir::exahype::registerPasses();

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "Exahype driver\n", registry));
}

#endif