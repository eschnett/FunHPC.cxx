#include <adt/index.hpp>

#include <gtest/gtest.h>

TEST(adt_index, types) {
  adt::irange_t{};
  adt::index_t<2>{};
  adt::range_t<2>{};
}
