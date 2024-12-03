add_executable(test_example test_example.cxx)
target_link_libraries(test_example GTest::gtest_main)

add_executable(test_make_models test_make_models.cxx)
target_link_libraries(test_make_models GTest::gtest_main dx2 Eigen3::Eigen nlohmann_json::nlohmann_json gemmi::gemmi_cpp)

include(GoogleTest)
gtest_discover_tests(test_example)