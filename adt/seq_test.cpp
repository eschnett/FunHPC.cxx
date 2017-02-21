#include <adt/seq_decl.hpp>

#include <adt/dummy.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <adt/seq_impl.hpp>

#include <gtest/gtest.h>

template <typename T>
using shared_vector =
    adt::seq<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;

TEST(adt_seq, basic) { shared_vector<int> xs; }
