#include "ParticleSpecies.h"

#include <limits>

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"

tarch::multicore::BooleanSemaphore swift2::ParticleSpecies::_semaphore;
tarch::logging::Log                swift2::ParticleSpecies::_log("swift2::ParticleSpecies");
