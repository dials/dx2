#include <dx2/reflection.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <hdf5.h>
#include <string>
#include <vector>

// Test fixture for ReflectionTable tests
class ReflectionTableTest : public ::testing::Test {
protected:
  std::filesystem::path test_file_path;

  void SetUp() override {
    test_file_path = std::filesystem::current_path() / "data/cut_strong.refl";

    // Create the test dataset if it does not exist
    create_test_dataset(test_file_path,
                        "/dials/processing/group_0/xyzobs.px.value");
  }

  void create_test_dataset(const std::string &filename,
                           const std::string &dataset_path) {
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (file < 0) {
      throw std::runtime_error("Error: Unable to open file: " + filename);
    }

    hid_t dataset = H5Dopen(file, dataset_path.c_str(), H5P_DEFAULT);
    if (dataset >= 0) {
      H5Dclose(dataset);
      H5Fclose(file);
      return; // Dataset already exists
    }

    // Create a simple 5x3 dataset for testing
    hsize_t dims[2] = {5, 3}; // 5 rows, 3 columns (X, Y, Z)
    hid_t dataspace = H5Screate_simple(2, dims, NULL);
    dataset = H5Dcreate(file, dataset_path.c_str(), H5T_NATIVE_DOUBLE,
                        dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
      H5Sclose(dataspace);
      H5Fclose(file);
      throw std::runtime_error("Error: Unable to create test dataset: " +
                               dataset_path);
    }

    // Example reflection table data (5 rows, 3 columns: X, Y, Z)
    double test_data[5][3] = {
        {1.1, 2.2, 3.3},    {4.4, 5.5, 6.6},    {7.7, 8.8, 9.9},
        {10.1, 11.2, 12.3}, {13.4, 14.5, 15.6},
    };

    H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
             test_data);

    // Close handles
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Fclose(file);
  }
};

// --------------- ReflectionTable Tests ---------------
#pragma region ReflectionTable tests

// // Test loading data from an HDF5 file
// TEST_F(ReflectionTableTest, LoadDataFromHDF5) {
//   ReflectionTable table(test_file_path.string());

//   EXPECT_NO_THROW(
//       table.select(ReflectionTable::Column::Z, [](double) { return true; }));
// }

// // Test selecting reflections where Z > 10
// TEST_F(ReflectionTableTest, SelectReflectionsByZ) {
//   ReflectionTable table(test_file_path.string());

//   table.select(ReflectionTable::Column::Z, [](double z) { return z > 10.0;
//   });

//   EXPECT_EQ(table.column(ReflectionTable::Column::Z).size(), 2);
//   EXPECT_EQ(table.column(ReflectionTable::Column::Z)[0], 12.3);
//   EXPECT_EQ(table.column(ReflectionTable::Column::Z)[1], 15.6);
// }

// // Test selecting reflections where X < 5
// TEST_F(ReflectionTableTest, SelectReflectionsByX) {
//   ReflectionTable table(test_file_path.string());

//   table.select(ReflectionTable::Column::X, [](double x) { return x < 5.0; });

//   EXPECT_EQ(table.column(ReflectionTable::Column::X).size(), 2);
//   EXPECT_EQ(table.column(ReflectionTable::Column::X)[0], 1.1);
//   EXPECT_EQ(table.column(ReflectionTable::Column::X)[1], 4.4);
// }

// // Test retrieving a single column
// TEST_F(ReflectionTableTest, RetrieveColumnY) {
//   ReflectionTable table(test_file_path.string());

//   std::vector<double> y_values = table.column(ReflectionTable::Column::Y);

//   EXPECT_EQ(y_values.size(), 5);
//   EXPECT_EQ(y_values[0], 2.2);
//   EXPECT_EQ(y_values[1], 5.5);
//   EXPECT_EQ(y_values[2], 8.8);
//   EXPECT_EQ(y_values[3], 11.2);
//   EXPECT_EQ(y_values[4], 14.5);
// }

// // Test selecting reflections on an empty dataset
// TEST_F(ReflectionTableTest, SelectOnEmptyDataset) {
//   std::string empty_dataset = "/dials/processing/empty_dataset";
//   create_test_dataset(test_file_path.string(), empty_dataset);

//   ReflectionTable table(test_file_path.string());

//   EXPECT_NO_THROW(
//       table.select(ReflectionTable::Column::Z, [](double) { return true; }));

//   EXPECT_EQ(table.column(ReflectionTable::Column::Z).size(), 0);
// }

// // Test reading from a non-existent file
// TEST_F(ReflectionTableTest, ReadFromNonExistentFile) {
//   std::string invalid_file = "invalid_file.h5";

//   EXPECT_THROW(ReflectionTable table(invalid_file), std::runtime_error);
// }

// // Test reading a non-existent dataset
// TEST_F(ReflectionTableTest, ReadNonExistentDataset) {
//   std::string invalid_dataset = "/this/does/not/exist";

//   EXPECT_THROW(ReflectionTable table(invalid_dataset), std::runtime_error);
// }

// // Test if data selection keeps shape integrity
// TEST_F(ReflectionTableTest, ShapeAfterSelect) {
//   ReflectionTable table(test_file_path.string());

//   table.select(ReflectionTable::Column::Z, [](double z) { return z > 5.0; });

//   EXPECT_EQ(table.column(ReflectionTable::Column::X).size(), 4);
//   EXPECT_EQ(table.column(ReflectionTable::Column::Y).size(), 4);
//   EXPECT_EQ(table.column(ReflectionTable::Column::Z).size(), 4);
// }

#pragma endregion ReflectionTable tests
