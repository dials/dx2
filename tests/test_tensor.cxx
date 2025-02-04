#include <dx2/tensor.hpp>
#include <gtest/gtest.h>
#include <vector>

// Test Fixture for 2D and higher-dimensional tensor tests
class TensorTest : public ::testing::Test {
protected:
  Tensor<int> tensor2D;
  Tensor<int> tensor3D;
  Tensor<float> tensor4D;
  Tensor<double> tensor5D;
  Tensor<int> tensor6D;

  void SetUp() override {
    tensor2D = Tensor<int>({3, 3});
    tensor3D = Tensor<int>({3, 4, 5});
    tensor4D = Tensor<float>({2, 3, 4, 5});
    tensor5D = Tensor<double>({2, 3, 4, 5, 6});
    tensor6D = Tensor<int>({2, 2, 2, 2, 2, 2});
  }
};

// --------------- BASIC FUNCTIONALITY TESTS ---------------
#pragma region Basic functionality tests

TEST_F(TensorTest, BasicAccess) {
  tensor2D(0, 0) = 0;
  tensor2D(0, 1) = 1;
  tensor2D(1, 1) = 4;

  EXPECT_EQ(tensor2D(0, 0), 0);
  EXPECT_EQ(tensor2D(1, 1), 4);

  // Modify value and check
  tensor2D(1, 2) = 42;
  EXPECT_EQ(tensor2D(1, 2), 42);
}

TEST_F(TensorTest, SizeComputation) {
  Tensor<int> tensor({4, 5});
  EXPECT_EQ(tensor.size(), 20); // 4 * 5 = 20 elements
}

#pragma endregion Basic functionality tests
// --------------- MEMORY LAYOUT TESTS ---------------
#pragma region Memory layout tests

class TensorLayoutTest : public ::testing::TestWithParam<Layout> {};

TEST_P(TensorLayoutTest, CheckMemoryLayout) {
  Layout layout = GetParam();
  std::vector<int> data(3 * 3);
  for (size_t i = 0; i < data.size(); ++i)
    data[i] = i;

  Tensor<int> tensor({3, 3}, data, layout);

  if (layout == Layout::RowMajor) {
    /*
     * Expected shape:
     * 0 1 2
     * 3 4 5
     * 6 7 8
     */
    EXPECT_EQ(tensor(0, 1), 1);
    EXPECT_EQ(tensor(1, 0), 3);
  } else if (layout == Layout::ColumnMajor) {
    /*
     * Expected shape:
     * 0 3 6
     * 1 4 7
     * 2 5 8
     */
    EXPECT_EQ(tensor(0, 1), 3);
    EXPECT_EQ(tensor(1, 0), 1);
  }
}

INSTANTIATE_TEST_SUITE_P(TensorLayoutTests, TensorLayoutTest,
                         ::testing::Values(Layout::RowMajor,
                                           Layout::ColumnMajor));

#pragma endregion Memory layout tests
// --------------- MULTI-DIMENSIONAL ACCESS TESTS ---------------
#pragma region Multi-dimensional access tests

TEST_F(TensorTest, ThreeDimensionalAccess) {
  tensor3D(1, 2, 3) = 42;
  EXPECT_EQ(tensor3D(1, 2, 3), 42);
}

TEST_F(TensorTest, FourDimensionalAccess) {
  tensor4D(1, 2, 3, 4) = 3.14f;
  EXPECT_FLOAT_EQ(tensor4D(1, 2, 3, 4), 3.14f);
}

TEST_F(TensorTest, FiveDimensionalAccess) {
  tensor5D(0, 1, 2, 3, 4) = 6.28;
  EXPECT_DOUBLE_EQ(tensor5D(0, 1, 2, 3, 4), 6.28);
}

TEST_F(TensorTest, SixDimensionalAccess) {
  tensor6D(1, 1, 1, 1, 1, 1) = 123;
  EXPECT_EQ(tensor6D(1, 1, 1, 1, 1, 1), 123);
}

#pragma endregion Multi - dimensional access tests
// --------------- OUT OF BOUNDS TESTS ---------------
#pragma region Out of bounds tests

TEST_F(TensorTest, OutOfBounds) {
  EXPECT_THROW(tensor2D(3, 0), OutOfBoundsException);
  EXPECT_THROW(tensor2D(0, 3), OutOfBoundsException);
}

TEST_F(TensorTest, HighDimensionalOutOfBounds) {
  EXPECT_THROW(tensor5D(2, 0, 0, 0, 0), OutOfBoundsException);
  EXPECT_THROW(tensor5D(0, 3, 0, 0, 0), OutOfBoundsException);
  EXPECT_THROW(tensor5D(0, 0, 4, 0, 0), OutOfBoundsException);
  EXPECT_THROW(tensor5D(0, 0, 0, 5, 0), OutOfBoundsException);
  EXPECT_THROW(tensor5D(0, 0, 0, 0, 6), OutOfBoundsException);
}

