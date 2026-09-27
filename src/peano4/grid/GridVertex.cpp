// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertex.h"

static_assert(
  std::is_standard_layout_v<peano4::grid::GridVertex>,
  "A grid vertex travels between ranks as raw bytes, so its layout has to be standard."
);
static_assert(
  std::is_trivially_copyable_v<peano4::grid::GridVertex>,
  "A grid vertex travels between ranks as raw bytes, so it has to be trivially copyable."
);

std::ostream&
operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex) {
  return (out << vertex.toString());
}

std::string
peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "GridVertex{State=" << static_cast<int>(state) << " (";

  switch (getState()) {
  case State::HangingVertex:
    out << "HangingVertex";
    break;
  case State::Unrefined:
    out << "Unrefined";
    break;
  case State::Refined:
    out << "Refined";
    break;
  }
  out << ")";

  out << ", Level=" << level;
#if PEANO_DEBUG
  out << ", Position=" << x;
#endif
  out << "}";

  return out.str();
}
