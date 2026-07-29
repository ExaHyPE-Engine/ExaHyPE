// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnHeapWithContainer.h"

#include "peano4/stacks/StackWithCustomSerialisation.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Request.h"

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(std::size_t size, DoFType value) {
  _container.reserve(size);
  for (std::size_t i = 0; i < size; i++) {
    _container.push_back(new DoFType(value));
  }
}

peano4::stacks::tests::TestDataOnHeapWithContainer::~TestDataOnHeapWithContainer() noexcept { clear(); }

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(
  const TestDataOnHeapWithContainer& other
) {
  clone(other);
}

peano4::stacks::tests::TestDataOnHeapWithContainer::TestDataOnHeapWithContainer(
  TestDataOnHeapWithContainer&& other
) noexcept :
  _container(std::move(other._container)) {
  other._container.clear();
}

peano4::stacks::tests::TestDataOnHeapWithContainer&
peano4::stacks::tests::TestDataOnHeapWithContainer::operator=(TestDataOnHeapWithContainer&& other) noexcept {
  if (this != &other) {
    clear();
    _container = std::move(other._container);
    other._container.clear(); // TODO: Should not be required. Check that the other's container is empty.
  }
  return *this;
}

peano4::stacks::tests::TestDataOnHeapWithContainer::DoFType
peano4::stacks::tests::TestDataOnHeapWithContainer::at(int index) const {
  assertion(index >= 0);
  assertion(index < size());
  return *(_container[static_cast<std::size_t>(index)]);
}

int peano4::stacks::tests::TestDataOnHeapWithContainer::size() const { return static_cast<int>(_container.size()); }

void peano4::stacks::tests::TestDataOnHeapWithContainer::clear() {
  for (DoFType* p : _container) {
    delete p;
  }
  _container.clear();
}

void peano4::stacks::tests::TestDataOnHeapWithContainer::clone(const TestDataOnHeapWithContainer& other) {
  if (this == &other) {
    return;
  }

  clear();

  // Deep copy from other
  _container.reserve(other._container.size());
  for (DoFType* p : other._container) {
    _container.push_back(p ? new DoFType(*p) : nullptr);
  }
}

void peano4::stacks::tests::TestDataOnHeapWithContainer::startSend(
  Stack<TestDataOnHeapWithContainer>&       stack,
  [[maybe_unused]] const SendReceiveContext context,
  MPI_Comm                                  comm
) {
  auto& customStack = static_cast<StackWithCustomSerialisation<TestDataOnHeapWithContainer>&>(stack);

  const int numberOfElements = static_cast<int>(customStack.size());
  assertion(numberOfElements >= 0, numberOfElements);

  customStack.countBuffer.assign(numberOfElements, 0);
  customStack.dataBuffer.clear();

  for (int i = 0; i < numberOfElements; ++i) {
    const TestDataOnHeapWithContainer& element    = stack[i];
    const int                          localCount = static_cast<int>(element._container.size());
    customStack.countBuffer[i]                    = localCount;
    for (DoFType* p : element._container) {
      customStack.dataBuffer.push_back(p ? *p : 0);
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
  Stack<TestDataOnHeapWithContainer>& stack,
  const SendReceiveContext            context,
  MPI_Comm                            comm,
  int                                 numberOfElements
) {
  auto& customStack = static_cast<StackWithCustomSerialisation<TestDataOnHeapWithContainer>&>(stack);

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
  Stack<TestDataOnHeapWithContainer>& stack
) {
  auto& customStack = static_cast<StackWithCustomSerialisation<TestDataOnHeapWithContainer>&>(stack);
  bool  allDone     = true;

  if (customStack.getIOMode() == IOMode::MPIReceive) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      tarch::mpi::test(&customStack.mpiCountRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
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
      int flag = 0;
      tarch::mpi::test(&customStack.mpiDataRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;

        const int numberOfElements = static_cast<int>(customStack.size());
        assertion(
          numberOfElements == static_cast<int>(customStack.countBuffer.size()),
          numberOfElements,
          customStack.countBuffer.size()
        );

        std::size_t offset = 0;
        for (int i = 0; i < numberOfElements; ++i) {
          const int localCount = customStack.countBuffer[i];
          assertion(localCount >= 0, localCount);

          TestDataOnHeapWithContainer& element = stack[i];

          // Wipe old heap data
          // We want to clean the container's data as the number of data per element can change
          // between messages
          element.clear();

          element._container.reserve(static_cast<std::size_t>(localCount));
          for (int j = 0; j < localCount; ++j) {
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
  } else if (customStack.getIOMode() == IOMode::MPISend) {
    if (customStack.mpiCountRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      tarch::mpi::test(&customStack.mpiCountRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiCountRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }

    if (customStack.mpiDataRequest != MPI_REQUEST_NULL) {
      int flag = 0;
      tarch::mpi::test(&customStack.mpiDataRequest, &flag, MPI_STATUS_IGNORE);
      if (flag) {
        customStack.mpiDataRequest = MPI_REQUEST_NULL;
      } else {
        allDone = false;
      }
    }
  }

  return allDone;
}
