// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnStack.h"

#include "tarch/accelerator/accelerator.h"

peano4::stacks::tests::TestDataOnStack::TestDataOnStack() { data.fill(0); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(int v) { data.fill(v); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(const TestDataOnStack& other) { clone(other); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(TestDataOnStack&& other) noexcept {
  data = std::move(other.data);
}

peano4::stacks::tests::TestDataOnStack& peano4::stacks::tests::TestDataOnStack::operator=(
  TestDataOnStack&& other
) noexcept {
  if (this != &other) {
    data = std::move(other.data);
  }
  return *this;
}

void peano4::stacks::tests::TestDataOnStack::clone(const TestDataOnStack& other) {
  std::copy(std::begin(other.data), std::end(other.data), std::begin(data));
}

#ifdef PEANO_WITH_MPI
MPI_Datatype peano4::stacks::tests::TestDataOnStack::Datatype = MPI_DATATYPE_NULL;

void peano4::stacks::tests::TestDataOnStack::initDatatype() {
  int errorCode = MPI_Type_contiguous(Cardinality, MPI_INT, &Datatype);
  errorCode += MPI_Type_commit(&Datatype);

  if (errorCode) {
    std::stringstream ss;
    ss << "Error constructing MPI datatype in " << __FILE__ << ":" << __LINE__;
    throw std::runtime_error(ss.str());
  }

  // MPI_Datatype tmp;
  // const int blocklength = Cardinality;
  // MPI_Datatype basetype = MPI_INT;

  // TestDataOnStack probe;
  // MPI_Aint base, addr, disp;

  // MPI_Get_address(&probe, &base);
  // MPI_Get_address(probe.data.data(), &addr);
  // disp = addr - base;

  // MPI_Type_create_struct(1, &blocklength, &disp, &basetype, &tmp);

  // MPI_Aint lb = 0, extent = static_cast<MPI_Aint>(sizeof(TestDataOnStack));
  // MPI_Type_create_resized(tmp, lb, extent, &Datatype);
  // MPI_Type_commit(&Datatype);
  // MPI_Type_free(&tmp);
}

void peano4::stacks::tests::TestDataOnStack::shutdownDatatype() { MPI_Type_free(&Datatype); }

MPI_Datatype peano4::stacks::tests::TestDataOnStack::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getMultiscaleDataExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getGlobalCommunicationDatatype() { return Datatype; }
#endif
