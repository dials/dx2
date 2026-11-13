#include <cassert>
#include <memory>

#include <dx2/beam.hpp>
#include <dx2/crystal.hpp>
#include <dx2/detector.hpp>
#include <dx2/experiment.hpp>
#include <dx2/goniometer.hpp>
#include <dx2/imagesequence.hpp>
#include <dx2/scan.hpp>
#include <gtest/gtest.h>

TEST(ModelTests, ExperimentTest) {

  Experiment::Models models(
    std::make_shared<Beam>(),
    std::make_shared<Detector>(),
    std::make_shared<Scan>(),
    std::make_shared<Goniometer>(),
    std::make_shared<ImageSequence>());

  Experiment expt(models);

  // Required models exist
  (void)expt.beam();
  (void)expt.detector();
  (void)expt.scan();
  (void)expt.goniometer();
  (void)expt.imagesequence();

  // Crystal initially absent
  assert(!expt.has_crystal());

  bool threw = false;
  try {
    (void)expt.crystal();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  assert(threw);

  // Attach crystal
  auto crystal = std::make_shared<Crystal>();
  expt.set_crystal(crystal);

  assert(expt.has_crystal());

  // Shared crystal
  Experiment expt2(models);
  expt2.set_crystal(crystal);

  assert(&expt.crystal() == &expt2.crystal());

  // Identifier generated automatically
  assert(!expt.identifier().empty());

}