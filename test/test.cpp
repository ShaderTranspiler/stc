#include <catch2/catch_test_macros.hpp>
#include <temp.h>

TEST_CASE("catch2 base test", "[sanity]") {
    REQUIRE(1 == 1);
}

TEST_CASE("library linking base test", "[sanity]") {
    REQUIRE(stc::Temp::retOne() == 1);
}
