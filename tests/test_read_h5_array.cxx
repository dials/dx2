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

TEST_F(HDF5ReadTest, ReadAnyDatasetFromH5) {
  std::string dataset_name = "/dials/processing/group_0/xyzobs.px.value";

  auto dataset = read_dataset_from_h5_file(test_file_path, dataset_name);
  ASSERT_NE(dataset, nullptr) << "Dataset is null.";

  if (auto *h5_data_d = dynamic_cast<H5Data<double> *>(dataset.get())) {
    EXPECT_FALSE(h5_data_d->data.empty());
    std::cout << "Dataset is of type double" << std::endl;
  } else if (auto *h5_data_f = dynamic_cast<H5Data<float> *>(dataset.get())) {
    EXPECT_FALSE(h5_data_f->data.empty());
    std::cout << "Dataset is of type float" << std::endl;
  } else if (auto *h5_data_i = dynamic_cast<H5Data<int> *>(dataset.get())) {
    EXPECT_FALSE(h5_data_i->data.empty());
    std::cout << "Dataset is of type int" << std::endl;
  } else {
    FAIL() << "Unknown dataset type!";
  }
}

// Test reading size_t array and validating shape
// TEST_F(HDF5ReadTest, ReadSizeTArrayFromH5) {
//   std::string flags_name = "/dials/processing/group_0/flags";

//   H5Data<std::size_t> h5_data;
//   read_array_from_h5_file(test_file_path, flags_name, h5_data);

//   EXPECT_FALSE(h5_data.data.empty());
//   EXPECT_EQ(h5_data.shape.size(), 1);

//   // Check a specific value
//   std::size_t expected_flag_value = 32;
//   EXPECT_EQ(h5_data.data[5], expected_flag_value);
// }

// // Test reading from a non-existent file
// TEST_F(HDF5ReadTest, ReadFromNonExistentFile) {
//   std::string invalid_file = "invalid_file.h5";
//   std::string dataset_name = "/some/dataset";

//   H5Data<double> h5_data;
//   EXPECT_THROW(read_array_from_h5_file(invalid_file, dataset_name, h5_data),
//                std::runtime_error);
// }

// // Test reading a non-existent dataset
// TEST_F(HDF5ReadTest, ReadNonExistentDataset) {
//   std::string invalid_dataset = "/this/does/not/exist";

//   H5Data<double> h5_data;
//   EXPECT_THROW(
//       read_array_from_h5_file(test_file_path, invalid_dataset, h5_data),
//       std::runtime_error);
// }

// // Test reading an empty dataset
// TEST_F(HDF5ReadTest, ReadEmptyDataset) {
//   std::string empty_dataset = "/dials/processing/empty_dataset";

//   H5Data<double> h5_data;
//   read_array_from_h5_file(test_file_path, empty_dataset, h5_data);

//   EXPECT_TRUE(h5_data.data.empty())
//       << "Expected an empty vector for empty dataset.";
// }

// // Test reading a multi-dimensional dataset
// TEST_F(HDF5ReadTest, ReadMultiDimensionalArrayFromH5) {
//   std::string dataset_name = "/dials/processing/group_0/xyzobs.px.value";

//   H5Data<double> h5_data;
//   read_array_from_h5_file(test_file_path, dataset_name, h5_data);

//   EXPECT_EQ(h5_data.shape.size(), 2);
//   EXPECT_EQ(h5_data.shape[1], 3); // Ensure the last dimension is 3
// }

// // Test data type mismatch
// TEST_F(HDF5ReadTest, ReadWithIncorrectType) {
//   std::string dataset = "/dials/processing/group_0/xyzobs.px.value";

//   // Try to read a double dataset as int (should fail)
//   H5Data<int> h5_data;
//   EXPECT_THROW(read_array_from_h5_file(test_file_path, dataset, h5_data),
//                std::runtime_error);
// }

// #pragma endregion read_array_from_h5_file tests
// // --------------- Discovery tests ---------------
// #pragma region Discovery tests

// TEST_F(HDF5ReadTest, DiscoverH5Datasets) {
//   // Discover all datasets in the test HDF5 file
//   std::vector<std::string> datasets = discover_datasets(test_file_path);

