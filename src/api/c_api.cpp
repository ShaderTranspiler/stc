#include "api/stc.h"

#include <type_traits>

#include "api/stc.hpp"
#include "common/config.hpp"
#include "meta.hpp"

#include "external/julia.hpp"

#define STC_GET_UNDERLYING(value) static_cast<std::underlying_type_t<decltype(value)>>(value)
#define STATIC_ASSERT_IS_SV_NULL_TERMINATED(string_view)                                           \
    static_assert((string_view).data()[(string_view).length()] == '\0',                            \
                  #string_view " is not null-terminated")

// catch libjulia drift
static_assert(std::same_as<_jl_value_t, jl_value_t>,
              "forward declaration in stc.h is no longer valid");

namespace stc::api {

extern "C" {
    static_assert(STC_ABI_VERSION == stc::meta::version_major,
                  "STC_ABI_VERSION drifted from stc::meta::version_major");

    STC_API uint8_t stc_abi_version() noexcept {
        return stc::meta::version_major;
    }

    STC_API uint8_t stc_version_major() noexcept {
        return stc::meta::version_major;
    }

    STC_API uint8_t stc_version_minor() noexcept {
        return stc::meta::version_minor;
    }

    STC_API uint8_t stc_version_patch() noexcept {
        return stc::meta::version_patch;
    }

    STC_API const char* stc_version_string(bool include_build_commit_hash) noexcept {
        STATIC_ASSERT_IS_SV_NULL_TERMINATED(stc::meta::version);
        STATIC_ASSERT_IS_SV_NULL_TERMINATED(stc::meta::version_and_build_commit);

        return include_build_commit_hash ? stc::meta::version_and_build_commit.data()
                                         : stc::meta::version.data();
    }

    STC_API const STC_ResultHandle STC_NULL_RESULT = nullptr;
    STC_API const STC_ConfigHandle STC_NULL_CONFIG = nullptr;

    static_assert(std::same_as<std::underlying_type_t<Status>, STC_Status>,
                  "STC_Status no longer reflects Status's underlying type");
    static_assert(STC_GET_UNDERLYING(Status::COUNT) == 3, "Missing STC_Status constants in C API");
    // clang-format off
    STC_API const STC_Status STC_SUCCESS        = STC_GET_UNDERLYING(Status::Success);
    STC_API const STC_Status STC_FAILED         = STC_GET_UNDERLYING(Status::Failed);
    STC_API const STC_Status STC_INTERNAL_ERROR = STC_GET_UNDERLYING(Status::InternalError);
    // clang-format on

    static_assert(std::same_as<std::underlying_type_t<::stc::DumpVerbosity>, STC_DumpVerbosity>,
                  "STC_DumpVerbosity no longer reflects stc::DumpVerbosity's underlying type");
    static_assert(STC_GET_UNDERLYING(DumpVerbosity::COUNT) == 3,
                  "Missing STC_DumpVerbosity constants in C API");
    // clang-format off
    STC_API const STC_DumpVerbosity STC_DUMP_VERBOSITY_NONE    = STC_GET_UNDERLYING(DumpVerbosity::None);
    STC_API const STC_DumpVerbosity STC_DUMP_VERBOSITY_PARTIAL = STC_GET_UNDERLYING(DumpVerbosity::Partial);
    STC_API const STC_DumpVerbosity STC_DUMP_VERBOSITY_VERBOSE = STC_GET_UNDERLYING(DumpVerbosity::Verbose);
    // clang-format on
}

} // namespace stc::api

#undef STATIC_ASSERT_IS_SV_NULL_TERMINATED
#undef STC_GET_UNDERLYING
