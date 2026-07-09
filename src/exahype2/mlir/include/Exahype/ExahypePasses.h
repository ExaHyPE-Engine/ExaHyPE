//===- ExahypePasses.h - Exahype passes  ------------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef EXAHYPE_EXAHYPEPASSES_H
#define EXAHYPE_EXAHYPEPASSES_H

#ifdef BUILD_MLIR_EXAHYPE

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace exahype {
#define GEN_PASS_DECL 
#include "Exahype/ExahypePasses.h.inc"

#define GEN_PASS_REGISTRATION
#include "Exahype/ExahypePasses.h.inc"
} // namespace exahype
} // namespace mlir

#endif
#endif