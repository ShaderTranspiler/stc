#pragma once

#include "api/export.h"

#ifdef _MSC_VER
    #define STC_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define STC_FORCE_INLINE inline
#endif

#ifdef _MSC_VER
    #define STC_PRETTY_FUNC __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_PRETTY_FUNC __PRETTY_FUNCTION__
#else
    #define STC_PRETTY_FUNC ""
#endif
