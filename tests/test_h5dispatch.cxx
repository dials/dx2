#include <dx2/h5/h5dispatch.hpp>
#include <gtest/gtest.h>
#include <hdf5.h>

namespace {

template <typename List> struct to_gtest_types;
template <typename... Ts> struct to_gtest_types<h5dispatch::type_list<Ts...>> {
  using type = ::testing::Types<Ts...>;
};

} // namespace

/*
 * Typed over every entry in h5dispatch::ColumnTypes, so a newly added
 * column type is covered without editing this file.
 */
template <typename T> class H5TypeTraitsTest : public ::testing::Test {};
TYPED_TEST_SUITE(H5TypeTraitsTest,
                 to_gtest_types<h5dispatch::ColumnTypes>::type);

TYPED_TEST(H5TypeTraitsTest, MakeReturnsATypeMatchingTheTraits) {
  using Traits = h5dispatch::H5TypeTraits<TypeParam>;
  h5utils::H5Type type = h5dispatch::make_h5_type<TypeParam>();

  ASSERT_TRUE(type) << "make_h5_type returned an invalid id";
  EXPECT_EQ(H5Tget_class(type), Traits::h5_class)
      << "HDF5 class differs from H5TypeTraits::h5_class";
  EXPECT_EQ(H5Tget_size(type), sizeof(TypeParam))
      << "HDF5 size differs from sizeof the C++ type";
  if constexpr (Traits::h5_class == H5T_INTEGER) {
    EXPECT_EQ(H5Tget_sign(type), Traits::h5_sign)
        << "HDF5 sign differs from H5TypeTraits::h5_sign";
  }
}

TYPED_TEST(H5TypeTraitsTest, EachCallReturnsADistinctTypeOwnedByTheCaller) {
  h5utils::H5Type first = h5dispatch::make_h5_type<TypeParam>();
  h5utils::H5Type second = h5dispatch::make_h5_type<TypeParam>();
  ASSERT_NE(first.id, second.id)
      << "Two calls returned the same id, so the type is shared";

  // Closing one must succeed, which fails for a predefined type, and must
  // leave the other untouched.
  EXPECT_GE(H5Tclose(first.id), 0) << "Caller could not close its own type";
  first.id = -1;
  EXPECT_GT(H5Iis_valid(second), 0) << "Closing one type invalidated another";
}