//   // Print the discovered datasets
//   for (size_t i = 0; i < datasets.size(); ++i) {
//     std::cout << "Dataset " << i << ": " << datasets[i] << std::endl;
//   }

//   // Expected dataset paths
//   std::vector<std::string> expected_datasets = {
//       {"/dials/processing/empty_dataset"}, // Empty set added by tests
//       {"/dials/processing/group_0/bbox"},
//       {"/dials/processing/group_0/flags"},
//       {"/dials/processing/group_0/id"},
//       {"/dials/processing/group_0/intensity.sum.value"},
//       {"/dials/processing/group_0/intensity.sum.variance"},
//       {"/dials/processing/group_0/n_signal"},
//       {"/dials/processing/group_0/panel"},
//       {"/dials/processing/group_0/xyzobs.px.value"},
//       {"/dials/processing/group_0/xyzobs.px.variance"},
//       {"/nx_reflections/group_0/bounding_box"},
//       {"/nx_reflections/group_0/det_module"},
//       {"/nx_reflections/group_0/flags"},
//       {"/nx_reflections/group_0/id"},
//       {"/nx_reflections/group_0/int_sum"},
//       {"/nx_reflections/group_0/int_sum_var"},
//       {"/nx_reflections/group_0/observed_frame"},
//       {"/nx_reflections/group_0/observed_frame_var"},
//       {"/nx_reflections/group_0/observed_px_x"},
//       {"/nx_reflections/group_0/observed_px_x_var"},
//       {"/nx_reflections/group_0/observed_px_y"},
//       {"/nx_reflections/group_0/observed_px_y_var"}};

//   // Check that all expected datasets exist
//   for (const auto &expected_path : expected_datasets) {
//     auto it = std::find(datasets.begin(), datasets.end(), expected_path);
//     EXPECT_NE(it, datasets.end()) << "Dataset not found: " << expected_path;
//   }

//   // Ensure no extra datasets are present
//   EXPECT_EQ(datasets.size(), expected_datasets.size());
// }

// TEST_F(HDF5ReadTest, DiscoverH5DatasetsWithNonExistentFile) {
//   std::string invalid_file = "invalid_file.h5";

//   EXPECT_THROW(discover_datasets(invalid_file), std::runtime_error);
// }

// TEST_F(HDF5ReadTest, DiscoverH5DatasetsInGroup) {
//   std::string group_name = "/dials/processing/group_0";

//   // Discover all datasets in the test HDF5 file
//   std::vector<std::string> datasets =
//       get_datasets_in_group(test_file_path, group_name);

//   // Print the discovered datasets
//   for (size_t i = 0; i < datasets.size(); ++i) {
//     std::cout << "Dataset " << i << ": " << datasets[i] << std::endl;
//   }

//   // Expected dataset paths
//   std::vector<std::string> expected_datasets = {
//       {"/dials/processing/group_0/bbox"},
//       {"/dials/processing/group_0/flags"},
//       {"/dials/processing/group_0/id"},
//       {"/dials/processing/group_0/intensity.sum.value"},
//       {"/dials/processing/group_0/intensity.sum.variance"},
//       {"/dials/processing/group_0/n_signal"},
//       {"/dials/processing/group_0/panel"},
//       {"/dials/processing/group_0/xyzobs.px.value"},
//       {"/dials/processing/group_0/xyzobs.px.variance"}};

//   // Check that all expected datasets exist
//   for (const auto &expected_path : expected_datasets) {
//     auto it = std::find(datasets.begin(), datasets.end(), expected_path);
//     EXPECT_NE(it, datasets.end()) << "Dataset not found: " << expected_path;
//   }

//   // Ensure no extra datasets are present
//   EXPECT_EQ(datasets.size(), expected_datasets.size());
// }

// TEST_F(HDF5ReadTest, GetDatasetName) {
//   std::string full_path = "/some/long/path/to/dataset";
//   std::string dataset_name = get_dataset_name(full_path);

//   EXPECT_EQ(dataset_name, "dataset");
// }

#pragma endregion Discovery tests
