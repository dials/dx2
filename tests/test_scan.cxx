#include <dx2/scan.hpp>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json ssx_scan_json = {{"image_range", {1, 1}},
                      {"batch_offset", 0},
                      {"properties",
                       {{"epochs", {0.0}},
                        {"exposure_time", {0.0}},
                        {"oscillation", {0.0}},
                        {"oscillation_width", {0.0}}}},
                      {"valid_image_ranges", json::object()}};

json rotation_scan_json = {{"image_range", {1, 4}},
                           {"batch_offset", 0},
                           {"properties",
                            {{"epochs", {0.0, 0.1, 0.2, 0.3}},
                             {"exposure_time", {0.0, 0.1, 0.2, 0.3}},
                             {"oscillation", {0.0, 0.5, 1.0, 1.5}}}},
                           {"valid_image_ranges", json::object()}};

TEST(ModelTests, ScanTest) {
  // Test loading rotation and SSX scans.

  Scan ssxscan(ssx_scan_json);
  ASSERT_EQ(ssxscan.get_oscillation()[1], 0);

  json ssx_tojson = ssxscan.to_json();
  Scan ssxscan2(ssx_tojson);
  ASSERT_EQ(ssxscan2.get_oscillation()[1], 0);

  Scan rotscan(rotation_scan_json);
  ASSERT_EQ(rotscan.get_oscillation()[1], 0.5);
}