#pragma endregion Out of bounds tests

// --------------- RESIZE TESTS ---------------
#pragma region Resize tests

TEST_F(TensorTest, ResizeTensor) {
  tensor2D(0, 0) = 10;
  tensor2D.resize({4, 4});

  EXPECT_EQ(tensor2D.shape()[0], 4);
  EXPECT_EQ(tensor2D.shape()[1], 4);
  EXPECT_EQ(tensor2D(0, 0), 10); // Old data should persist
}

#pragma endregion Resize tests

// --------------- FILTER & ROW REMOVAL TESTS ---------------
#pragma region Filter and row removal tests

TEST_F(TensorTest, RemoveRows) {
  for (size_t i = 0, k = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j, ++k) {
      tensor2D(i, j) = k;
    }
  }

  /*
   * Tensor data:
   * 0 1 2
   * 3 4 5
   * 6 7 8
   */

  std::unordered_set<size_t> filter = {0, 2};
  // Remove the mismatching rows
  tensor2D.remove_rows(filter);

  /*
   * Expected filtered tensor:
   * Row 0: 0 1 2 ✘
   * Row 1: 3 4 5 ✔
   * Row 2: 6 7 8 ✘
   *
   * Rows 0 and 2 should be removed
   *
   * Expected tensor:
   * Row 0: 3 4 5
   */

  // Check the tensor after removal
  EXPECT_EQ(tensor2D(0, 0), 3);
  EXPECT_EQ(tensor2D(0, 1), 4);
  EXPECT_EQ(tensor2D(0, 2), 5);
  EXPECT_THROW(tensor2D(1, 0), OutOfBoundsException);
  EXPECT_THROW(tensor2D(2, 0), OutOfBoundsException);
  // Check shape
  EXPECT_EQ(tensor2D.shape()[0], 1);
  EXPECT_EQ(tensor2D.shape()[1], 3);
}

TEST_F(TensorTest, MismatchRows) {
  for (size_t i = 0, k = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j, ++k) {
      tensor2D(i, j) = k;
    }
  }

  /*
   * Tensor data:
   * 0 1 2
   * 3 4 5
   * 6 7 8
   */

  // Create a predicate the returns true for elements less than 4
  // This will filter out the last row as it is the onlt row where
  // thhe first element is greater than 4
  auto filter = [](const int &x) { return x < 4; };
  // Get the rows that do not match the filter
  auto mismatching_rows = tensor2D.mismatch_rows(0, filter);

  /*
   * Expected filtered tensor:
   * Row 0: 0 1 2 ✔
   * Row 1: 3 4 5 ✔
   * Row 2: 6 7 8 ✘
   *
   * mismatching_rows should contain row 2
   */

  // Expected rows: 2
  EXPECT_EQ(mismatching_rows.size(), 1);
  EXPECT_TRUE(mismatching_rows.find(0) == mismatching_rows.end());
  EXPECT_TRUE(mismatching_rows.find(1) == mismatching_rows.end());
  // The row with index 2 should not be in the set
  EXPECT_TRUE(mismatching_rows.find(2) != mismatching_rows.end());
}

TEST_F(TensorTest, FilterRows) {
  for (size_t i = 0, k = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j, ++k) {
      tensor2D(i, j) = k;
    }
  }

  /*
   * Tensor data:
   * 0 1 2
   * 3 4 5
   * 6 7 8
   */

  // Create a predicate the returns true for elements less than 4
  // This will filter out the last row as it is the onlt row where
  // thhe first element is greater than 4
  auto filter = [](const int &x) { return x < 4; };
  // Filter the rows
  auto mismatching_rows = tensor2D.filter_rows(0, filter);

  /*
   * Expected filtered tensor:
   * Row 0: 0 1 2 ✔
   * Row 1: 3 4 5 ✔
   * Row 2: 6 7 8 ✘
   *
   * Expected tensor:
   * Row 0: 0 1 2
   * Row 1: 3 4 5
   */

  // Check the tensor after filtering
  EXPECT_EQ(tensor2D(0, 0), 0);
  EXPECT_EQ(tensor2D(0, 1), 1);
  EXPECT_EQ(tensor2D(0, 2), 2);
  EXPECT_EQ(tensor2D(1, 0), 3);
  EXPECT_EQ(tensor2D(1, 1), 4);
  EXPECT_EQ(tensor2D(1, 2), 5);
  EXPECT_THROW(tensor2D(2, 0), OutOfBoundsException);
  // Check shape
  EXPECT_EQ(tensor2D.shape()[0], 2);
  EXPECT_EQ(tensor2D.shape()[1], 3);
}

#pragma endregion Filter and row removal tests