#include <funhpc/main>

#include <gtest/gtest.h>

int funhpc_main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
