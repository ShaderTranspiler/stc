#include "api/stc.h"

#include <type_traits>

#include "api/stc.hpp"
#include "common/config.h"
#include "meta.h"

#include "external/julia.h"

#define STC_GET_UNDERLYING(value) static_cast<std::underlying_type_t<decltype(value)>>(value)

// catch libjulia drift
static_assert(std::same_as<_jl_value_t, jl_value_t>,
              "forward declaration in stc.h is no longer valid");

namespace stc::api {

extern "C" {
    STC_API const STC_ResultHandle STC_NULL_RESULT = nullptr;
    STC_API const STC_ConfigHandle STC_NULL_CONFIG = nullptr;

    static_assert(std::same_as<std::underlying_type_t<Status>, STC_StatusValue>,
                  "STC_StatusValue no longer reflects Status's underlying type");
    static_assert(STC_GET_UNDERLYING(Status::COUNT) == 3,
                  "Missing STC_StatusValue constants in C API");
    // clang-format off
    STC_API const STC_StatusValue STC_SUCCESS        = STC_GET_UNDERLYING(Status::Success);
    STC_API const STC_StatusValue STC_FAILED         = STC_GET_UNDERLYING(Status::Failed);
    STC_API const STC_StatusValue STC_INTERNAL_ERROR = STC_GET_UNDERLYING(Status::InternalError);
    // clang-format on

    static_assert(
        std::same_as<std::underlying_type_t<::stc::DumpVerbosity>, STC_DumpVerbosityValue>,
        "STC_DumpVerbosityValue no longer reflects stc::DumpVerbosity's underlying type");
    static_assert(STC_GET_UNDERLYING(DumpVerbosity::COUNT) == 3,
                  "Missing STC_DumpVerbosityValue constants in C API");
    // clang-format off
    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_NONE    = STC_GET_UNDERLYING(DumpVerbosity::None);
    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_PARTIAL = STC_GET_UNDERLYING(DumpVerbosity::Partial);
    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_VERBOSE = STC_GET_UNDERLYING(DumpVerbosity::Verbose);
    // clang-format on

    STC_API uint8_t stc_abi_version() noexcept {
        return stc::meta::version_major;
    }
}

} // namespace stc::api

#undef STC_GET_UNDERLYING
