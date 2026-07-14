// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::stacks {
  enum class IOMode { None, MPISend, MPIReceive };
} // namespace peano4::stacks

std::string toString(peano4::stacks::IOMode mode);
