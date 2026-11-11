#include "api/core.h"

#include <type_traits>

#define STC_GET_UNDERLYING(value) static_cast<std::underlying_type_t<decltype(value)>>(value)

namespace stc::api {

extern "C" {
    STC_API const STC_ResultHandle STC_NULL_RESULT = nullptr;
    STC_API const STC_ConfigHandle STC_NULL_CONFIG = nullptr;

    // clang-format off
    STC_API const STC_StatusValue STC_SUCCESS        = STC_GET_UNDERLYING(STC_Status::Success);
    STC_API const STC_StatusValue STC_FAILED         = STC_GET_UNDERLYING(STC_Status::Failed);
    STC_API const STC_StatusValue STC_INTERNAL_ERROR = STC_GET_UNDERLYING(STC_Status::InternalError);

    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_NONE    = STC_GET_UNDERLYING(DumpVerbosity::None);
    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_PARTIAL = STC_GET_UNDERLYING(DumpVerbosity::Partial);
    STC_API const STC_DumpVerbosityValue STC_DUMP_VERBOSITY_VERBOSE = STC_GET_UNDERLYING(DumpVerbosity::Verbose);
    // clang-format on
}

} // namespace stc::api

#undef STC_GET_UNDERLYING
