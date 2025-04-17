#include <dx2/reflection.hpp>
#include <experimental/mdspan>
#include <filesystem>
#include <gtest/gtest.h>
#include <iomanip> // for std::setprecision
#include <iostream>
#include <string>
#include <vector>

/*
 * Test fixture for ReflectionTable tests. This fixture sets up the test
 * environment for the ReflectionTable class and provides a common path
 * to the test file.
 */
class ReflectionTableTest : public ::testing::Test {
protected:
  std::filesystem::path test_file_path;

  void SetUp() override {
    test_file_path = std::filesystem::current_path() / "data/cut_strong.refl";
  }
};

#pragma region ReflectionTable Tests
// Test loading data from an HDF5 file

TEST_F(ReflectionTableTest, LoadDataFromHDF5) {
  ReflectionTable table(test_file_path.string());

  const auto &column_names = table.get_column_names();
  EXPECT_FALSE(column_names.empty());

  std::cout << "Loaded column names:\n";
  for (const auto &name : column_names) {
    std::cout << "  - " << name << "\n";
  }
}

TEST_F(ReflectionTableTest, GetTypedColumnWithCorrectType) {
  ReflectionTable table(test_file_path.string());

  auto *col = table.get_column<double>("xyzobs.px.value");
  ASSERT_NE(col, nullptr);

  auto shape = col->get_shape();
  EXPECT_EQ(shape.size(), 2);
  EXPECT_EQ(shape[1], 3);
  EXPECT_GT(shape[0], 0); // Must have at least one row

  const auto &span = col->span();

  std::cout << "First 5 rows of xyzobs.px.value:\n";
  for (size_t i = 0; i < std::min(shape[0], size_t(5)); ++i) {
    std::cout << "  [" << i << "] ";
    for (size_t j = 0; j < shape[1]; ++j) {
      std::cout << std::fixed << std::setprecision(4) << span(i, j) << " ";
    }
    std::cout << "\n";
  }

  EXPECT_NEAR(span(0, 0), 1190.93, 1e-2);
}

TEST_F(ReflectionTableTest, GetTypedColumnWithWrongTypeReturnsNullptr) {
  ReflectionTable table(test_file_path.string());

  auto *col = table.get_column<int>("xyzobs.px.value");
  EXPECT_EQ(col, nullptr); // should be double
}

TEST_F(ReflectionTableTest, GetAllColumnNamesAndShapes) {
  ReflectionTable table(test_file_path.string());
  auto names = table.get_column_names();

  EXPECT_NE(std::find(names.begin(), names.end(), "xyzobs.px.value"),
            names.end());

  std::cout << "Column shapes:\n";

  for (const auto &name : names) {
    if (auto *col = table.get_column<double>(name)) {
      auto shape = col->get_shape();
      std::cout << "  - " << name << " [double] shape: ";
      for (auto dim : shape)
        std::cout << dim << " ";
      std::cout << "\n";

      EXPECT_GE(shape.size(), 1);
      EXPECT_LE(shape.size(), 2);

    } else if (auto *col = table.get_column<int>(name)) {
      auto shape = col->get_shape();
      std::cout << "  - " << name << " [int] shape: ";
      for (auto dim : shape)
        std::cout << dim << " ";
      std::cout << "\n";

      EXPECT_GE(shape.size(), 1);
      EXPECT_LE(shape.size(), 2);

    } else {
      std::cout << "  - " << name << " [unsupported type or missing]\n";
    }
  }
}

TEST_F(ReflectionTableTest, PrintAllColumnContents) {
  ReflectionTable table(test_file_path.string());

  for (const auto &name : table.get_column_names()) {
    if (auto *col = table.get_column<double>(name)) {
      auto span = col->span();
      std::cout << "[double] " << name << ":\n";
      for (size_t i = 0; i < col->get_shape()[0]; ++i) {
        std::cout << "  [" << i << "] ";
        for (size_t j = 0; j < col->get_shape()[1]; ++j) {
          std::cout << std::fixed << std::setprecision(4) << span(i, j) << " ";
        }
        std::cout << "\n";
      }
    } else if (auto *col = table.get_column<int>(name)) {
      auto span = col->span();
      std::cout << "[int] " << name << ":\n";
      for (size_t i = 0; i < col->get_shape()[0]; ++i) {
        std::cout << "  [" << i << "] ";
        for (size_t j = 0; j < col->get_shape()[1]; ++j) {
          std::cout << span(i, j) << " ";
        }
        std::cout << "\n";
      }
    }
  }
}

TEST_F(ReflectionTableTest, LoadFromNonExistentFileThrows) {
  EXPECT_THROW(ReflectionTable table("does_not_exist.h5"), std::runtime_error);
}

