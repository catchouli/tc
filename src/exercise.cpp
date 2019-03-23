#include <catch.hpp>

int test() {
  return 5;
}

TEST_CASE("test") {
  CHECK(test() == 4);
}