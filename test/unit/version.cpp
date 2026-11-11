#include "api/stc.h"
#include "meta.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("library version test", "[sanity]") {
    REQUIRE(stc::api::stc_abi_version() == stc::meta::version_major);
}
