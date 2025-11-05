
#include <gtest/gtest.h>
#include <dx2/goniometer.hpp>
#include <nlohmann/json.hpp>

TEST(ModelTests, SingleAxisGoniometerTest) {
    // Test loading single axis goniometer from json.
    // Note the fixed rotation is not strictly valid, but fine for testing serialization.
    std::string json_str = R"({
        "goniometer": [
            {
                "rotation_axis": [1.0, 0.0, 0.0],
                "fixed_rotation": [0.99, 0.01, 0.0, -0.01, 0.99, 0.0, 0.0, 0.0, 1.0],
                "setting_rotation": [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
            }
        ]
    })";
    json j = json::parse(json_str);
    auto goniometer_data = j["goniometer"][0];
    Goniometer gonio(goniometer_data);
    Matrix3d setting = gonio.get_setting_rotation();
    Matrix3d sample = gonio.get_sample_rotation();
    Matrix3d expected_setting{
      {1.0,0.0,0.0},
      {0.0,1.0,0.0},
      {0.0,0.0,1.0}};
    Matrix3d expected_sample{
        {0.99,0.01,0.0},
        {-0.01,0.99,0.0},
        {0.00,0.00,1.0}};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            EXPECT_NEAR(setting(i, j), expected_setting(i, j), 1E-6);
            EXPECT_NEAR(sample(i, j), expected_sample(i, j), 1E-6);
        }
    }
    json output = gonio.to_json();
    std::vector<double> expected_fixed = {0.99,0.01,0.0,-0.01,0.99,0.0,0.0,0.0,1.0};
    for (int i=0;i<9;++i){
        ASSERT_EQ(output["fixed_rotation"][i], expected_fixed[i]);
    }
}

TEST(ModelTests, MultiAxisGoniometerTest) {
    // Test loading multi axis goniometer from json.
    std::string json_str = R"({
       "goniometer": [
            {
            "axes": [
                [1.0, -0.0025, 0.0056],
                [-0.006, -0.0264, -0.9996],
                [1.0, 0.0, 0.0]
            ],
            "angles": [0.0, 5.0, 0.0],
            "names": ["phi", "chi", "omega"],
            "scan_axis": 2
            }
        ]
    })";
    json j = json::parse(json_str);
    auto goniometer_data = j["goniometer"][0];
    Goniometer gonio(goniometer_data);
    Matrix3d setting = gonio.get_setting_rotation();
    Matrix3d sample = gonio.get_sample_rotation();
    Matrix3d expected_setting{
      {1.0,0.0,0.0},
      {0.0,1.0,0.0},
      {0.0,0.0,1.0}};
    Matrix3d expected_sample{
        {0.996195,0.0871244,-0.00227816},
        {-0.0871232,0.996197,0.000623378},
        {0.00232381,-0.000422525,0.999997}};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            EXPECT_NEAR(setting(i, j), expected_setting(i, j), 1E-6);
            EXPECT_NEAR(sample(i, j), expected_sample(i, j), 1E-6);
        }
    }
    json output = gonio.to_json();
    ASSERT_EQ(output["angles"][0], 0.0);
    ASSERT_EQ(output["angles"][1], 5.0);
    ASSERT_EQ(output["angles"][0], 0.0);
}