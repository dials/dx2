#include <dx2/h5/h5read_processed.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <hdf5.h>
#include <string>
#include <vector>

// Test fixture for HDF5 read tests
class HDF5ReadTest : public ::testing::Test {
protected:
  std::filesystem::path test_file_path;

  void SetUp() override {
    // Set the test file path (assumes the tests directory as the working
    // directory)
    test_file_path = std::filesystem::current_path() / "data/cut_strong.refl";

    // Create the empty dataset if it does not exist
    create_empty_dataset(test_file_path, "/dials/processing/empty_dataset");
  }

  void create_empty_dataset(const std::string &filename,
                            const std::string &dataset_path) {
    // Open the HDF5 file
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (file < 0) {
      throw std::runtime_error("Error: Unable to open file: " + filename);
    }

    // Check if the dataset exists
    hid_t dataset = H5Dopen(file, dataset_path.c_str(), H5P_DEFAULT);
    if (dataset >= 0) {
      H5Dclose(dataset);
      H5Fclose(file);
      return;
    }

    // Create the empty dataset
    hsize_t dims[1] = {0}; // Zero elements
    hid_t dataspace = H5Screate_simple(1, dims, NULL);
    dataset = H5Dcreate(file, dataset_path.c_str(), H5T_NATIVE_DOUBLE,
                        dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
      H5Sclose(dataspace);
      H5Fclose(file);
      throw std::runtime_error("Error: Unable to create empty dataset: " +
                               dataset_path);
    }

    // Close handles
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Fclose(file);
  }
};

// --------------- read_array_from_h5_file TESTS ---------------
#pragma region read_array_from_h5_file tests

// Test reading double array and validating shape
TEST_F(HDF5ReadTest, ReadDoubleArrayFromH5) {
  std::string array_name = "/dials/processing/group_0/xyzobs.px.value";

  H5Data<double> h5_data;
  read_array_from_h5_file(test_file_path, array_name, h5_data);

  EXPECT_FALSE(h5_data.data.empty());
  EXPECT_EQ(h5_data.shape.size(), 2);
  EXPECT_EQ(h5_data.shape[1], 3);

  // Check a specific value
  double expected_value = 528.86470588235295;
  EXPECT_EQ(h5_data.data[10], expected_value);
}

// Test reading size_t array and validating shape
TEST_F(HDF5ReadTest, ReadSizeTArrayFromH5) {
  std::string flags_name = "/dials/processing/group_0/flags";

  H5Data<std::size_t> h5_data;
  read_array_from_h5_file(test_file_path, flags_name, h5_data);

  EXPECT_FALSE(h5_data.data.empty());
  EXPECT_EQ(h5_data.shape.size(), 1);

  // Check a specific value
  std::size_t expected_flag_value = 32;
  EXPECT_EQ(h5_data.data[5], expected_flag_value);
}

// Test reading from a non-existent file
TEST_F(HDF5ReadTest, ReadFromNonExistentFile) {
  std::string invalid_file = "invalid_file.h5";
  std::string dataset_name = "/some/dataset";

  H5Data<double> h5_data;
  EXPECT_THROW(read_array_from_h5_file(invalid_file, dataset_name, h5_data),
               std::runtime_error);
}

// Test reading a non-existent dataset
TEST_F(HDF5ReadTest, ReadNonExistentDataset) {
  std::string invalid_dataset = "/this/does/not/exist";

  H5Data<double> h5_data;
  EXPECT_THROW(
      read_array_from_h5_file(test_file_path, invalid_dataset, h5_data),
      std::runtime_error);
}

// Test reading an empty dataset
TEST_F(HDF5ReadTest, ReadEmptyDataset) {
  std::string empty_dataset = "/dials/processing/empty_dataset";

  H5Data<double> h5_data;
  read_array_from_h5_file(test_file_path, empty_dataset, h5_data);

  EXPECT_TRUE(h5_data.data.empty())
      << "Expected an empty vector for empty dataset.";
}

// Test reading a multi-dimensional dataset
TEST_F(HDF5ReadTest, ReadMultiDimensionalArrayFromH5) {
  std::string dataset_name = "/dials/processing/group_0/xyzobs.px.value";

  H5Data<double> h5_data;
  read_array_from_h5_file(test_file_path, dataset_name, h5_data);

  EXPECT_EQ(h5_data.shape.size(), 2);
  EXPECT_EQ(h5_data.shape[1], 3); // Ensure the last dimension is 3
}

// Test data type mismatch
TEST_F(HDF5ReadTest, ReadWithIncorrectType) {
  std::string dataset = "/dials/processing/group_0/xyzobs.px.value";

  // Try to read a double dataset as int (should fail)
  H5Data<int> h5_data;
  EXPECT_THROW(read_array_from_h5_file(test_file_path, dataset, h5_data),
               std::runtime_error);
}

#pragma endregion read_array_from_h5_file tests
