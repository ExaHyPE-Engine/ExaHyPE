// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnStack.h"

#include "tarch/accelerator/accelerator.h"

peano4::stacks::tests::TestDataOnStack::TestDataOnStack() { value.fill(0); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(int v) { value.fill(v); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(const TestDataOnStack& other) { clone(other); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(TestDataOnStack&& other) noexcept {
  value = std::move(other.value);
}

peano4::stacks::tests::TestDataOnStack& peano4::stacks::tests::TestDataOnStack::operator=(TestDataOnStack&& other
) noexcept {
  if (this != &other) {
    value = std::move(other.value);
  }
  return *this;
}

void peano4::stacks::tests::TestDataOnStack::clone(const TestDataOnStack& other) {
  std::copy(std::begin(other.value), std::end(other.value), std::begin(value));
}

#ifdef WITH_MPI
MPI_Datatype peano4::stacks::tests::TestDataOnStack::Datatype;

void peano4::stacks::tests::TestDataOnStack::initDatatype() {
  int errorCode = MPI_Type_contiguous(Cardinality, MPI_INT, &Datatype);
  errorCode += MPI_Type_commit(&Datatype);

  if (errorCode) {
    std::stringstream ss;
    ss << "Error constructing MPI datatype in " << __FILE__ << ":" << __LINE__;
    throw std::runtime_error(ss.str());
  }
}

void peano4::stacks::tests::TestDataOnStack::shutdownDatatype() { MPI_Type_free(&Datatype); }

MPI_Datatype peano4::stacks::tests::TestDataOnStack::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getMultiscaleDataExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::stacks::tests::TestDataOnStack::getGlobalCommunicationDatatype() { return Datatype; }
#endif
