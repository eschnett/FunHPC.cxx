#include <adt/par_decl.hpp>

#include <adt/dummy.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <adt/par_impl.hpp>

#include <gtest/gtest.h>

template <typename T>
using shared_vector =
    adt::par<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;

TEST(adt_par, basic) { shared_vector<int> xs; }
