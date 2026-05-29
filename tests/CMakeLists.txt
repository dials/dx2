include(GoogleTest)

add_executable(test_crystal test_crystal.cxx)
target_link_libraries(test_crystal GTest::gtest_main dx2 Eigen3::Eigen nlohmann_json::nlohmann_json gemmi::gemmi_cpp)

add_executable(test_beam test_beam.cxx)
target_link_libraries(test_beam GTest::gtest_main dx2 Eigen3::Eigen nlohmann_json::nlohmann_json)

add_executable(test_beam_ops test_beam_ops.cxx)
target_link_libraries(test_beam_ops GTest::gtest_main dx2 Eigen3::Eigen nlohmann_json::nlohmann_json)

add_executable(test_detector_attenuations test_detector_attenuations.cxx)
target_link_libraries(test_detector_attenuations GTest::gtest_main dx2)

add_executable(test_goniometer test_goniometer.cxx)
target_link_libraries(test_goniometer GTest::gtest_main dx2 nlohmann_json::nlohmann_json)

add_executable(test_write_h5_array test_write_h5_array.cxx)
target_link_libraries(test_write_h5_array GTest::gtest_main dx2 hdf5::hdf5)

add_compile_definitions(
    TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data"
) # Test data dir used in the two tests below.

add_executable(test_read_h5_array test_read_h5_array.cxx)
target_link_libraries(test_read_h5_array GTest::gtest_main dx2 hdf5::hdf5)

add_executable(test_reflection_table test_reflection_table.cxx)
target_link_libraries(test_reflection_table GTest::gtest_main dx2)

set(DX2_TESTS
    test_crystal
    test_beam
    test_beam_ops
    test_detector_attenuations
    test_goniometer
    test_write_h5_array
    test_read_h5_array
    test_reflection_table
)

foreach(t ${DX2_TESTS})
    gtest_discover_tests(${t} PROPERTIES LABELS dx2tests)
endforeach()
