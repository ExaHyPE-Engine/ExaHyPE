#include "ParticleSet.h"
#include "tests/TestParticle.h"


//
// This C++ file has no real meaning, as the relevant ParticlSet is a template.
// So all we do is to instantiate the ParticlSet with the (minimal)
// TestParticle to ensure that the code compiles properly.
//


namespace {
  toolbox::particles::ParticleSet< toolbox::particles::tests::TestParticle >  TestParticleSet;
}
