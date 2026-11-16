#include <gtest/gtest.h>

#include <dx2/beam.hpp>
#include <dx2/beam_ops.hpp>
#include <dx2/crystal.hpp>
#include <dx2/detector.hpp>
#include <dx2/experiment.hpp>
#include <dx2/experiment_list.hpp>
#include <dx2/goniometer.hpp>
#include <dx2/imagesequence.hpp>
#include <dx2/scan.hpp>
#include <nlohmann/json.hpp>

json gonio_json = {
    {"axes",
     {{1.0, -0.0025, 0.0056}, {-0.006, -0.0264, -0.9996}, {1.0, 0.0, 0.0}}},
    {"angles", {0.0, 0.0, 0.0}},
    {"names", {"phi", "chi", "omega"}},
    {"scan_axis", 2}};

TEST(ExperimentListTest, ExperimentListCoreFeatures) {

  MonochromaticBeam beam(0.8);
  Panel panel(100, {500, 600}, {0.075, 0.075}, {3000, 4000}, "x", "-y", 1.0,
              0.01);
  std::vector<Panel> panels;
  panels.push_back(panel);
  Detector det(panels);
  Scan scan({1, 100}, {0.0, 0.1});
  Goniometer gonio(gonio_json);
  std::string filename = "test.h5";
  ImageSequence iseq(filename);
  std::shared_ptr<Beam> beamptr = std::make_shared<Beam>(beam);
  std::shared_ptr<Detector> detptr = std::make_shared<Detector>(det);
  std::shared_ptr<Scan> scanptr = std::make_shared<Scan>(scan);
  std::shared_ptr<Goniometer> gonptr = std::make_shared<Goniometer>(gonio);
  std::shared_ptr<ImageSequence> iseqptr =
      std::make_shared<ImageSequence>(iseq);

  Experiment::Models models(beamptr, detptr, scanptr, gonptr, iseqptr);

  Experiment e1(models, "1");
  ExperimentList elist;
  elist.append(e1);
  ASSERT_EQ(elist.size(), 1);
  const auto &expt = elist[0];

  EXPECT_EQ(expt.identifier(), "1");

  EXPECT_FALSE(expt.has_crystal());

  json elist_json = elist.to_json();

  ExperimentList elist2 = ExperimentList(elist_json);
  const auto &expt2 = elist2[0];

  EXPECT_EQ(expt2.identifier(), "1");
  EXPECT_EQ(beam_ops::require_monochromatic(expt2.beam()).get_wavelength(),
            0.8); // check the round trip for the beam model.

  // Build an experiment list with two experiments with shared models (e.g.
  // detector)

  Experiment e2(models, "2");
  elist.append(e2);

  ASSERT_EQ(elist.size(), 2);

  EXPECT_EQ(&elist[0].detector(), &elist[1].detector());

  // now test selecting by identifier

  auto selected = elist.select(std::vector<std::string>{
      "1",
  });
  ASSERT_EQ(selected.size(), 1);
  EXPECT_EQ(selected[0].identifier(), "1");

  // now test select by index
  auto selected2 = elist.select(std::vector<std::size_t>{1});
  ASSERT_EQ(selected2.size(), 1);
  EXPECT_EQ(selected2[0].identifier(), "2");

  // test selecting preserves shared models.

  auto selected3 = elist.select(std::vector<std::string>{"1", "2"});
  ASSERT_EQ(selected3.size(), 2);
  EXPECT_EQ(&selected3[0].detector(), &selected3[1].detector());
}