//===- Dialects.h - CAPI for dialects -----------------------------*- C -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef EXAHYPE_C_DIALECTS_H
#define EXAHYPE_C_DIALECTS_H

#ifdef BUILD_MLIR_EXAHYPE

#include "mlir-c/IR.h"

#ifdef __cplusplus
extern "C" {
#endif

MLIR_DECLARE_CAPI_DIALECT_REGISTRATION(Exahype, exahype);

#ifdef __cplusplus
}
#endif

#endif // BUILD_MLIR_EXAHYPE
#endif // EXAHYPE_C_DIALECTS_H