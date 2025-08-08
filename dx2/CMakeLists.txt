add_library(
    dx2
    reflection.cxx
    h5/h5read_processed.cxx
    h5/h5write.cxx
    beam.cxx
)
target_link_libraries(
    dx2
    PUBLIC
    Eigen3::Eigen
    fmt::fmt
    nlohmann_json::nlohmann_json
    hdf5::hdf5
    gemmi::gemmi_cpp
)

target_include_directories(
    dx2 PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${mdspan_SOURCE_DIR}/include>
)
