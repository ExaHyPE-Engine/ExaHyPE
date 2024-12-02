#include "Benchmarks-main.h"

/*
 * This file is empty on purpose
 *
 * Swift generates a main for an SPH simulation run. If the main file does
 * exist already, it will not(!) overwrite it. We don't want an SPH main here,
 * as we have our own main. I originally just wanted to put this benchmark main
 * in here and then commit the file, but I then decided to highlight how we
 * alter the workflow and hence split up the main files in the root directory
 * into an empty file (here) which effectively blocks the creation of an SPH
 * file and the actual driver main.
 *
 */
