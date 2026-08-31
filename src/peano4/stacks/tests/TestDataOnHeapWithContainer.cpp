// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnHeapWithContainer.h"

#include "peano4/stacks/StackWithCustomSerialisation.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Request.h"

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(
  const std::size_t size,
  const DoFType     value
) {
  _container.reserve(size);
  for (std::size_t entryNumber = 0; entryNumber < size; entryNumber++) {
    _container.push_back(new DoFType(value));
  }
}

peano4::stacks::tests::TestDataOnHeapWithContainer::~TestDataOnHeapWithContainer() noexcept { clear(); }

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(
  const peano4::stacks::tests::TestDataOnHeapWithContainer& other
) {
  clone(other);
}

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(
  peano4::stacks::tests::TestDataOnHeapWithContainer&& other
) noexcept :
  _container(std::move(other._container)) {
  other._container.clear();
}

peano4::stacks::tests::TestDataOnHeapWithContainer& peano4::stacks::tests::TestDataOnHeapWithContainer::operator=(
  peano4::stacks::tests::TestDataOnHeapWithContainer&& other
) noexcept {
  if (this != &other) {
    clear();
    _container = std::move(other._container);
    other._container.clear(); // TODO: Should not be required. Check that the other's container is empty.
  }
  return *this;
}

peano4::stacks::tests::TestDataOnHeapWithContainer::DoFType peano4::stacks::tests::TestDataOnHeapWithContainer::at(
  const int index
) const {
  assertion(index >= 0);
  assertion(index < size());
  return *(_container[static_cast<std::size_t>(index)]);
}

int peano4::stacks::tests::TestDataOnHeapWithContainer::size() const { return static_cast<int>(_container.size()); }

void peano4::stacks::tests::TestDataOnHeapWithContainer::clear() {
  for (DoFType* entry : _container) {
    delete entry;
  }
  _container.clear();
}

void peano4::stacks::tests::TestDataOnHeapWithContainer::clone(
  const peano4::stacks::tests::TestDataOnHeapWithContainer& other
) {
  if (this == &other) {
    return;
  }

  clear();

  _container.reserve(other._container.size());
  for (const DoFType* entry : other._container) {
    _container.push_back(entry != nullptr ? new DoFType(*entry) : nullptr);
  }
}

void peano4::stacks::tests::TestDataOnHeapWithContainer::startSend(
  peano4::stacks::Stack<peano4::stacks::tests::TestDataOnHeapWithContainer>& stack,
  [[maybe_unused]] const peano4::stacks::SendReceiveContext                  context,
  const MPI_Comm                                                             comm
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>&>(
      stack
    );

  const int numberOfElements = static_cast<int>(customStack.size());
  assertion(numberOfElements >= 0, numberOfElements);

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  for (int elementNumber = 0; elementNumber < numberOfElements; ++elementNumber) {
    const peano4::stacks::tests::TestDataOnHeapWithContainer& element    = stack[elementNumber];
    const auto                                                localCount = static_cast<int>(element._container.size());
    customStack.countBuffer[elementNumber]                               = localCount;
    for (const DoFType* entry : element._container) {
      customStack.dataBuffer.push_back(entry != nullptr ? *entry : 0);
    }
  }

  const int totalValues = static_cast<int>(customStack.dataBuffer.size());

  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = tarch::mpi::isend(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }

  if (totalValues > 0) {
    [[maybe_unused]] const int result = tarch::mpi::isend(
      customStack.dataBuffer.data(),
      static_cast<int>(totalValues * sizeof(DoFType)),
      MPI_BYTE,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiDataRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

void peano4::stacks::tests::TestDataOnHeapWithContainer::startReceive(
  peano4::stacks::Stack<peano4::stacks::tests::TestDataOnHeapWithContainer>& stack,
  const peano4::stacks::SendReceiveContext                                   context,
  const MPI_Comm                                                             comm,
  const int                                                                  numberOfElements
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>&>(
      stack
    );

  assertion(numberOfElements >= 0, numberOfElements);
  assertion(numberOfElements == static_cast<int>(customStack.size()), numberOfElements, customStack.size());

  customStack.mpiComm    = comm;
  customStack.mpiContext = context;

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  if (numberOfElements > 0) {
    [[maybe_unused]] const int result = tarch::mpi::irecv(
      customStack.countBuffer.data(),
      numberOfElements,
      MPI_INT,
      customStack.getActiveIORank(),
      customStack.getActiveIOTag(),
      comm,
      &customStack.mpiCountRequest
    );
    assertion(result == MPI_SUCCESS);
  }
}

bool peano4::stacks::tests::TestDataOnHeapWithContainer::tryToFinishSendOrReceive(
  peano4::stacks::Stack<peano4::stacks::tests::TestDataOnHeapWithContainer>& stack
) {
  auto& customStack =
    static_cast<peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>&>(
      stack
    );
  bool allDone = true;

  if (customStack.getIOMode() == peano4::stacks::IOMode::MPIReceive) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int transferComplete = 0;
      tarch::mpi::test(&customStack.mpiCountRequest, &transferComplete, MPI_STATUS_IGNORE);
      if (transferComplete) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;

        const int totalValues = std::accumulate(customStack.countBuffer.begin(), customStack.countBuffer.end(), 0);
        assertion(totalValues >= 0);
        customStack.dataBuffer.resize(totalValues);

        if (totalValues > 0) {
          [[maybe_unused]] const int result = tarch::mpi::irecv(
            customStack.dataBuffer.data(),
            static_cast<int>(totalValues * sizeof(DoFType)),
            MPI_BYTE,
            customStack.getActiveIORank(),
            customStack.getActiveIOTag(),
            customStack.mpiComm,
            &customStack.mpiDataRequest
          );
          assertion(result == MPI_SUCCESS);
          allDone = false;
        }
      } else {
        allDone = false;
      }
    }

    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int transferComplete = 0;
      tarch::mpi::test(&customStack.mpiDataRequest, &transferComplete, MPI_STATUS_IGNORE);
      if (transferComplete) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;

        const int numberOfElements = static_cast<int>(customStack.size());
        assertion(
          numberOfElements == static_cast<int>(customStack.countBuffer.size()),
          numberOfElements,
          customStack.countBuffer.size()
        );

        std::size_t offset = 0;
        for (int elementNumber = 0; elementNumber < numberOfElements; ++elementNumber) {
          const int localCount = customStack.countBuffer[elementNumber];
          assertion(localCount >= 0, localCount);

          peano4::stacks::tests::TestDataOnHeapWithContainer& element = stack[elementNumber];

          // The number of values per element changes between messages, so the
          // container drops its entries before it takes the incoming ones.
          element.clear();

          element._container.reserve(static_cast<std::size_t>(localCount));
          for (int valueNumber = 0; valueNumber < localCount; ++valueNumber) {
            assertion(offset < customStack.dataBuffer.size(), offset, customStack.dataBuffer.size());
            element._container.push_back(new DoFType(customStack.dataBuffer[offset]));
            ++offset;
          }
        }
        assertion(offset == customStack.dataBuffer.size(), offset);
      } else {
        allDone = false;
      }
    }
  } else if (customStack.getIOMode() == peano4::stacks::IOMode::MPISend) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int transferComplete = 0;
      tarch::mpi::test(&customStack.mpiCountRequest, &transferComplete, MPI_STATUS_IGNORE);
      if (transferComplete) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }

    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int transferComplete = 0;
      tarch::mpi::test(&customStack.mpiDataRequest, &transferComplete, MPI_STATUS_IGNORE);
      if (transferComplete) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }
  }

  return allDone;
}