TEST_F(ReflectionTableTest, LoadFileMissingGroupSkipsQuietly) {
  std::filesystem::path invalid_path =
      std::filesystem::current_path() / "data/empty.h5";
  hid_t file =
      H5Fcreate(invalid_path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  H5Fclose(file);

  EXPECT_NO_THROW({
    ReflectionTable table(invalid_path.string());
    EXPECT_TRUE(table.get_column_names().empty());
  });
}

TEST_F(ReflectionTableTest, SelectZGreaterThanThreshold) {
  ReflectionTable table(test_file_path.string());

  // Select rows where Z > 1.0 from xyzobs.px.value
  auto filtered = table.select<double>("xyzobs.px.value", [](auto row) {
    return row[2] > 1.0; // Z component
  });

  std::cout << "Selected "
            << filtered.get_column<double>("xyzobs.px.value")->get_shape()[0]
            << " rows where Z > 1.0\n";

  // All Zs should be > 1.0 in the result
  auto *col = filtered.get_column<double>("xyzobs.px.value");
  ASSERT_NE(col, nullptr);

  const auto &span = col->span();
  for (size_t i = 0; i < col->get_shape()[0]; ++i) {
    EXPECT_GT(span(i, 2), 1.0) << "Row " << i << " has Z = " << span(i, 2);
  }
}

TEST_F(ReflectionTableTest, SelectAllRows) {
  ReflectionTable table(test_file_path.string());

  auto *col = table.get_column<double>("xyzobs.px.value");
  ASSERT_NE(col, nullptr);
  size_t original_rows = col->get_shape()[0];

  // Select all
  auto filtered =
      table.select<double>("xyzobs.px.value", [](auto) { return true; });

  auto *filtered_col = filtered.get_column<double>("xyzobs.px.value");
  ASSERT_NE(filtered_col, nullptr);
  EXPECT_EQ(filtered_col->get_shape()[0], original_rows);
}

TEST_F(ReflectionTableTest, SelectNoRows) {
  ReflectionTable table(test_file_path.string());

  // Select none
  auto filtered =
      table.select<double>("xyzobs.px.value", [](auto) { return false; });

  auto *col = filtered.get_column<double>("xyzobs.px.value");
  ASSERT_NE(col, nullptr);
  EXPECT_EQ(col->get_shape()[0], 0);
}

TEST_F(ReflectionTableTest, SelectThrowsOnWrongColumnName) {
  ReflectionTable table(test_file_path.string());

  EXPECT_THROW(
      {
        table.select<double>("nonexistent_column", [](auto) { return true; });
      },
      std::runtime_error);
}

TEST_F(ReflectionTableTest, SelectThrowsOnWrongType) {
  ReflectionTable table(test_file_path.string());

  // xyzobs.px.value is a double column
  EXPECT_THROW(
      { table.select<int>("xyzobs.px.value", [](auto) { return true; }); },
      std::runtime_error);
}

class ReflectionTableAddWriteTest : public ::testing::Test {
protected:
  std::filesystem::path test_file_path;
  std::filesystem::path output_file_path;

  void SetUp() override {
    test_file_path = std::filesystem::current_path() / "data/cut_strong.refl";
    output_file_path =
        std::filesystem::current_path() / "data/test_write_output.h5";

    // Remove old test output if it exists
    if (std::filesystem::exists(output_file_path)) {
      std::filesystem::remove(output_file_path);
    }
  }
};

TEST_F(ReflectionTableAddWriteTest, AddColumnAndCheckContents) {
  ReflectionTable table(test_file_path.string());
  size_t rows = table.get_column<double>("xyzobs.px.value")->get_shape()[0];

  // Create dummy int column to add
  std::vector<int> num_pixels(rows, 42);
  table.add_column("num_pixels", rows, 1, num_pixels);

  // Verify column added
  auto *col = table.get_column<int>("num_pixels");
  ASSERT_NE(col, nullptr);
  EXPECT_EQ(col->get_shape(), std::vector<size_t>({rows, 1}));
  for (size_t i = 0; i < rows; ++i) {
    EXPECT_EQ(col->span()(i, 0), 42);
  }
}

TEST_F(ReflectionTableAddWriteTest, WriteToFileAndReadBack) {
  ReflectionTable table(test_file_path.string());
  size_t rows = table.get_column<double>("xyzobs.px.value")->get_shape()[0];

  // Add dummy column
  std::vector<int> num_pixels(rows);
  for (size_t i = 0; i < rows; ++i)
    num_pixels[i] = static_cast<int>(i * 10);
  table.add_column("num_pixels", rows, 1, num_pixels);

  // Write to new file
  table.write(output_file_path.string());

  // Reload from the written file
  ReflectionTable reloaded(output_file_path.string());

  // Verify original column
  auto *col_xyz = reloaded.get_column<double>("xyzobs.px.value");
  ASSERT_NE(col_xyz, nullptr);
  EXPECT_EQ(col_xyz->get_shape()[0], rows);

  // Verify newly written column
  auto *col_num_pixels = reloaded.get_column<int>("num_pixels");
  ASSERT_NE(col_num_pixels, nullptr);
  for (size_t i = 0; i < rows; ++i) {
    EXPECT_EQ(col_num_pixels->span()(i, 0), static_cast<int>(i * 10));
  }
}

#pragma endregion