# Only generate the pass definitions - no dialect or operations
set(LLVM_TARGET_DEFINITIONS ExahypePasses.td)
mlir_tablegen(ExahypePasses.h.inc --gen-pass-decls)
add_public_tablegen_target(MLIRExahypePassesIncGen)
